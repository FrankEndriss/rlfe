
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <malloc.h>
#include "tbexec.h"

#define PROMPT "> "

int aPipes[TB_PIPESSIZE];
FILE *slaveStdin;

extern char **environ;

int stdinClosed=0;

static void myLineHandler(char* line) {
	if(line && *line) { // not null and not empty
		add_history(line);
		// send line to working process
		fprintf(slaveStdin, "%s\n", line);
		fflush(slaveStdin);
		//fprintf(stderr, "line: %s\n", line);
	}
	if(line)
		free(line);
	else { // stdin of this proc was closed
		fclose(slaveStdin);
		stdinClosed=1;
	}
}

static void usage(char *arg0) {
	fprintf(stderr, "usage: %s %s\n", arg0, "<prog> [arg]...");
	exit(2);
}

static int do_fdset(int maxval, int fd, fd_set *set) {
	FD_SET(fd, set);
	return maxval>fd?maxval:fd;
}

/* reads a byte from infd and writes to outfd */
static size_t cpByte(int infd, int outfd) {
	char buf[1];
	if(read(infd, buf, 1)<=0)
		return -1;
	return write(outfd, buf, 1);
}

/**
 * Call like:
 * "rlfe java -jar toolbox.jar"
 * It will exec the first arg as a new process passing all remaining arguments as arguments.
 * The envron of this process is passed also to the execed process.
 * While execution it will read lines from stdin using the readline library, and passes them
 * to the executed process' using thats stdin.
 * It will put anything written to the executed process' stdout and stderr to this process'
 * stdout and stderr.
 * So, this is a simple readline-addon for the executed process. Usefull for programs where
 * readline is not available, like in java programs.
 * This programm quits when the sub-process quits.
 */
int main(int argc, char** argv) {
	int i;

	if(argc<2)
		usage(argv[0]);

	int mallocSize=sizeof(char*)*argc;
	//fprintf(stderr, "argc=%d; malloc: %d\n", argc, mallocSize);
	char **cArgs=malloc(mallocSize);
	for(i=1; i<argc; i++) {
		//fprintf(stderr, "copy arg: %s\n", argv[i]);
		cArgs[i-1]=argv[i];
	}
	//fprintf(stderr, "last idx=%d\n", i);
	cArgs[i-1]=0;

	/* Install myLineHandler to be called on any complete input line */
	rl_callback_handler_install(PROMPT, myLineHandler);

	/* create the slave process */
	int cPid=createChild(argv[1], cArgs, environ, aPipes);
	if(cPid<=0) {
		perror("cannot exec slave");
		exit(1);
	}

	/* open a FILE* to slaves stdin to use fprintf() */
	slaveStdin=fdopen(aPipes[TB_STDIN], "a");

	fd_set readfds;
	fd_set writefds;
	fd_set exepfds;

	int slaveOutClosed=0;
	int slaveErrClosed=0;

	do {
		int maxfds=0;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exepfds);

		if(!stdinClosed)
			maxfds=do_fdset(maxfds, STDIN_FILENO, &readfds);
		if(!slaveOutClosed)
			maxfds=do_fdset(maxfds, aPipes[TB_STDOUT], &readfds);
		if(!slaveErrClosed)
			maxfds=do_fdset(maxfds, aPipes[TB_STDERR], &readfds);

		// select() on the streams and do according io
		// until the streams of the slave process close
		int retval=select(maxfds+1, &readfds, &writefds, &exepfds, 0);
		if(retval<0) {
			perror("select()");
		} else if(retval==0)
			continue;	// timeout
		else {

			if((!slaveOutClosed) && FD_ISSET(aPipes[TB_STDOUT], &readfds))
				slaveOutClosed=(0==cpByte(aPipes[TB_STDOUT], STDOUT_FILENO));

			if((!slaveErrClosed) && FD_ISSET(aPipes[TB_STDERR], &readfds))
				slaveErrClosed=(0==cpByte(aPipes[TB_STDERR], STDERR_FILENO));

			// if stdin is availabe then read a char.
			// may be optimized to read while(availabe)... for longer inputs

			if((!stdinClosed) && FD_ISSET(STDIN_FILENO, &readfds)) {
				if(isatty(STDIN_FILENO))
					rl_callback_read_char();
				else
					stdinClosed=(0==cpByte(STDIN_FILENO, aPipes[TB_STDIN]));
			}
		}

	}while(!(slaveOutClosed && slaveErrClosed));
	rl_callback_handler_remove(); // restore the terminal

	int s;
	waitpid(cPid, &s, WNOHANG);
	if(WIFEXITED(s))
		exit(WEXITSTATUS(s));
	else if(WIFSIGNALED(s)) {
		if(WCOREDUMP(s)) {
			fprintf(stderr, "slave coredump\n");
			exit(EXIT_FAILURE);
		} else if(WIFSIGNALED(s)) {
			int sig=WTERMSIG(s);
			fprintf(stderr, "slave terminated by signal: %d\n", sig);
				exit(EXIT_SUCCESS);
		}
	}
	exit(EXIT_SUCCESS);
}
