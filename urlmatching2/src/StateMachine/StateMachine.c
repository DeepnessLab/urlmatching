/*
 * StateMachine.c
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <math.h>
#include <assert.h>
#include "../Common/BitArray/BitArray.h"

#include "../Common/Flags.h"
#include "../States/LinearEncodedState.h"
#include "../States/BitmapEncodedState.h"
#include "../States/LookupTableState.h"
#include "../States/PathCompressedState.h"
#include "../States/SimpleLinearEncodedState.h"
#include "../Common/StateTable.h"
#include "../Common/FastLookup/FastLookup.h"
#include "StateMachine.h"



inline int getNextState(StateMachine *machine, State *current, char *str, int length
		, int *idx, NextStateResult *result, int verbose)
{
	StateHeader *header = (StateHeader*)current;

	switch (header->type) {
	case STATE_TYPE_LINEAR_ENCODED:
		return getNextState_LE(current, str, length, idx, result, machine, machine->patternTable, verbose);
	case STATE_TYPE_BITMAP_ENCODED:
		return getNextState_BM(current, str, length, idx, result, machine, machine->patternTable, verbose);
	case STATE_TYPE_LOOKUP_TABLE:
		return getNextState_LT(current, str, length, idx, result, machine, machine->patternTable, verbose);
	case STATE_TYPE_PATH_COMPRESSED:
		return getNextState_PC(current, str, length, idx, result, machine, machine->patternTable, verbose,NULL,NULL);
	}
	return FALSE;
}

#define GET_NEXT_STATE(machine, current, str, length, idx, result) 												\
		((GET_STATE_TYPE(current) == STATE_TYPE_LINEAR_ENCODED) ?													\
				getNextState_LE(current, str, length, idx, result, machine, machine->patternTable, verbose) :					\
				((GET_STATE_TYPE(current) == STATE_TYPE_BITMAP_ENCODED) ?												\
						getNextState_BM(current, str, length, idx, result, machine, machine->patternTable, verbose) :				\
						((GET_STATE_TYPE(current) == STATE_TYPE_LOOKUP_TABLE) ?											\
								getNextState_LT(current, str, length, idx, result, machine, machine->patternTable, verbose) :			\
								((GET_STATE_TYPE(current) == STATE_TYPE_PATH_COMPRESSED) ?										\
										getNextState_PC(current, str, length, idx, result, machine, machine->patternTable, verbose) : FALSE	\
								))))

int getStateID(State *state) {
	StateHeader *header = (StateHeader*)state;

	switch (header->type) {
	case STATE_TYPE_LINEAR_ENCODED:
		return getStateID_LE(state);
	case STATE_TYPE_BITMAP_ENCODED:
		return getStateID_BM(state);
	case STATE_TYPE_LOOKUP_TABLE:
		return getStateID_LT(state);
	case STATE_TYPE_PATH_COMPRESSED:
		return getStateID_PC(state);
	}
	return FALSE;
}

int matchRecursive(StateMachine *machine, char *input, int length, int *idx, State *s, int verbose)
{
	State *nextState;
	int res;
	NextStateResult next;

	if (*idx >= length)
		return 0;

	res = 0;

	//getNextState(machine, s, input, length, idx, &next);
	//GET_NEXT_STATE(machine, s, input, length, idx, &next);
	switch (GET_STATE_TYPE(s)) {
	case STATE_TYPE_LINEAR_ENCODED:
		getNextState_LE(s, input, length, idx, &next, machine, machine->patternTable, verbose);
		break;
	case STATE_TYPE_BITMAP_ENCODED:
		getNextState_BM(s, input, length, idx, &next, machine, machine->patternTable, verbose);
		break;
	case STATE_TYPE_LOOKUP_TABLE:
		getNextState_LT(s, input, length, idx, &next, machine, machine->patternTable, verbose);
		break;
	case STATE_TYPE_PATH_COMPRESSED:
		getNextState_PC(s, input, length, idx, &next, machine, machine->patternTable, verbose,NULL,NULL);
		break;
	}

	nextState = next.next;

	if ((s == nextState) && (s == (machine->states->table[0])) && (next.fail)) {
		(*idx)++;
	}

	if (next.match) {
#ifdef PRINT_MATCHES
		if (verbose) {
			printf("%s\n", next.pattern);
		}
#endif
		res = 1;
	}

#ifdef TRACE_STATE_MACHINE
	printf("Current state: %d, next char: %c, %s, next state: %d\n", getStateID(s), input[*idx], (next.match ? "matched" : "not matched"), getStateID(nextState));
#endif

	return matchRecursive(machine, input, length, idx, nextState, verbose) || res;
}


int matchIterative(StateMachine *machine, const char *input, int length, int *idx, State *s, int verbose, MachineStats *stats)
{
	State *nextState;
	int res;
	NextStateResult next;
	State **stateTable;
	STATE_PTR_TYPE nxtid;

#ifdef TRACE_STATE_MACHINE
	char lastC;
#endif

	res = 0;
	stateTable = machine->states->table;
	int realIdx = -1;

	while (*idx < length) {
		realIdx++;
#ifdef TRACE_STATE_MACHINE
		lastC = input[*idx];
#endif
		next.fail = FALSE;
		next.match = FALSE;
		//getNextState(machine, s, input, length, idx, &next);
		//GET_NEXT_STATE(machine, s, input, length, idx, &next);

		if (s == stateTable[0]) {
			nxtid = GET_FIRST_LEVEL_STATE(machine, input[*idx]);
			if (nxtid == LOOKUP_VALUE_NOT_FOUND) {
				next.fail = TRUE;
				nextState = s;
			} else {
				nextState = GET_STATE_POINTER_FROM_ID(machine, nxtid);
				if (nextState == s)
					next.fail = TRUE;
				else
					(*idx)++;
			}
			// THIS PREVENTS ONE CHAR MATCHES!
		} else {

			switch (GET_STATE_TYPE(s)) {
			case STATE_TYPE_LOOKUP_TABLE:
				//getNextState_LT(s, input, length, idx, &next, machine, machine->patternTable, verbose);
				//GET_NEXT_STATE_LT(s, input, length, idx, &next, machine, machine->patternTable, verbose)
			{
				uchar *table = &((s)[5 + 2 + 32]);
				uchar c = (uchar)input[*(idx)];
				int s_idx = c * 2;

				if (table[s_idx] != (uchar)(-1) || table[s_idx + 1] != (uchar)(-1)) { /* There is a goto */
					int i, count, ind;
					(&next)->next = ((machine)->states->table[(int)((table[(int)s_idx + 1] << 8) | (table[(int)s_idx]))]);
					(&next)->match = (((&((s)[5 + 2]))[(c) / 8] >> ((c) % 8)) & 0x01);
