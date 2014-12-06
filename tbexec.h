/*
 * tbexec.h
 *
 *  Created on: 29.11.2014
 *      Author: frank
 */

#ifndef TBEXEC_H_
#define TBEXEC_H_

#define TB_STDIN 0
#define TB_STDOUT 1
#define TB_STDERR 2
#define TB_PIPESSIZE 3

int createChild(const char* szCommand, char* const aArguments[], char* const aEnvironment[], int aPipes[]);
void closePipes(int aPipes[]);

#endif /* TBEXEC_H_ */
