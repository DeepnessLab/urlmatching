/*
 * PathCompressedState.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "../Common/FastLookup/FastLookup.h"
#include "../Common/BitArray/BitArray.h"
#include "PathCompressedState.h"

// #define GOTO_IDX 6
/*
State *createState_PC(int size, char *chars, STATE_PTR_TYPE *failures, int *matches, STATE_PTR_TYPE next) {
	int numPtrsToFirstOrSecondLevel = 0;
	int i, j, isptr, memsize;
	int current_idx_flags, current_idx_failures;
	unsigned int flag;
	uchar *data;
	uchar sizeLSB;
	StateHeader header;

	header.type = STATE_TYPE_PATH_COMPRESSED;
	header.size = (size >> 8);
	sizeLSB = (uchar)size;

	for (i = 0; i < size; i++) {
		if (isFirstLevelState(failures[i]) || isSecondLevelState(failures[i]))
			numPtrsToFirstOrSecondLevel++;
	}

	memsize = 5 + 1 + 1 + size * sizeof(char) +						// header + sizeLSB + sizeInBytes + chars
			PTR_SIZE * (size - numPtrsToFirstOrSecondLevel) +	// real ptrs
			(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			PTR_SIZE;											// goto ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, sizeMSB:6, sizeLSB:8, idMSB:8, idLSB:8, sizeInBytesOfThisStruct, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = sizeLSB;
	data[2] = (uchar)()
	data[3] = (uchar)
	data[4] = (uchar)memsize;
	data[GOTO_IDX] = (uchar)next;
	data[GOTO_IDX + 1] = (uchar)(next >> 8);

	current_idx_flags = GOTO_IDX + 2 + size;
	current_idx_failures = GOTO_IDX + 2 + size + (int)ceil((3 * size) / 8.0);
	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		// Set character
		data[GOTO_IDX + 2 + i] = chars[i];

		flag = PTR_TYPE_REGULAR;

		// Set two bits for pointer type
		isptr = FALSE;
		if (isFirstLevelState(failures[i])) {
			flag = PTR_TYPE_LEVEL1;
		} else if (isSecondLevelState(failures[i])) {
			flag = PTR_TYPE_LEVEL2;
		} else {
			isptr = TRUE;
		}

		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			data[current_idx_failures + j * 2] = (uchar)failures[i];
			data[current_idx_failures + j * 2 + 1] = (uchar)(failures[i] >> 8);
			j++;
		}
	}

	return (State*)data;
}
*/

State *createEmptyState_PC(int id, int size, int numFailPtrsToFirstOrSecondLevel) {
	int memsize;
	uchar *data;
	uchar sizeLSB;
	StateHeader header;

	header.type = STATE_TYPE_PATH_COMPRESSED;
	header.size = (size >> 8);
	sizeLSB = (uchar)size;

	memsize = 3 + 1 + 2 + size * sizeof(char) +						// header + sizeLSB + sizeInBytes + chars
			PTR_SIZE * (size - numFailPtrsToFirstOrSecondLevel) +	// real ptrs
			//(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			(int)ceil(size / 8.0) +								// match bits
			(int)ceil(size / 4.0) +								// two bits flags
			PTR_SIZE;											// goto ptr

	data = (unsigned char*)malloc(memsize);

	// OLD:
	// [ type:2, sizeMSB:6, sizeLSB:8, idMSB:8, idLSB:8, sizeInBytesOfThisStruct:16, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// NEW:
	// [ type:2, sizeMSB:6, sizeLSB:8, idMSB:8, idLSB:8, sizeInBytesOfThisStruct:16, fail:16, char0, ..., charN-1, match0, ..., matchN-1, flag0, ..., flagN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = sizeLSB;
	data[2] = (uchar)(id >> 8);
	data[3] = (uchar)id;
	data[4] = (uchar)(memsize >> 8);
	data[5] = (uchar)memsize;

	return (State*)data;
}