//					(&next)->stateID = (int)((table[(int)s_idx + 1] << 8) | (table[(int)s_idx]));


					if ((verbose) && (&next)->match) {
						count = 0;
						ind = 5 + 2;
						for (i = 0; i < 32; i++) {
							if (((s)[ind + i] & 1) != 0 && i * 8 < (int)c)
								count++;
							if (((s)[ind + i] & 2) != 0 && i * 8 + 1 < (int)c)
								count++;
							if (((s)[ind + i] & 4) != 0 && i * 8 + 2 < (int)c)
								count++;
							if (((s)[ind + i] & 8) != 0 && i * 8 + 3 < (int)c)
								count++;
							if (((s)[ind + i] & 16) != 0 && i * 8 + 4 < (int)c)
								count++;
							if (((s)[ind + i] & 32) != 0 && i * 8 + 5 < (int)c)
								count++;
							if (((s)[ind + i] & 64) != 0 && i * 8 + 6 < (int)c)
								count++;
							if (((s)[ind + i] & 128) != 0 && i * 8 + 7 < (int)c)
								count++;
							if (i * 8 + 8 >= (int)c)
								break;
						}
						(&next)->pattern = (machine->patternTable)->patterns[((s)[2] << 8) | (s)[3]][count];
					}
					(&next)->fail = 0;
					(*idx)++;
				} else { /* Failure */
					(&next)->next = ((machine)->states->table[(int)(((s)[5 + 1] << 8) | ((s)[5]))]);
					(&next)->match = 0;
					(&next)->fail = 1;
				}
			}
				break;
			case STATE_TYPE_PATH_COMPRESSED:
				GET_NEXT_STATE_PC(s, input, length, idx, &next, machine, machine->patternTable, verbose);
				break;
			case STATE_TYPE_LINEAR_ENCODED:
				getNextState_LE(s, input, length, idx, &next, machine, machine->patternTable, verbose );
				break;
			case STATE_TYPE_BITMAP_ENCODED:
				getNextState_BM(s, input, length, idx, &next, machine, machine->patternTable, verbose );
				break;
			}

			nextState = next.next;
		}

		if ((s == nextState) && (s == (stateTable[0])) && (next.fail)) {
			(*idx)++;
		}

		if (next.match) {
#ifdef PRINT_MATCHES
			if (verbose) {
				printf("Ignore Print: %s(%d)\n", next.pattern,*idx);
				fflush(stdout);
			}
#endif
			res = 1;
		}

