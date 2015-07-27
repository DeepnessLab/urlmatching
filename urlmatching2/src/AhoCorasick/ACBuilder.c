/*
 * ACBuilder.c
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "../Common/Flags.h"
#include "ACBuilder.h"
#include "NodeQueue.h"
#include "../Common/Types.h"

#define READ_BUFFER_SIZE 1024
#define MAX_STATES 65536
#define MAX_STATES_ACTIVE
#define STATES_HARD_LIMIT (2*1048576) //524288

#ifdef MIX_IDS

#define F 149
//#define F 57331

#define GET_NEW_ID(x, size) \
	(((int)ceil((double)(size) / F)) * ((x) % F) + (((x) - ((x) % F)) / F))

void mixIDs(Node *node, int size) {
	Pair *pair;
	int newID;

	newID = GET_NEW_ID(node->id, size);
	if (newID >= size || newID < 0) {
		printf("Cannot mix IDs. Choose a different factor.\n");
		exit(1);
	}
	node->id = newID;

	hashmap_iterator_reset(node->gotos);
	while ((pair = hashmap_iterator_next(node->gotos)) != NULL) {
		mixIDs(pair->ptr, size);
	}
}

#endif

Node *createNewNode(ACTree *tree, Node *parent) {
	Node *node = (Node*)malloc(sizeof(Node));
	node->id = tree->size++;
	node->message = NULL;
	node->messageLength = -1;
	node->gotos = hashmap_create();
	if (node->gotos == NULL) {
		fprintf(stderr, "ERROR: Cannot create new hashmap\n");
		exit(1);
	}
	node->failure = NULL;
	node->numGotos = 0;
	node->match = 0;
	node->hasFailInto = 0;
	if (parent != NULL) {
		node->depth = parent->depth + 1;
	} else {
		node->depth = 0;
	}
	node->isFirstLevelNode = 0;
	node->isSecondLevelNode = 0;
	node->c1 = 0;
	node->c2 = 0;
	node->marked = 0;
	return node;
}

Pair *createNewPair(char c, Node *node) {
	Pair *p = (Pair*)malloc(sizeof(Pair));
	p->c = c;
	p->ptr = node;
	return p;
}

Node *acGetNextNode(Node *node, char c) {
	Pair *pair;
	pair = (Pair*)(hashmap_get(node->gotos, (int)c));
	if (pair == NULL)
		return NULL;
	return pair->ptr;
}

int enter_simulate_addional_states(ACTree *tree, char *pattern, int len) {
	Node *state = tree->root;
	int j = 0;
	Node *next;

	while ((next = acGetNextNode(state, pattern[j])) != NULL && j < len) {
		state = next;
		j++;
	}
	return (len - j);
}


void enter(ACTree *tree, char *pattern, int len) {
	Node *state = tree->root;
	int j = 0, p;
	Node *next, *newState;

	while ((next = acGetNextNode(state, pattern[j])) != NULL && j < len) {
		state = next;
		j++;
	}

	for (p = j; p < len; p++) {
		newState = createNewNode(tree, state);
		Pair *pair = createNewPair(pattern[p], newState);
		hashmap_put(state->gotos, pattern[p], pair);
		state->numGotos++;
		state = newState;
	}

	// Match
	state->match = 1;
	state->message = (char*)malloc(sizeof(char) * (len + 1));
	strncpy(state->message, pattern, len);
	state->message[len] = '\0';
	state->messageLength = len;
}

void constructFailures(ACTree *tree) {
	NodeQueue q;
	Node *root, *state, *r, *s;
	Pair *pair;
	HashMap *map;
	char a;
	int toL0, toL1, toL2;

	toL0 = toL1 = toL2 = 0;

	nodequeue_init(&q);

	root = tree->root;
	root->failure = root;

	map = root->gotos;
	hashmap_iterator_reset(map);
	while ((pair = hashmap_iterator_next(map)) != NULL) {
		nodequeue_enqueue(&q, pair->ptr);
		pair->ptr->failure = root;
		toL0++;
	}

	while (!nodequeue_isempty(&q)) {
		r = nodequeue_dequeue(&q);
		map = r->gotos;
		hashmap_iterator_reset(map);
		while ((pair = hashmap_iterator_next(map)) != NULL) {
			a = pair->c;
			s = pair->ptr;
			nodequeue_enqueue(&q, s);
			if (r->failure == NULL)
				r->failure = root;
			state = r->failure;
			while (state->id != 0 && acGetNextNode(state, a) == NULL) {
				state = state->failure;
			}
			state = acGetNextNode(state, a);
			if (state == NULL) {
				state = root;
			}
			state->hasFailInto++;
			s->failure = state;

			if (state->depth == 1)
				toL1++;
			else if (state->depth == 2)
				toL2++;
			else if (state == root)
				toL0++;

		}
	}

	//printf("To L0: %d, To L1: %d, to L2: %d\n", toL0, toL1, toL2);

	nodequeue_destroy_elements(&q, 1);
}

void findFails(Node *node, int *l0, int *l1, int *l2) {
	Pair *pair;

	if (node->failure->depth == 1)
		(*l1)++;
	else if (node->failure->depth == 2)
		(*l2)++;
	else if (node->failure->depth == 0)
		(*l0)++;

	hashmap_iterator_reset(node->gotos);
	while ((pair = hashmap_iterator_next(node->gotos))) {
		findFails(pair->ptr, l0, l1, l2);
	}
}

void getL2nums(ACTree *tree) {
	Node *root = tree->root;
	Pair *pair;
	int l0, l1, l2;

	l1 = root->numGotos;
	l2 = 0;

	hashmap_iterator_reset(root->gotos);
	while ((pair = hashmap_iterator_next(root->gotos))) {
		l2 += pair->ptr->numGotos;
	}

	//printf("Level 1 #: %d\n", l1);
	//printf("Level 2 #: %d\n", l2);

	l0 = l1 = l2 = 0;

	findFails(root, &l0, &l1, &l2);

	//printf("To L0: %d, To L1: %d, to L2: %d\n", l0, l1, l2);


}

char *clone_string(char *s, int len) {
	//int len;
	char *res;

	if (s == NULL) {
		return NULL;
	}

	//len = strlen(s);
	res = (char*)malloc(sizeof(char) * (len + 1));

	memcpy(res, s, len);
	res[len] = '\0';

	return res;
}

char *concat_strings(char *s1, char *s2, int len1, int len2, int freeS1, int freeS2, int *newLen) {
	//int len1, len2;
	char *res;

	if (s1 == NULL) {
		*newLen = len2;
		return clone_string(s2, len2);
	}

	if (s2 == NULL) {
		*newLen = len1;
		return clone_string(s1, len1);
	}
/*
	if (strcmp(s1, s2) == 0) {
		printf("Strings are equal: %s ;; %s\n", s1, s2);
	}
*/
	//len1 = strlen(s1);
	//len2 = strlen(s2);
	res = (char*)malloc(sizeof(char) * (len1 + len2 + 2));
	memcpy(res, s1, len1);
	res[len1] = ACDELIMITER;
	memcpy(&(res[len1+1]), s2, len2);
	res[len1+len2+1] = '\0';

	if (freeS1)
		free(s1);
	if (freeS2)
		free(s2);

	*newLen = len1 + len2 + 1;
	return res;
}

