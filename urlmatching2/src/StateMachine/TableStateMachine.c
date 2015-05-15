/*
 * TableStateMachine.c
 *
 *  Created on: Jan 23, 2011
 *      Author: yotamhc
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../Common/Flags.h"
#include "../Common/BitArray/BitArray.h"
#include "TableStateMachine.h"
#include "../CommonCTypes.h"
#include <sys/time.h>


#define MAX_PATTERN_LENGTH 1024

#define GET_TABLE_IDX(state, c) \
	(((state) * 256) + (unsigned char)(c))

#define GET_NEXT_STATE(table, state, c) \
	((table)[GET_TABLE_IDX(state, c)])

TableStateMachine *createTableStateMachine(unsigned int numStates) {
	TableStateMachine *machine;
	STATE_PTR_TYPE_WIDE *table;
	unsigned char *matches;
	char **patterns;
#ifdef USE_DEPTHMAP
	int *depthMap;
#endif

	machine = (TableStateMachine*)malloc(sizeof(TableStateMachine));
	table = (STATE_PTR_TYPE_WIDE*)malloc(sizeof(STATE_PTR_TYPE_WIDE) * numStates * 256);
	matches = (unsigned char*)malloc(sizeof(unsigned char) * (int)(ceil(numStates / 8.0)));
	patterns = (char**)malloc(sizeof(char*) * numStates);
#ifdef USE_DEPTHMAP
	depthMap = (int*)malloc(sizeof(int) * numStates);
#endif

	memset(table, 0, sizeof(STATE_PTR_TYPE_WIDE) * numStates * 256);
	memset(matches, 0, sizeof(unsigned char) * (int)(ceil(numStates / 8.0)));
	memset(patterns, 0, sizeof(char*) * numStates);

	machine->table = table;
	machine->numStates = numStates;
	machine->matches = matches;
	machine->patterns = patterns;
#ifdef USE_DEPTHMAP
	machine->depthMap = depthMap;
#endif
	return machine;
}

void destroyTableStateMachine(TableStateMachine *machine) {
	unsigned int i;

	for (i = 0; i < machine->numStates; i++) {
		if (machine->patterns[i]) {
			free(machine->patterns[i]);
		}
	}
	free(machine->patterns);
	free(machine->matches);
	free(machine->table);
#ifdef USE_DEPTHMAP
	free(machine->depthMap);
#endif
	free(machine);
}

void setGoto(TableStateMachine *machine, STATE_PTR_TYPE_WIDE currentState, char c, STATE_PTR_TYPE_WIDE nextState) {
	machine->table[GET_TABLE_IDX(currentState, c)] = nextState;
}

#define TO_HEX(val) \
	(((val) >= 10) ? ('A' + ((val) - 10)) : (char)('0' + (val)))

char *createPattern_TM(char *pattern, int len) {
	char buff[MAX_PATTERN_LENGTH];
	char *res;
	int i, j;

	for (i = 0, j = 0; i < len; i++) {
		if (pattern[i] >= 32 && pattern[i] < 127) {
			buff[j++] = pattern[i];
		} else {
			buff[j++] = '|';
			buff[j++] = TO_HEX((pattern[i] & 0x0F0) >> 4);
			buff[j++] = TO_HEX(pattern[i] & 0x00F);
			buff[j++] = '|';
		}
	}
	buff[j++] = '\0';
	res = (char*)malloc(sizeof(char) * j);
	strcpy(res, buff);
	return res;
}

void setMatch(TableStateMachine *machine, STATE_PTR_TYPE_WIDE state, char *pattern, int length) {
	char *cpy;
	SET_1BIT_ELEMENT(machine->matches, state, 1);
	cpy = createPattern_TM(pattern, length);
	/*
	cpy = (char*)malloc(sizeof(char) * (length + 1));
	strncpy(cpy, pattern, length);
	cpy[length] = '\0';
	*/
	machine->patterns[state] = cpy;
}

STATE_PTR_TYPE_WIDE getNextStateFromTable(TableStateMachine *machine, STATE_PTR_TYPE_WIDE currentState, char c) {
	return GET_NEXT_STATE(machine->table, currentState, c);
}