void setStateData_PC(State *state, uchar *chars, STATE_PTR_TYPE *failures, short *ptrTypes, int *matches, STATE_PTR_TYPE next) {
	int i, j, isptr, size, memsize;
	int current_idx_matches, current_idx_flags, current_idx_failures;
	unsigned int flag;
	uchar *data;
	StateHeader *header;

	data = (uchar*)state;
	header = (StateHeader*)data;
	size = (header->size << 8) | data[1];

	memsize = (int)((data[4] << 8) | data[5]);

	data[GOTO_IDX] = (uchar)next;
	data[GOTO_IDX + 1] = (uchar)(next >> 8);

	current_idx_matches = GOTO_IDX + 2 + size;
	current_idx_flags = current_idx_matches + (int)ceil(size / 8.0);
	current_idx_failures = current_idx_flags + (int)ceil(size / 4.0);
	memset(&(data[current_idx_matches]), 0, (int)ceil(size / 8.0) + (int)ceil(size / 4.0));
	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		// Set character
		data[GOTO_IDX + 2 + i] = chars[i];

		flag = PTR_TYPE_REGULAR;

		// Set two bits for pointer type
		isptr = FALSE;
		/*
		if (isFirstLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL1;
		} else if (isSecondLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL2;
		} else {
			isptr = TRUE;
		}
		*/
		flag = ptrTypes[i];
		if (failures[i] == 0)
			flag = PTR_TYPE_LEVEL0;
		isptr = (flag == PTR_TYPE_REGULAR);

		/*
		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);
		 */

		// Set match bit
		SET_1BIT_ELEMENT(&data[current_idx_matches], i, matches[i]);

		// Set flag 2 bits
		SET_2BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			if (current_idx_failures + j * 2 + 1 >= memsize) {
				fprintf(stderr, "[PC] Memory index out of bound: %d, mem size: %d\n", (current_idx_failures + j * 2 + 1), memsize);
				exit(1);
			}
			data[current_idx_failures + j * 2] = (uchar)failures[i];
			data[current_idx_failures + j * 2 + 1] = (uchar)(failures[i] >> 8);
			j++;
		}
	}
}

char *concat_strings_nofree(char *s1, char *s2) {
	int len1, len2;
	char *res;

	if (s1 == NULL)
		return s2;

	if (s2 == NULL)
		return s1;

	len1 = strlen(s1);
	len2 = strlen(s2);
	res = (char*)malloc(sizeof(char) * (len1 + len2 + 2));
	strcpy(res, s1);
	res[len1] = ';';
	strcpy(&(res[len1+1]), s2);

	return res;
}

#ifdef COUNT_CALLS
static int counter = 0;
#endif

int getNextState_PC(State *pathCompressedState, char *str, int slen, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	StateHeader *header;
	int i, j, count, length, failed, matched, id;
	STATE_PTR_TYPE next;
	//uchar *failures;
	uchar *chars;
	char *pattern;
	//unsigned int value;
	uchar *matches, *flags;

#ifdef COUNT_CALLS
	counter++;
#endif

	pattern = NULL;
	header = (StateHeader*)pathCompressedState;
	length = (header->size << 8) | (pathCompressedState[1] & 0x0FF);
	chars = (uchar*)(&(pathCompressedState[GOTO_IDX + 2]));
	//failures = (&(pathCompressedState[GOTO_IDX + 2 + length + (int)ceil((3 * length) / 8.0)]));
	matches = &(pathCompressedState[GOTO_IDX + 2 + length]);
	flags = &(pathCompressedState[GOTO_IDX + 2 + length + (int)ceil(length / 8.0)]);

	failed = FALSE;
	matched = FALSE;
	for (i = 0; i < length; i++) {
		if (*idx >= slen) {
			break;
		}
		if ((uchar)str[*idx] != chars[i]) {
			failed = TRUE;
			break;
		} else {
			//value = GET_3BITS_ELEMENT(flags, i);
			if (GET_1BIT_ELEMENT(matches, i)) {
			//if (GET_MATCH_BIT(value)) {
				// It is a match
				matched = TRUE;

				if (verbose) {
					// Count pattern index
					count = 0;
					for (j = 0; j < i; j++) {
						if (GET_1BIT_ELEMENT(matches, i))
							count++;
					}
					id = (pathCompressedState[2] << 8) | pathCompressedState[3];
					pattern = concat_strings_nofree(pattern, patternTable->patterns[id][count]);
				}
			}
			(*idx)++;
		}
	}

	if (failed) {
		//value = GET_3BITS_ELEMENT(flags, i);
		switch (GET_2BITS_ELEMENT(flags, i)) {
		case PTR_TYPE_REGULAR:
			// Search ptr is ptrs array
			count = 0;
			for (j = 0; j < i; j++) {
				if (GET_2BITS_ELEMENT(flags, j) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = GOTO_IDX + 2 + length + (int)ceil(length / 8.0) + (int)ceil(length / 4.0) + count * 2;
			result->next = GET_STATE_POINTER_FROM_ID(machine, (pathCompressedState[j + 1] << 8) | (pathCompressedState[j]));
			break;
		case PTR_TYPE_LEVEL0:
			result->next = machine->states->table[0];
			break;
		case PTR_TYPE_LEVEL1:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_FIRST_LEVEL_STATE(machine, str[*idx - 1]));
			break;
		case PTR_TYPE_LEVEL2:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_SECOND_LEVEL_STATE(machine, str[*idx - 2], str[*idx - 1]));
			break;
		default:
			//fprintf(stderr, "Unknown pointer type in a path compressed state: %d\n", GET_PTR_TYPE(value));
			exit(1);
		}
		result->fail = TRUE;
		result->match = matched;
		result->pattern = pattern;
		return FALSE;
	}

	next = (pathCompressedState[GOTO_IDX + 1] << 8) | (pathCompressedState[GOTO_IDX]);
	result->next = GET_STATE_POINTER_FROM_ID(machine, next);
	result->match = matched;
	result->fail = FALSE;
	result->pattern = pattern;
	return TRUE;
}