#ifdef TRACE_STATE_MACHINE
		printf("Current state: %d, last char: %c, next char: %c, %s, next state: %d\n", getStateID(s), lastC, input[*idx], (next.match ? "matched" : "not matched"), getStateID(nextState));
#endif

		s = nextState;
	}

	return res;
}


int matchIterativeSimple(StateMachine *machine, const char *input, int length, int *idx, State *s, int verbose, MachineStats *stats) {
	State *nextState;
	int res, id;
	NextStateResult next;
	State **stateTable;
#ifdef TRACE_STATE_MACHINE
	char lastC;
	int id2;
#endif

	res = 0;
	stateTable = machine->states->table;

	while (*idx < length) {
#ifdef TRACE_STATE_MACHINE
		lastC = input[*idx];
#endif
		//getNextState(machine, s, input, length, idx, &next);
		//GET_NEXT_STATE(machine, s, input, length, idx, &next);

		getNextState_SLE(s, input, length, idx, &next, machine, machine->patternTable, verbose);

		nextState = next.next;

		if (verbose && ((((StateHeader*)nextState)->size) & 0x20)) {
			// Count pattern index
			id = (nextState[2] << 8) | nextState[3];
			if ((((StateHeader*)nextState)->size) & 0x10) {
				id += 0x10000;
			}
			next.pattern = machine->patternTable->patterns[id][0];
			next.match = TRUE;
		}


		if ((s == nextState) && (s == (stateTable[0])) && (next.fail)) {
			(*idx)++;
		}

		if (next.match) {
#ifdef PRINT_MATCHES
			if (verbose) {
				printf("%s\n", next.pattern);
			}
#endif
			res = 1;
		}
#ifdef TRACE_STATE_MACHINE
		id = (nextState[2] << 8) | nextState[3];
		id2 = (s[2] << 8) | s[3];
		printf("Current state: %d (%p), last char: %c (0x%X), next char: %c (0x%X), %s, next state: %d (%p)\n", id2, s, lastC, (unsigned int)lastC, input[*idx], (unsigned int)(input[*idx]), (next.match ? "matched" : "not matched"), id, nextState);
#endif

		s = nextState;
	}

	return res;
}

int match(StateMachine *machine,const char *input, int length, int verbose, MachineStats *stats)
{
	//matching should not be possible if data for matching data not provided.
	assert (machine->handlePatternData != NULL);
	int idx = 0;
	//return matchRecursive(machine, input, length, &idx, machine->states->table[0], verbose);
	if (machine->isSimple) {
		return matchIterativeSimple(machine, input, length, &idx, machine->states->table[0], verbose, stats);
	} else {
		return matchIterative(machine, input, length, &idx, machine->states->table[0], verbose, stats);
	//make sure next call won't reuse this buffer by default
	machine->handlePatternData = NULL;
	}
}

void compressStateTable(StateMachine *machine) {
	StateTable *oldTable, *newTable;
	int size, i;
	oldTable = machine->states;
	size = oldTable->nextID;
	newTable = createStateTable(size);
	for (i = 0; i < size; i++) {
		newTable->table[i] = oldTable->table[i];
	}
	newTable->nextID = i;
	machine->states = newTable;
	destroyStateTable(oldTable);
}