void avoidFailToAcceptingStatesRecursive(Node *state) {
	Pair *pair;
	int newLen;

	while (state->failure->match) {
		state->match = 1;
		//state->message = (char*)malloc(sizeof(char));
		state->message = concat_strings(state->message, state->failure->message, state->messageLength, state->failure->messageLength, 1, 0, &newLen);
		state->messageLength = newLen;
		if (state->failure->numGotos == 0) {
			if (state->failure->hasFailInto > 0) {
				state->failure->hasFailInto--;
			}
			state->failure = state->failure->failure;
		} else {
			break;
		}
	}

	hashmap_iterator_reset(state->gotos);
	while ((pair = hashmap_iterator_next(state->gotos)) != NULL) {
		avoidFailToAcceptingStatesRecursive(pair->ptr);
	}
}

void avoidFailToAcceptingStates(ACTree *tree) {
	avoidFailToAcceptingStatesRecursive(tree->root);
}

void printPair(void *data) {
	Pair *pair;
	pair = (Pair*)data;
	if (pair->c >= 32 && pair->c < 127) {
		printf("%c:%d", pair->c, pair->ptr->id);
	} else {
		printf("%0X:%d", (int)((pair->c) & 0x0FF), pair->ptr->id);
	}
}

void printNode(Node *node) {
	int i, val;
	printf("Node ID: %d, Depth: %d, Gotos: ", node->id, node->depth);
	hashmap_print(node->gotos, &printPair);
	printf(", Failure: %d, Match: %d", node->failure->id, node->match);
	if (node->match) {
		printf(" (message: ");//, node->message);
		for (i = 0; i < node->messageLength; i++) {
			if (node->message[i] >= 32 && node->message[i] < 127) {
				printf("%c", node->message[i]);
			}else {
				val = (int)((node->message[i]) & 0x0FF);
				printf("|");
				if (val < 16) {
					printf("0");
				}
				printf("%X|", val);
			}
		}
		printf(" [length: %d])", node->messageLength);
	}
	if (node->hasFailInto) {
		printf(" *Has %d fail(s) into it*", node->hasFailInto);
	}
	printf("\n");
}