void printState_PC(State *state, int depth) {
	StateHeader *header = (StateHeader*)state;
	int length = (header->size << 8) | (state[1] & 0x0FF);
	//int sizeInBytes = (int)((state[4] << 8) | state[5]);
	int next = (state[GOTO_IDX + 1] << 8) | state[GOTO_IDX];
	int i, id;
	id = (state[2] << 8) | state[3];

	char buff[2048];
	int nextPos;
	sprintf(buff, "PC\t%6d\t%6d\t%6d\t%6d\t", id, depth, next, -999);
	nextPos = strlen(buff);

	/*
	printf("Path compressed state\n");
	printf("ID: %d\n", id);
	printf("Size in bytes: %d\n", sizeInBytes);
	printf("Goto state: %d\n", next);
	*/

	char *chars = (char*)malloc(sizeof(char) * length);
	int *matches = (int*)malloc(sizeof(int) * length);
	int *failures = (int*)malloc(sizeof(int) * length);

	uchar flag;
	int count, j;

	for (i = 0; i < length; i++) {
		chars[i] = state[GOTO_IDX + 2 + i];
		flag = GET_3BITS_ELEMENT(&state[GOTO_IDX + 2 + length], i);
		matches[i] = GET_MATCH_BIT(flag);

		count = 0;
		switch (GET_PTR_TYPE(flag)) {
		case PTR_TYPE_REGULAR:
			// Search ptr is ptrs array
			for (j = 0; j < i; j++) {
				flag = GET_3BITS_ELEMENT(&state[GOTO_IDX + 2 + length], j);
				if (GET_PTR_TYPE(flag) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = GOTO_IDX + 2 + length + (int)ceil((3 * length) / 8.0) + count * 2;
			failures[i] = (state[j + 1] << 8) | (state[j]);
			break;
		case PTR_TYPE_LEVEL1:
			failures[i] = -1; //getFirstLevelState(c);
			break;
		case PTR_TYPE_LEVEL2:
			failures[i] = -2; //getSecondLevelState(chars[*idx - 1], c);
			break;
		}
	}

	for (i = 0; i < length; i++) {
		//printf("Char: %c, match: %d, failure: %d\n", chars[i], matches[i], failures[i]);
		sprintf(&(buff[nextPos]), "|%c (%d, %6d)", chars[i], matches[i], failures[i]);
		nextPos += 14;
	}
	printf("%s\n", buff);

	free(failures);
	free(matches);
	free(chars);
}

int getAllNextStates_PC(State *state, int *next) {
	next[0] = (state[GOTO_IDX + 1] << 8) | state[GOTO_IDX];
	return 1;
}

STATE_PTR_TYPE getStateID_PC(State *state) {
	return (state[2] << 8) | state[3];
}

int getSizeInBytes_PC(State *state) {
	return (int)((state[4] << 8) | state[5]);
}

State *getNextStatePointer_PC(State *pathCompressedState) {
	return &(pathCompressedState[(int)((pathCompressedState[4] << 8) | pathCompressedState[5])]);
}

#ifdef COUNT_CALLS
void printCounter_PC() {
	printf("getNextState_PC count: %d\n", counter);
}
#endif
