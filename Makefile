
CC=gcc -Wall

all : rlfe

clean:
	rm *.o rlfe

tbexec.o : tbexec.c 
	$(CC) -o tbexec.o -c tbexec.c

readline.o : readline.c tbexec.h
	$(CC) -o readline.o -c readline.c
	
rlfe : tbexec.o readline.o
	$(CC) -o rlfe tbexec.o readline.o -lreadline

