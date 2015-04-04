/*
 * TableStateMachineGenerator.h
 *
 *  Created on: Jan 25, 2011
 *      Author: yotamhc
 */

#ifndef TABLESTATEMACHINEGENERATOR_H_
#define TABLESTATEMACHINEGENERATOR_H_

#include "TableStateMachine.h"
#include "../AhoCorasick/ACBuilder.h"

TableStateMachine *generateTableStateMachine(const char *path, int verbose);
TableStateMachine *generateTableStateMachineFunc(getStringFuncType func, void* func_struct, int verbose);

#endif /* TABLESTATEMACHINEGENERATOR_H_ */
