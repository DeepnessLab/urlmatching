/*
 * BitmapEncodedState.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "../Common/BitArray/BitArray.h"
#include "../Common/FastLookup/FastLookup.h"
#include "BitmapEncodedState.h"

#define FAIL_IDX 4

/*
State *createState_BM(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure) {
	int numPtrsToFirstOrSecondLevel = 0;
	int i, j, isptr, memsize;
	int current_idx_flags, current_idx_gotos;
	unsigned int flag;
	uchar *data, *bitmap;
	StateHeader header;

	header.type = STATE_TYPE_BITMAP_ENCODED;
	header.size = size;

	for (i = size; i != 0; i--) {
		if (isFirstLevelState(gotos[i]) || isSecondLevelState(gotos[i]))
			numPtrsToFirstOrSecondLevel++;
	}

	memsize = 3 + 1 + 32 +						// header + sizeInBytes + bitmap
			PTR_SIZE * (size - numPtrsToFirstOrSecondLevel) +	// real ptrs
			(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, size:6, id:16, sizeInBytes, fail:16, bitmap(32bytes), flag0, ..., flagN-1, ptr_i, ..., ptr_j ]
	// size is part of the header but is not really necessary
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)(GET_HEADER_AS_BYTE1(header));
	data[2] = (uchar)(GET_HEADER_AS_BYTE2(header));
	data[3] = (uchar)memsize;
	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	bitmap = &(data[FAIL_IDX + 2]);
	memset(bitmap, 0, 32);

	current_idx_flags = FAIL_IDX + 2 + 32;
	current_idx_gotos = FAIL_IDX + 2 + 32 + (int)ceil((3 * size) / 8.0);
	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		flag = PTR_TYPE_REGULAR;

		// Set bitmap value
		SET_1BIT_ELEMENT(bitmap, (uchar)chars[i], 1);

		// Set two bits for pointer type
		isptr = FALSE;
		if (isFirstLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL1;
		} else if (isSecondLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL2;
		} else {
			isptr = TRUE;
		}

		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			data[current_idx_gotos + j * 2] = (uchar)gotos[i];
			data[current_idx_gotos + j * 2 + 1] = (uchar)(gotos[i] >> 8);
			j++;
		}
	}

	return (State*)data;
}
*/
State *createEmptyState_BM(int id, int size, int numPtrsToFirstOrSecondLevel) {
	int memsize;
	uchar *data;
	StateHeader header;

	header.type = STATE_TYPE_BITMAP_ENCODED;
	header.size = size;

	memsize = 3 + 1 + 32 +						// header + sizeInBytes + bitmap
			PTR_SIZE * (size - numPtrsToFirstOrSecondLevel) +	// real ptrs
			(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, idMSB:8, idLSB:8, size:6, sizeInBytes, fail:16, bitmap(32bytes), flag0, ..., flagN-1, ptr_i, ..., ptr_j ]
	// size is part of the header but is not really necessary
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)(id >> 8);
	data[2] = (uchar)id;
	data[3] = (uchar)memsize;

	return (State*)data;
}

void setStateData_BM(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure) {
	int i, j, isptr, size, memsize;
	int current_idx_flags, current_idx_gotos;
	unsigned int flag;
	uchar *data, *bitmap;
	StateHeader *header;

	data = (uchar*)state;
	header = (StateHeader*)state;
	size = header->size;
	memsize = data[3];

	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	bitmap = &(data[FAIL_IDX + 2]);
	memset(bitmap, 0, 32);

	current_idx_flags = FAIL_IDX + 2 + 32;
	current_idx_gotos = FAIL_IDX + 2 + 32 + (int)ceil((3 * size) / 8.0);
	memset(&(data[current_idx_flags]), 0, (int)ceil((3 * size) / 8.0));
	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		flag = PTR_TYPE_REGULAR;

		// Set bitmap value
		SET_1BIT_ELEMENT(bitmap, (uchar)chars[i], 1);

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
		isptr = (flag == PTR_TYPE_REGULAR);

		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			if (current_idx_gotos + j * 2 + 1 >= memsize) {
				fprintf(stderr, "[BM] Memory index out of bound: %d, mem size: %d\n", (current_idx_gotos + j * 2 + 1), memsize);
				exit(1);
			}
			data[current_idx_gotos + j * 2] = (uchar)gotos[i];
			data[current_idx_gotos + j * 2 + 1] = (uchar)(gotos[i] >> 8);
			j++;
		}
	}
	//printState_BM(state);
}

#ifdef POPCNT
#define POPCNT_INT(x, count) \
    __asm__("popcnt %1,%0" : "=r" (count) : "r" (x));
#else
#define POPCNT_INT(x, count) \
    count = __builtin_popcount(x);
#endif

