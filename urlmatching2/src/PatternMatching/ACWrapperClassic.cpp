/*
 * AhoCrosarikClassic.cpp
 *
 *  Created on: 14 бреб 2014
 *      Author: Daniel
 */

#include "../PatternMatching/ACWrapperClassic.h"
#include <string.h>
#include <assert.h>
#include "../UrlToolkit/UrlDictionay.h"
#include "MatchingFunctions.h"
extern "C" {
#include "../Common/Types.h"
}


//#define DBG(__str__) std::cout<<__str__
/*

//uint32_t getStringFromMap(char* out_buff, uint32_t max_size) {
//	ACWrapperClassic ac = ACWrapperClassic::getInstance();
//	std::string* str= ac.nextPattern();
//	if (str==NULL)
//		return 0;
//	const char* chars= str->c_str();
//	strncpy(out_buff,chars,max_size);
//	return strlen(chars);
//}

//uint32_t getStringFromList(char* out_buff, uint32_t max_size, void* list_struct) {
//	stringlistType* list_describtor = (stringlistType*) list_struct;
//	if (list_describtor->index == list_describtor->size)
//		return 0;
//	const char* chars= list_describtor->list[list_describtor->index]->c_str();
//	list_describtor->index = list_describtor->index+1;
//	strncpy(out_buff,chars,max_size);
//	return strlen(chars);
//}


ACWrapperClassic::ACWrapperClassic() {
//	resetNextPattern();
	_patternsList=NULL;

}

ACWrapperClassic::~ACWrapperClassic() {
	if (_machine != NULL) {
		destroyTableStateMachine(_machine);
	}
}


bool ACWrapperClassic::load_patterns(std::string filepath) {

	const char* tmp = filepath.c_str();
	_machine = generateTableStateMachine(tmp,0);
	return true;

}

bool ACWrapperClassic::load_patterns(Symbol2pPatternArr patternsList, uint32_t size) {
	//make a copy for Symbol2PatternType list
	_patternsList = patternsList;
	//convert Symbol2PatternType to StringListType
	StringListType list = new std::string*[size];
	for (symbolT i=0; i< size; i++)
		list[i]=&(patternsList[i]->_str);

	StringListDBWithIdxType db={list,0,size};
	_machine = generateTableStateMachineFunc(getStringFromList,&db,0);
	delete list;
	return true;

}

bool ACWrapperClassic::find_patterns(std::string input_str, symbolT* result) {

	//prepare metadata
	urlMatchingType module;
	initModule(module);

	const char* str = input_str.c_str();
	matchTableMachine(_machine, str, strlen(str),
			TRUE, NULL, NULL, NULL, NULL 	//all others are statistics - use NULL
			,handle_pattern, &module);	//use patterncollector

	return true;
	finalize_result(module,result);

	return true;
}
*/