#ifdef DEPTHMAP_INFOCOM
static long _idx = 0;
#endif
//#define TRACE_STATE_MACHINE 0
int matchTableMachine(TableStateMachine *machine, const char *input, int length, int verbose,
		long *numAccesses, long *accessesByDepth, long *accessesByState, unsigned int *lastState
		, callBackWithPattern patternFunc, void* data) {
//	int matchTableMachine(TableStateMachine *machine, char *input, int length, int verbose) {
	STATE_PTR_TYPE_WIDE current, next;
	STATE_PTR_TYPE_WIDE *table;
	unsigned char *matches;
	char **patterns;
	int idx;
	int res;

#ifdef COUNT_BY_DEPTH
	int currentDepth;
#endif


#ifdef TRACE_STATE_MACHINE
	int val;
#endif

	res = 0;
	table = machine->table;
	matches = machine->matches;
	patterns = machine->patterns;
	idx = 0;
	current = 0;

	while (idx < length) {
#ifdef COUNT_BY_DEPTH
		(*numAccesses)++;
		currentDepth = machine->depthMap[current];
		accessesByDepth[currentDepth]++;
		accessesByState[current]++;
#endif

#ifdef DEPTHMAP_INFOCOM
		//if (_idx <= 1000000) {
			printf("%ld\t%d\n", _idx++, currentDepth);
		//}
#endif

		next = GET_NEXT_STATE(table, current, input[idx]);

#ifdef TRACE_STATE_MACHINE
		printf("Current state: %d, next char: ", current);
		val = (int)((input[idx]) & 0x0FF);
		if (val >= 32 && val < 127) {
			printf("%c", input[idx]);
		} else {
			printf("|%0X|", val);
		}
		printf(", %s, next state: %d\n", (GET_1BIT_ELEMENT(matches, next) ? "matched" : "not matched"), next);
#endif
		if (GET_1BIT_ELEMENT(matches, next)) {
			// It's a match!
			res = 1;
			patternFunc(patterns[next],idx,data);
#ifdef PRINT_MATCHES
			if (verbose) {
				printf("%s\n", patterns[next]);
#ifdef TRACE_STATE_MACHINE
//				getc(stdin);
#endif
			}
#endif
		}
		current = next;
		idx++;
	}
	if (lastState) {
		*lastState = current;
	}

	return res;
}

#define SCAN_SINGLE_CHAR(idx, trace, print_matches) \
	do {																			\
		next = GET_NEXT_STATE(table, current, input[(idx)]);						\
																					\
		if (trace) {																\
			int val;																\
			printf("Current state: %d, next char: ", current);						\
			val = (int)((input[(idx)]) & 0x0FF);									\
			if (val >= 32 && val < 127) {											\
				printf("%c", input[(idx)]);											\
			} else {																\
				printf("|%0X|", val);												\
			}																		\
			printf(", %s, next state: %d\n", (GET_1BIT_ELEMENT(matches, next) ? "matched" : "not matched"), next);	\
		}																			\
		if (GET_1BIT_ELEMENT(matches, next)) {										\
			/* It's a match! */														\
			res = 1;																\
			if (print_matches) {													\
				if (verbose) {														\
					printf("%s\n", patterns[next]);									\
					if (trace) {													\
						/* getc(stdin); */											\
					}																\
				}																	\
			}																		\
		}																			\
		current = next;																\
		(idx)++;																	\
	} while (0)

#define SCAN_SINGLE_CHAR_SHORT(idx, trace, print_matches) \
	do {																			\
		/* printf(">>onechar\n"); */ \
		next = GET_NEXT_STATE(table, current, input[(idx)]);						\
																					\
		if (GET_1BIT_ELEMENT(matches, next)) {										\
			/* It's a match! */														\
			res = 1;																\
			if (print_matches) {													\
				if (verbose) {														\
					printf("%s\n", patterns[next]);									\
					if (trace) {													\
						/* getc(stdin); */											\
					}																\
				}																	\
			}																		\
		}																			\
		current = next;																\
		(idx)++;																	\
	} while (0)


//static int temp = 0;

#ifdef TEST_BLOOM_PERF_ONLY
static long micros = 0;
#endif

#ifdef VAR_SIZE_DICT_PRINT_STAT
static int min_len = 999999, max_len = 0, len_sum = 0, len_count = 0;
#endif

