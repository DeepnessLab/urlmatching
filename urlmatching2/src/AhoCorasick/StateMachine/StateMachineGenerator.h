/*
 * StateMachineGenerator.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef STATEMACHINEGENERATOR_H_
#define STATEMACHINEGENERATOR_H_

#include "StateMachine.h"

StateMachine *createStateMachine(const char *path, int maxGotosLE, int maxGotosBM, int verbose);
StateMachine *createStateMachineFunc(getStringFuncType func, void* func_struct, int max_patterns_to_load, int maxGotosLE, int maxGotosBM, int verbose);
StateMachine *createSimpleStateMachine(const char *path, int maxGotosLE, int maxGotosBM, int verbose);
StateMachine *createSimpleStateMachineFunc(getStringFuncType func, void* func_struct, int maxGotosLE, int maxGotosBM, int verbose);

void destroyStateMachine(StateMachine *machine);

#endif /* STATEMACHINEGENERATOR_H_ */