void acPrintTree(ACTree *tree) {
	NodeQueue queue;
	Node *node;
	Pair *pair;
	int i;

	nodequeue_init(&queue);

	printf("Printing Aho-Corasick tree:\n");

	nodequeue_enqueue(&queue, tree->root);

	while (!nodequeue_isempty(&queue)) {
		node = nodequeue_dequeue(&queue);
		for (i = 0; i < node->depth; i++) {
			printf("-");
		}
		printNode(node);

		hashmap_iterator_reset(node->gotos);
		while ((pair = hashmap_iterator_next(node->gotos)) != NULL) {
			nodequeue_enqueue(&queue, pair->ptr);
		}
	}

	nodequeue_destroy_elements(&queue, 1);
}
#ifdef PRINT_OUR_WC_PATTERNS
int getChar(char c, char *s, int idx) {
	unsigned char low, high;

	if (c >= 32 && c < 127) {
		s[idx] = c;
		s[idx + 1] = '\0';
		return 1;
	} else {
		low = c & 0x0F;
		high = ((c & 0xF0) >> 4);
		s[idx] = '|';
		s[idx + 1] = (high < 10) ? ('0' + high) : ('A' + high - 10);
		s[idx + 2] = (low < 10) ? ('0' + low) : ('A' + low - 10);
		s[idx + 3] = '|';
		s[idx + 4] = '\0';
		return 4;
	}
}

void findNodeLongestFail(Node *node, char *last, int idx) {
	int len, slen;
	Node *fail;
	Pair *pair;
	char *pattern;
	char EMPTY[1] = { '\0' };

	len = 0;
	slen = 0;
	if (node->id != 0) {
		fail = node->failure;
		len = 1;
		while (fail->id != 0) {
			fail = fail->failure;
			len++;
		}

		char s[5];
		slen = getChar(node->c1, s, 0);

		//last[node->depth] = '\0';
		strncpy(&(last[idx]), s, slen);
		last[idx + slen] = '\0';

		if (node->depth == len) {
			pattern = last;
			printf("%s\n", last);
		} else {
			pattern = EMPTY;
		}

		//printf("%d\t%s\t%d\t%d\t%s\n", node->id, s, node->depth, len, pattern);
	}
	hashmap_iterator_reset(node->gotos);
	while ((pair = hashmap_iterator_next(node->gotos)) != NULL) {
		pair->ptr->c1 = pair->c;
		findNodeLongestFail(pair->ptr, last, idx + slen);
	}
}
#endif

