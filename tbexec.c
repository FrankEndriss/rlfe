/*
 * tbexec.c
 *
 *  Created on: 29.11.2014
 *      Author: frank
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1


#include "tbexec.h"

/** Closes the pipes created by createChild(...)
 */
void closePipes(int aPipes[]) {
	int i;
	for(i=0; i<TB_PIPESSIZE; i++)
		close(aPipes[i]);
}

/* Creates a child process, redirecting io to the pipes.
 * The pipes field-descriptors are returned in the array. It should be allocated as int[TB_PIPESSIZE].
 * Refer to them as aPipes[TB_STDIN], aPipes[TB_STDOUT] and aPipes[TB_STDERR]
 *
 * @return the childs pid as returned by fork()
 */
int createChild(const char* szCommand, char* const aArguments[], char* const aEnvironment[], int aPipes[])
{
	int nChild;
	int nResult;
	int aStdinPipe[2];
	int aStdoutPipe[2];
	int aStderrPipe[2];

	if (pipe(aStdinPipe) < 0) {
		perror("allocating pipe for child stdin redirect");
		return -1;
	}
	if (pipe(aStdoutPipe) < 0) {
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		perror("allocating pipe for child stdout redirect");
		return -1;
	}
	if(pipe(aStderrPipe) < 0) {
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStdoutPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		perror("allocating pipe for child stderr redirect");
		return -1;
	}

	nChild = fork();
	if (0 == nChild) {
		// child continues here

		// redirect stdin
		if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
			perror("redirecting stdin");
			return -1;
		}

		// redirect stdout
		if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
			perror("redirecting stdout");
			return -1;
		}

		// redirect stderr
		if (dup2(aStderrPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
			perror("redirecting stderr");
			return -1;
		}

		// all these are for use by parent only
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStdoutPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_READ]);
		close(aStderrPipe[PIPE_WRITE]);

		// run child process image
		// replace this with any exec* function find easier to use ("man exec")
		nResult = execve(szCommand, aArguments, aEnvironment);

		// if we get here at all, an error occurred, but we are in the child
		// process, so just exit
		perror("exec of the child process");
		exit(nResult);
	} else if (nChild > 0) {
		// parent continues here

		// close unused file descriptors, these are for child only
		close(aStdinPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_WRITE]);

	} else {
		// failed to create child
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStdoutPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_READ]);
		close(aStderrPipe[PIPE_WRITE]);
	}

	aPipes[TB_STDIN]=aStdinPipe[PIPE_WRITE];
	aPipes[TB_STDOUT]=aStdoutPipe[PIPE_READ];
	aPipes[TB_STDERR]=aStderrPipe[PIPE_READ];
	return nChild;
}
