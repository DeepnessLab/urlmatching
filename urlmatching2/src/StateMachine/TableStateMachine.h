/*
 * TableStateMachine.h
 *
 *  Created on: Jan 23, 2011
 *      Author: yotamhc
 */

#ifndef TABLESTATEMACHINE_H_
#define TABLESTATEMACHINE_H_

#include "../Common/Types.h"
#include "../CommonCTypes.h"

//#include "../Dedup/Dictionary.h"

#ifdef COUNT_BY_DEPTH
#ifndef USE_DEPTHMAP
#define USE_DEPTHMAP
#endif
#endif
//typedef unsigned int STATE_PTR_TYPE_WIDE;

typedef struct {
	STATE_PTR_TYPE_WIDE *table;
	unsigned char *matches;
	char **patterns;
	unsigned int numStates;
#ifdef USE_DEPTHMAP
	int *depthMap;
#endif
} TableStateMachine;

typedef int (*callBackWithPattern)(char *,unsigned int, void* );


TableStateMachine *createTableStateMachine(unsigned int numStates);
void destroyTableStateMachine(TableStateMachine *machine);

void setGoto(TableStateMachine *machine, STATE_PTR_TYPE_WIDE currentState, char c, STATE_PTR_TYPE_WIDE nextState);
void setMatch(TableStateMachine *machine, STATE_PTR_TYPE_WIDE state, char *pattern, int length);

//int matchTableMachine(TableStateMachine *tableMachine, char *input, int length, int verbose);
int matchTableMachine(TableStateMachine *machine, const char *input, int length, int verbose, //verbose - print patterns or not
		long *numAccesses, long *accessesByDepth, long *accessesByState, unsigned int *lastState //all others are statistics - use NULL
		, callBackWithPattern patternFunc, void* data);
/*
int matchDictionaryTableMachine(TableStateMachine *machine, char *input, int length, int verbose,
			Dictionary *dict, int dict_word_size, RollingHashTool *hasher, long *totalSkipped, long *totalLeftBorderChars, long *totalLeftBorders,
			long *dictGetTries, long *memcmpTries, long *memcmpFails);
*/
void printMicros();

#endif /* TABLESTATEMACHINE_H_ */