#ifdef PRINT_LENGTH_HIST
void getLengthHistogram(Node *node, int histogram[], int *maxlen) {
	Pair *pair;

	if (node->match) {
		if (node->depth >= 2048) {
			fprintf(stderr, "Histogram is too small\n");
			exit(1);
		}
		histogram[node->depth]++;
		if (*maxlen < node->depth) {
			*maxlen = node->depth;
		}
	}

	hashmap_iterator_reset(node->gotos);
	while ((pair = hashmap_iterator_next(node->gotos)) != NULL) {
		getLengthHistogram(pair->ptr, histogram, maxlen);
	}
}

void printLengthHistogram(ACTree *tree) {
	int i;
	int maxlen;
	int histogram[2048];
	maxlen = 0;

	memset(histogram, 0, sizeof(int) * 2048);

	getLengthHistogram(tree->root, histogram, &maxlen);

	for (i = 1; i <= maxlen; i++) {
		printf("Length: %d\tCount: %d\n", i, histogram[i]);
	}
}
#endif

#ifdef FIND_FAIL_HISTOGRAM

void findFailPathLengthHistogram(Node *node, int *histogram, int maxLen, int *nodesWithFailInto) {
	int count;
	Node *fail;
	Pair *pair;

	if (node->id != 0 && node->hasFailInto == 0) {
		// No other node fails to this one
		// Can take whole path from here
		count = 1;
		fail = node->failure;
		while (fail != NULL && fail->id != 0) {
			fail = fail->failure;
			count++;
		}
		if (count >= maxLen) {
			fprintf(stderr, "Path is too long");
			exit(1);
		}
		histogram[count]++;
	} else {
		(*nodesWithFailInto)++;
	}

	hashmap_iterator_reset(node->gotos);
	while ((pair = hashmap_iterator_next(node->gotos)) != NULL) {
		findFailPathLengthHistogram(pair->ptr, histogram, maxLen, nodesWithFailInto);
	}
}

#define MAX_HIST_SIZE 1024

void printFailPathLengthHistogram(ACTree *tree) {
	int i, nodesWithFailIntos;
	int hist[MAX_HIST_SIZE];

	nodesWithFailIntos = 0;
	memset(hist, 0, sizeof(int) * MAX_HIST_SIZE);
	findFailPathLengthHistogram(tree->root, hist, MAX_HIST_SIZE, &nodesWithFailIntos);

	for (i = 0; i < MAX_HIST_SIZE; i++) {
		printf("%d\t%d\n", i, hist[i]);
	}

	printf("Nodes with fail into them: %d\n", nodesWithFailIntos);
}

#endif

//#define MAX_PATTERNS 16512
#define MAX_PATTERNS -1

void acBuildTree(ACTree *tree, const char *path, int avoidFailToLeaves, int mixID) {
	FILE *f;
	char buff[READ_BUFFER_SIZE];
	unsigned char size[2];
	int len, length, count, count2;
#ifdef PRINT_CHAR_COUNT
	long charcount;
#endif

#ifdef MAX_STATES_ACTIVE
	fprintf(stderr, "Warning: Maximal AC states limit is active and is set to %d\n", MAX_STATES);
#else
	fprintf(stderr, "Warning: Maximal AC states limit is off. Compressed automaton may suffer from state ID overflow!\n");
#endif

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "Cannot read file %s\n", path);
		exit(1);
	}

	tree->size = 0;
	count = 0;
	count2 = 0;
#ifdef PRINT_CHAR_COUNT
	charcount = 0;