/*
#define POPCOUNT(bitmap, bitIdx, sum) 									\
	do {																\
		unsigned int j, jump, tmp;										\
		unsigned char byteIdx;											\
		sum = 0;														\
		byteIdx = (bitIdx) / 8;											\
		jump = sizeof(long);											\
		for (j = 0; j + jump <= byteIdx; j += jump) {					\
			sum += __builtin_popcountl(*((long*)(&((bitmap)[j]))));		\
		}																\
		jump = sizeof(int);												\
		for (; j + jump <= byteIdx; j += jump) {						\
			sum += __builtin_popcount(*((int*)(&((bitmap)[j]))));		\
		}																\
		tmp = j * 8;													\
		for (; tmp < (bitIdx); j+=1) {									\
			if ((tmp < (bitIdx)) && (bitmap)[j] & 1) sum++;				\
			if (((tmp + 1) < (bitIdx)) && (bitmap)[j] & 2) sum++;		\
			if (((tmp + 2) < (bitIdx)) && (bitmap)[j] & 4) sum++;		\
			if (((tmp + 3) < (bitIdx)) && (bitmap)[j] & 8) sum++;		\
			if (((tmp + 4) < (bitIdx)) && (bitmap)[j] & 16) sum++;		\
			if (((tmp + 5) < (bitIdx)) && (bitmap)[j] & 32) sum++;		\
			if (((tmp + 6) < (bitIdx)) && (bitmap)[j] & 64) sum++;		\
			if (((tmp + 7) < (bitIdx)) && (bitmap)[j] & 128) sum++;		\
			tmp += 8;													\
		}																\
	} while (0);
 */

#define POPCOUNT(bitmap, bitIdx, sum) 									\
	do {																\
		unsigned int j, jump, tmp;										\
		int count;														\
		unsigned char byteIdx;											\
		sum = 0;														\
		byteIdx = (bitIdx) / 8;											\
		jump = sizeof(int);												\
		for (j = 0; j + jump <= byteIdx; j += jump) {						\
			POPCNT_INT(*((int*)(&((bitmap)[j]))), count);				\
			sum += count;												\
		}																\
		tmp = j * 8;													\
		for (; tmp < (bitIdx); j+=1) {									\
			if ((tmp < (bitIdx)) && (bitmap)[j] & 1) sum++;				\
			if (((tmp + 1) < (bitIdx)) && (bitmap)[j] & 2) sum++;		\
			if (((tmp + 2) < (bitIdx)) && (bitmap)[j] & 4) sum++;		\
			if (((tmp + 3) < (bitIdx)) && (bitmap)[j] & 8) sum++;		\
			if (((tmp + 4) < (bitIdx)) && (bitmap)[j] & 16) sum++;		\
			if (((tmp + 5) < (bitIdx)) && (bitmap)[j] & 32) sum++;		\
			if (((tmp + 6) < (bitIdx)) && (bitmap)[j] & 64) sum++;		\
			if (((tmp + 7) < (bitIdx)) && (bitmap)[j] & 128) sum++;		\
			tmp += 8;													\
		}																\
	} while (0);

int getNextState_BM(State *bitmapEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	uchar c;
	StateHeader *header;
	unsigned int i, size, found, j, count, id;
	STATE_PTR_TYPE fail;
	uchar *bitmap;
	unsigned int value, value2;

	header = (StateHeader*)bitmapEncodedState;
	size = header->size;

	c = (uchar)str[*idx];
	bitmap = (uchar*)(&(bitmapEncodedState[FAIL_IDX + 2]));

	found = GET_1BIT_ELEMENT(bitmap, c);

	if (found) {
		result->fail = FALSE;

		// Count bits to char
		//i = 0;
		/*
		tmp = j * 8;
		for (j = 0; tmp <= c; j++) {
			if ((tmp < c) && bitmap[j] & 1) i++;
			if (((tmp + 1) < c) && bitmap[j] & 2) i++;
			if (((tmp + 2) < c) && bitmap[j] & 4) i++;
			if (((tmp + 3) < c) && bitmap[j] & 8) i++;
			if (((tmp + 4) < c) && bitmap[j] & 16) i++;
			if (((tmp + 5) < c) && bitmap[j] & 32) i++;
			if (((tmp + 6) < c) && bitmap[j] & 64) i++;
			if (((tmp + 7) < c) && bitmap[j] & 128) i++;
			tmp += 8;
		}
		*/
		POPCOUNT(bitmap, c, i);

		// Find the pointer

		// find the i-th three-bits sequence
		value = GET_3BITS_ELEMENT(&bitmapEncodedState[FAIL_IDX + 2 + 32], i);

		result->match = GET_MATCH_BIT(value);
		if (verbose && result->match) {
			// Search ptr in ptrs array
			count = 0;
			for (j = 0; j < i; j++) {
				value2 = GET_3BITS_ELEMENT(&bitmapEncodedState[FAIL_IDX + 2 + 32], j);
				if (GET_MATCH_BIT(value2))
					count++;
			}
			id = (bitmapEncodedState[1] << 8) | bitmapEncodedState[2];
			result->pattern = patternTable->patterns[id][count];
		}

		count = 0;
		switch (GET_PTR_TYPE(value)) {
		case PTR_TYPE_REGULAR:
			// Search ptr is ptrs array
			for (j = 0; j < i; j++) {
				value = GET_3BITS_ELEMENT(&bitmapEncodedState[FAIL_IDX + 2 + 32], j);
				if (GET_PTR_TYPE(value) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = FAIL_IDX + 2 + 32 + (int)ceil((3 * size) / 8.0) + count * 2;
			result->next = GET_STATE_POINTER_FROM_ID(machine, (bitmapEncodedState[j + 1] << 8) | (bitmapEncodedState[j]));
			break;
		case PTR_TYPE_LEVEL1:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_FIRST_LEVEL_STATE(machine, c));
			break;
		case PTR_TYPE_LEVEL2:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_SECOND_LEVEL_STATE(machine, (uchar)str[*idx - 1], c));
			break;
		default:
			fprintf(stderr, "Unknown pointer type in a bitmap encoded state: %d\n", GET_PTR_TYPE(value));
			exit(1);
		}
		(*idx)++;
	} else {
		fail = (bitmapEncodedState[FAIL_IDX + 1] << 8) | (bitmapEncodedState[FAIL_IDX]);
		result->next = GET_STATE_POINTER_FROM_ID(machine, fail);
		result->match = FALSE;
		result->fail = TRUE;
	}
	return found;
}