#ifdef TEST_DICT_PERF
static long dict_micros = 0;
static long dict_bytes = 0;
#endif
/*
int matchDictionaryTableMachine(TableStateMachine *machine, char *input, int length, int verbose,
			Dictionary *dict, int dict_word_size, RollingHashTool *hasher, long *totalSkipped, long *totalLeftBorderChars, long *totalLeftBorders,
			long *dictGetTries, long *memcmpTries, long *memcmpFails) {
	STATE_PTR_TYPE_WIDE current, next;
	STATE_PTR_TYPE_WIDE *table;
	unsigned char *matches;
	char **patterns;
	int idx;
	int res;
	int bloom_not_found;
	int tmp1;
	unsigned long tmp2 = 0;
	unsigned char *bloom_bitmap;

#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
	int skipped, noskip;
#endif
#ifdef COUNT_MEMCMP_FAILURES
	int memcmp_tries, memcmp_fails, dict_get_tries;
#endif

	DictionaryEntry *entry;
	unsigned long hash;
	int potentialJumpLen;
	int potentialJumpState;
	int j;
#ifdef VAR_SIZE_DICT
	int dict_word_idx;
#endif
#ifdef TEST_DICT_PERF
	struct timeval start, end;
#endif
	//int found;
	//int fromJump;

	int trace = 0;
	int print_matches = 0;

	res = 0;
	table = machine->table;
	matches = machine->matches;
	patterns = machine->patterns;
	idx = 0;
	current = 0;
	//fromJump = 0;

	//HASH_WORD(hasher, input, dict_word_size, hash);
	HASH_WORD_NOROLL_1(hasher, input, dict_word_size, hash);
#if (BLOOM_FUNCS > 1)
	HASH_WORD_NOROLL_2(hasher, input, dict_word_size, hash, tmp2);
#endif

#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
	skipped = 0;
#endif
#ifdef TRACE_STATE_MACHINE
	trace = 1;
#endif
#ifdef PRINT_MATCHES
	print_matches = 1;
#endif
#ifdef COUNT_MEMCMP_FAILURES
	memcmp_tries = 0;
	memcmp_fails = 0;
	dict_get_tries = 0;
#endif

#ifdef TEST_BLOOM_PERF_ONLY
	int pos, pos2, val = 0;
	struct timeval start, end;

	gettimeofday(&start, NULL);
#endif

#ifdef DICT_USES_BLOOM
	bloom_bitmap = dict->bloom->a;
#endif

	while (idx < length) {

#ifdef TEST_BLOOM_PERF_ONLY

		//FAST_BLOOM_CHECK2(dict->bloom, hasher, &(input[idx]), 16, hash, val, tmp2);

		pos = hash % BLOOM_SIZE;
#if (BLOOM_FUNCS > 1)
		pos2 = tmp2 % BLOOM_SIZE;
		val = (GETBIT(bloom_bitmap, pos) && GETBIT(bloom_bitmap, pos2));
#else
		val = (GETBIT(bloom_bitmap, pos));
#endif


		//HASH_WORD_NOROLL_16(hasher, &(input[idx]), dict_word_size, val);

		idx++; skipped+=val;
		HASH_WORD_NOROLL_1(hasher, ((unsigned long long *)(&(input[idx]))), dict_word_size, hash);
#if (BLOOM_FUNCS > 1)
		HASH_WORD_NOROLL_2(hasher, ((unsigned long long *)(&(input[idx]))), dict_word_size, hash, tmp2);
#endif


#else
		if (idx + dict_word_size < length) {
#ifdef TEST_DICT_PERF
			gettimeofday(&start, NULL);
#endif
			DICTIONARY_GET(dict, bloom_bitmap, hasher, &(input[idx]), hash, entry, bloom_not_found, tmp1, tmp2);
#ifdef TEST_DICT_PERF
			gettimeofday(&end, NULL);
			dict_micros += (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
			dict_bytes++;
#endif
#ifdef COUNT_MEMCMP_FAILURES
			dict_get_tries++;
			if (!bloom_not_found) {
				memcmp_tries++;
			}
#endif
			if (entry) {
				// Word is in dictionary!
#ifdef VAR_SIZE_DICT
				dict_word_idx = 0;
				if (entry->num_words > 1) {
					while (dict_word_idx < entry->num_words &&
							   entry->words[dict_word_idx] != NULL &&
							   memcmp(&(input[idx + dict_word_size]),
									  entry->words[dict_word_idx],
									  entry->word_lengths[dict_word_idx]
									  ) != 0) {
						dict_word_idx++;
					}
#ifdef VAR_SIZE_DICT_PRINT_STAT
					min_len = (min_len < dict_word_idx + 1 ? min_len : dict_word_idx + 1);
					max_len = (max_len > dict_word_idx + 1 ? max_len : dict_word_idx + 1);
					len_sum += dict_word_idx + 1;
#endif

					if (entry->num_words == dict_word_idx) {
						fprintf(stderr, "Fatal error: Invalid dictionary entry.\n");
						exit(1);
					}
				}
#ifdef VAR_SIZE_DICT_PRINT_STAT
				else { min_len = 1; if (max_len == 0) max_len = 1; len_sum++; }
				len_count++;
#endif

				potentialJumpState = entry->jump_states[dict_word_idx];
				potentialJumpLen = entry->word_lengths[dict_word_idx] + dict_word_size;
#else
				potentialJumpState = entry->jump_state;
				potentialJumpLen = dict_word_size;
#endif

	#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
				noskip = 0;
	#endif
				for (j = idx; j < idx + potentialJumpLen; ) {
					if (machine->depthMap[current] <= (j - idx)) {
						break;
					}
	#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
					noskip++;
	#endif
					SCAN_SINGLE_CHAR_SHORT(j, trace, print_matches);
				}
				if (j < idx + potentialJumpLen) {
					// Jumping using the dictionary
					current = potentialJumpState;
					idx += potentialJumpLen;
					//fromJump = 1;
				} else {
					// Not jumping anywhere eventually
					idx = j;
					//fromJump = 0;
				}
	#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
				skipped += potentialJumpLen;
				(*totalLeftBorderChars) += noskip;
				(*totalLeftBorders)++;
	#endif
				// Update hash
				//HASH_WORD(hasher, &(input[idx]), dict_word_size, hash);
				HASH_WORD_NOROLL_1(hasher, ((unsigned long long *)(&(input[idx]))), dict_word_size, hash);
#if (BLOOM_FUNCS > 1)
				HASH_WORD_NOROLL_2(hasher, ((unsigned long long *)(&(input[idx]))), dict_word_size, hash, tmp2);
#endif
				continue;
			}
#ifdef COUNT_MEMCMP_FAILURES
			else {
				if (!bloom_not_found) {
					memcmp_fails++;
					//printf("\n");
					//printf(", hash=%lu, dict_idx=%lu\n", hash, hash % dict->size);
				}
			}
#endif
		}
		// Not in dictionary (or not enough characters left and ending is not in dictionary)
		SCAN_SINGLE_CHAR_SHORT(idx, trace, print_matches);
		//fromJump = 0;
		if (idx + dict_word_size < length) {
			//REHASH_WORD(hasher, hash, input[(idx + dict_word_size - 1)], input[(idx - 1)], dict_word_size, hash);
			HASH_WORD_NOROLL_1(hasher, ((unsigned long long*)(&(input[idx]))), dict_word_size, hash);
#if (BLOOM_FUNCS > 1)
			HASH_WORD_NOROLL_2(hasher, &(input[idx]), dict_word_size, hash, tmp2);
#endif
		}
#endif
	}

#ifdef TEST_BLOOM_PERF_ONLY
	gettimeofday(&end, NULL);
	micros += (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
#endif

#ifdef COUNT_DICTIONARY_SKIPPED_BYTES
	if (totalSkipped) {
		*totalSkipped += skipped;
	}
#endif
#ifdef COUNT_MEMCMP_FAILURES
	if (memcmpFails) {
		*memcmpFails += memcmp_fails;
	}
	if (memcmpTries) {
		*memcmpTries += memcmp_tries;
	}
	if (dictGetTries) {
		*dictGetTries += dict_get_tries;
	}
#endif
	tmp2++;
	return res;
}
*/
void printMicros() {
	//printf(">> Total time for BLOOM TEST: %ld microseconds\n", micros);
#ifdef VAR_SIZE_DICT_PRINT_STAT
	printf("********************************************\n");
	printf("* Variable Size Dict. Runtime Statistics:  *\n");
	printf("* ======================================== *\n");
	printf("* Number of entries matched: %13d *\n", len_count);
	printf("* Number of words matched: %15d *\n", len_sum);
	printf("* Average number of words/entry: %9.2f *\n", ((double)len_sum)/len_count);
	printf("* Minimal number of words/entry: %9d *\n", min_len);
	printf("* Maximal number of words/entry: %9d *\n", max_len);
	printf("********************************************\n");
#endif
#ifdef TEST_DICT_PERF
	printf("Total time spent on dictionary: %ld us (%2.3f ns per time)\n", dict_micros, (dict_micros * 1000.0) / dict_bytes);
#endif
}