#endif
	tree->root = createNewNode(tree, NULL);

	while (!feof(f) && (MAX_PATTERNS <= 0 || count < MAX_PATTERNS)) {
		len = fread(size, sizeof(unsigned char), 2, f);
		if (len == 0)
			break;

		if (len != 2) {
			fprintf(stderr, "Cannot read size of pattern from file %s\n", path);
			exit(1);
		}
		length = (size[0] << 8) | size[1];

		len = fread(buff, sizeof(char), length, f);
		if (len != length) {
			fprintf(stderr, "Cannot read pattern from file %s\n", path);
			exit(1);
		}

		buff[length] = '\0';

		if (length == 0) {
			fprintf(stderr, "Found zero length pattern in file %s\n", path);
			exit(1);
		}
		if (tree->size + length <= STATES_HARD_LIMIT) {
#ifdef MAX_STATES_ACTIVE
			if (tree->size + length <= MAX_STATES) {
#endif
				enter(tree, buff, length);
				count2++;
#ifdef MAX_STATES_ACTIVE
			}
#endif
		}
		count++;
#ifdef PRINT_CHAR_COUNT
		charcount += length;
#endif
	}

	printf("Total patterns: %d\n", count);
	printf("Total patterns used: %d\n", count2);
/*
	while (fgets(buff, READ_BUFFER_SIZE, f)) {
		len = strlen(buff);
		if (len > 0 && (buff[len - 1] == '\r' || buff[len - 1] == '\n')) {
			buff[len - 1] = '\0';
		}
		if (len > 1 && (buff[len - 2] == '\r' || buff[len - 2] == '\n')) {
			buff[len - 2] = '\0';
		}

		enter(tree, buff);
	}
*/
	fclose(f);
#ifdef PRINT_CHAR_COUNT
	printf("Total chars: %ld\n", charcount);
#endif

	constructFailures(tree);

	getL2nums(tree);

	if (avoidFailToLeaves) {
		avoidFailToAcceptingStates(tree);
	}
#ifdef PRINT_OUR_WC_PATTERNS
	char temp[2048];
	printf("Node ID\tChar\tDepth\tFail chain len\n");
	findNodeLongestFail(tree->root, temp, 0);
#endif

#ifdef PRINT_LENGTH_HIST
	printLengthHistogram(tree);
#endif

#ifdef FIND_FAIL_HISTOGRAM
	printFailPathLengthHistogram(tree);
#endif

#ifdef MIX_IDS
	if (mixID) {
		mixIDs(tree->root, tree->size);
	}
#endif

#ifdef PRINT_AHO_CORASICK
	acPrintTree(tree);
#endif
	printf("Total states: %d\n", tree->size);

}

void acBuildTreeASCII(ACTree *tree, const char *path, int avoidFailToLeaves, int mixID) {
	FILE *f;
	char buff[READ_BUFFER_SIZE];
	int length, count;
#ifdef PRINT_CHAR_COUNT
	long charcount;
#endif

	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Cannot read file %s\n", path);
		exit(1);
	}

	tree->size = 0;
	count = 0;
#ifdef PRINT_CHAR_COUNT
	charcount = 0;
#endif
	tree->root = createNewNode(tree, NULL);

	while (!feof(f) && (MAX_PATTERNS <= 0 || count < MAX_PATTERNS)) {
		if (fgets(buff, READ_BUFFER_SIZE, f)) {
			length = strlen(buff) - 1;
			buff[length] = '\0';
			if (length == 0) {
				fprintf(stderr, "Found zero length pattern in file %s\n", path);
				exit(1);
			}
			//if (tree->size + length <= MAX_STATES) {
			enter(tree, buff, length);
			//}
			count++;
	#ifdef PRINT_CHAR_COUNT
			charcount += length;
	#endif
		}
	}
/*
	while (fgets(buff, READ_BUFFER_SIZE, f)) {
		len = strlen(buff);
		if (len > 0 && (buff[len - 1] == '\r' || buff[len - 1] == '\n')) {
			buff[len - 1] = '\0';
		}
		if (len > 1 && (buff[len - 2] == '\r' || buff[len - 2] == '\n')) {
			buff[len - 2] = '\0';
		}

		enter(tree, buff);
	}
*/
	fclose(f);