void printState_BM(State *state) {
	StateHeader *header = (StateHeader*)state;
	int size = header->size;
	int id = (state[1] << 8) | state[2];
	int sizeInBytes = state[3];
	int fail = (state[FAIL_IDX + 1] << 8) | state[FAIL_IDX];
	int i;

	printf("Bitmap encoded state\n");
	printf("ID: %d\n", id);
	printf("Size in bytes: %d\n", sizeInBytes);
	printf("Failure state: %d\n", fail);

	char *chars = (char*)malloc(sizeof(char) * size);
	int *matches = (int*)malloc(sizeof(int) * size);
	int *gotos = (int*)malloc(sizeof(int) * size);
	uchar *bitmap = &(state[FAIL_IDX + 2]);

	uchar flag;
	int count, j;

	j = 0;
	for (i = 0; i < 32 && j < size; i++) {
		if (bitmap[i] & 1) chars[j++] = (char)(i * 8);
		if (bitmap[i] & 2) chars[j++] = (char)(i * 8 + 1);
		if (bitmap[i] & 4) chars[j++] = (char)(i * 8 + 2);
		if (bitmap[i] & 8) chars[j++] = (char)(i * 8 + 3);
		if (bitmap[i] & 16) chars[j++] = (char)(i * 8 + 4);
		if (bitmap[i] & 32) chars[j++] = (char)(i * 8 + 5);
		if (bitmap[i] & 64) chars[j++] = (char)(i * 8 + 6);
		if (bitmap[i] & 128) chars[j++] = (char)(i * 8 + 7);
	}

	for (i = 0; i < size; i++) {
		//chars[i] = state[FAIL_IDX + 2 + i];
		flag = GET_3BITS_ELEMENT(&state[FAIL_IDX + 2 + 32], i);
		matches[i] = GET_MATCH_BIT(flag);

		count = 0;
		switch (GET_PTR_TYPE(flag)) {
		case PTR_TYPE_REGULAR:
			// Search ptr is ptrs array
			for (j = 0; j < i; j++) {
				flag = GET_3BITS_ELEMENT(&state[FAIL_IDX + 2 + 32], j);
				if (GET_PTR_TYPE(flag) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = FAIL_IDX + 2 + 32 + (int)ceil((3 * size) / 8.0) + count * 2;
			gotos[i] = (state[j + 1] << 8) | (state[j]);
			break;
		case PTR_TYPE_LEVEL1:
			gotos[i] = -1; //getFirstLevelState(c);
			break;
		case PTR_TYPE_LEVEL2:
			gotos[i] = -2; //getSecondLevelState(chars[*idx - 1], c);
			break;
		}
	}

	for (i = 0; i < size; i++) {
		printf("Char: %c, match: %d, goto: %d\n", chars[i], matches[i], gotos[i]);
	}

	free(gotos);
	free(matches);
	free(chars);
}

STATE_PTR_TYPE getStateID_BM(State *state) {
	return (state[1] << 8) | state[2];
}

int getSizeInBytes_BM(State *state) {
	return state[3];
}

State *getNextStatePointer_BM(State *bitmapEncodedState) {
	return &(bitmapEncodedState[bitmapEncodedState[3]]);
}

