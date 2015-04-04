/*
 * ACBuilder.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef ACBUILDER_H_
#define ACBUILDER_H_

#include "ACTypes.h"

void acBuildTree(ACTree *tree, const char *path, int avoidFailToLeaves, int mixIDs);
void acBuildTreeASCII(ACTree *tree, const char *path, int avoidFailToLeaves, int mixID);

typedef uint32_t (*getStringFuncType)(char *, uint32_t, void*);
void acBuildTreeFunc(ACTree *tree, getStringFuncType func , void* func_struct, int avoidFailToLeaves, int mixID);

void acDestroyTreeNodes(ACTree *tree);
Node *acGetNextNode(Node *node, char c);
void acPrintTree(ACTree *tree);

#endif /* ACBUILDER_H_ */