#ifdef PRINT_CHAR_COUNT
	printf("Total chars: %ld\n", charcount);
#endif

	constructFailures(tree);

	getL2nums(tree);

	if (avoidFailToLeaves) {
		avoidFailToAcceptingStates(tree);
	}
#ifdef PRINT_OUR_WC_PATTERNS
	char temp[2048];
	printf("Node ID\tChar\tDepth\tFail chain len\n");
	findNodeLongestFail(tree->root, temp, 0);
#endif

#ifdef PRINT_LENGTH_HIST
	printLengthHistogram(tree);
#endif

#ifdef FIND_FAIL_HISTOGRAM
	printFailPathLengthHistogram(tree);
#endif

#ifdef MIX_IDS
	if (mixID) {
		mixIDs(tree->root, tree->size);
	}
#endif

#ifdef PRINT_AHO_CORASICK
	acPrintTree(tree);
#endif

}


void acBuildTreeFunc(ACTree *tree,  getStringFuncType func , void* func_struct, int avoidFailToLeaves, int mixID) {

	char buff[READ_BUFFER_SIZE];
	int length, count;
#ifdef PRINT_CHAR_COUNT
	long charcount;
#endif

	tree->size = 0;
	count = 0;
#ifdef PRINT_CHAR_COUNT
	charcount = 0;
#endif
	tree->root = createNewNode(tree, NULL);

	while ( func(buff,READ_BUFFER_SIZE,func_struct,tree)!=0 && (MAX_PATTERNS <= 0 || count < MAX_PATTERNS)) {
//		printf("Loaded %s\n",buff);
		length = strlen(buff)/* - 1*/;
		if (length == 0) {
			fprintf(stderr, "Found zero length pattern in input\n");
			exit(1);
		}
		buff[length] = '\0';
		//if (tree->size + length <= MAX_STATES) {
		enter(tree, buff, length);
		//}
		count++;
#ifdef PRINT_CHAR_COUNT
		charcount += length;
#endif
	}


/*
	while (fgets(buff, READ_BUFFER_SIZE, f)) {
		len = strlen(buff);
		if (len > 0 && (buff[len - 1] == '\r' || buff[len - 1] == '\n')) {
			buff[len - 1] = '\0';
		}
		if (len > 1 && (buff[len - 2] == '\r' || buff[len - 2] == '\n')) {
			buff[len - 2] = '\0';
		}

		enter(tree, buff);
	}
*/
#ifdef PRINT_CHAR_COUNT
	printf("Total chars: %ld\n", charcount);
#endif

	constructFailures(tree);

	getL2nums(tree);

	if (avoidFailToLeaves) {
		avoidFailToAcceptingStates(tree);
	}
#ifdef PRINT_OUR_WC_PATTERNS
	char temp[2048];
	printf("Node ID\tChar\tDepth\tFail chain len\n");
	findNodeLongestFail(tree->root, temp, 0);
#endif

#ifdef PRINT_LENGTH_HIST
	printLengthHistogram(tree);
#endif

#ifdef FIND_FAIL_HISTOGRAM
	printFailPathLengthHistogram(tree);
#endif

#ifdef MIX_IDS
	if (mixID) {
		mixIDs(tree->root, tree->size);
	}
#endif

#ifdef PRINT_AHO_CORASICK
	acPrintTree(tree);
#endif

}

void acDestroyNodesRecursive(Node *node) {
	Pair *pair;
	Node *child;
	HashMap *map = node->gotos;
	hashmap_iterator_reset(map);

	while ((pair = (Pair*)hashmap_iterator_next(map)) != NULL) {
		child = pair->ptr;
		acDestroyNodesRecursive(child);
		free(pair);
	}
	if (node->message) {
		free(node->message);
	}
	hashmap_destroy(map);
	free(node);
}

void acDestroyTreeNodes(ACTree *tree) {
	acDestroyNodesRecursive(tree->root);
}
