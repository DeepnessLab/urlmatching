/*
 * ACWrapperCompressed.cpp
 *
 *  Created on: 7 בדצמ 2014
 *      Author: Daniel
 */


/*
 * AhoCrosarikClassic.cpp
 *
 *  Created on: 14 בנוב 2014
 *      Author: Daniel
 */

#include "../PatternMatching/ACWrapperCompressed.h"
#include <string.h>
#include <assert.h>
#include "../UrlDictionay.h"
#include "MatchingFunctions.h"

extern "C" {
#include "../Common/Types.h"
}


#define DBG(__str__) std::cout<<__str__


//uint32_t getStringFromMap(char* out_buff, uint32_t max_size) {
//	ACWrapperClassic ac = ACWrapperClassic::getInstance();
//	std::string* str= ac.nextPattern();
//	if (str==NULL)
//		return 0;
//	const char* chars= str->c_str();
//	strncpy(out_buff,chars,max_size);
//	return strlen(chars);
//}
//
//uint32_t getStringFromList(char* out_buff, uint32_t max_size, void* list_struct) {
//	stringlistType* list_describtor = (stringlistType*) list_struct;
//	if (list_describtor->index == list_describtor->size)
//		return 0;
//	const char* chars= list_describtor->list[list_describtor->index]->c_str();
//	list_describtor->index = list_describtor->index+1;
//	strncpy(out_buff,chars,max_size);
//	return strlen(chars);
//}

int special_handle_pattern(char* str,uint32_t idx, void* data) {
	urlMatchingType* url_module = (urlMatchingType*) data;
	assert(url_module!=NULL);

	//TODO: remove the following
	std::string real_str(str);
	std::cout<<"at idx="<<idx<<", pattern="<<real_str<<std::endl;

	//update module with the pattern



	return 1;
}



ACWrapperCompressed::ACWrapperCompressed() {
//	resetNextPattern();
	_patternsList=NULL;

}

ACWrapperCompressed::~ACWrapperCompressed() {
	if (_machine != NULL) {
		destroyStateMachine(_machine);
	}
}


bool ACWrapperCompressed::load_patterns(std::string filepath) {

	const char* tmp = filepath.c_str();
	_machine = createStateMachine(tmp,100,100,0);
	return true;

}

bool ACWrapperCompressed::load_patterns(Symbol2PatternType patternsList, uint32_t size) {
	//make a copy for Symbol2PatternType list
	_patternsList = patternsList;
	//convert Symbol2PatternType to StringListType
	StringListType list = new std::string*[size];
	for (symbolT i=0; i< size; i++) {
		list[i]=&(patternsList[i]->_str);
		const char* c = patternsList[i]->_str.c_str();
		if (strlen ( patternsList[i]->_str.c_str() ) == 1) {//this is a one char symbol
			_char_to_symbol[*c] = i;
		}
	}

	StringListDBWithIdxType db={list,0,size};
//	_machine = generateTableStateMachineFunc(getStringFromList,&db,0);
	_machine = createStateMachineFunc(getStringFromList,&db,1000,1000,0);
	//TODO: delete all lists attached strings
	delete list;
	return true;

}

bool ACWrapperCompressed::find_patterns(std::string input_str, symbolT* result) {

	//prepare metadata
	urlMatchingType module;
	initModule(module);

	const char* str = input_str.c_str();
	char* str2= new char[strlen(input_str.c_str())+10];

	//TODO: insert the following into input string to init
	module.input_string = (char *) str;
	uint32_t symbol_idx =  str[0];
	symbolT first_place_symbol = _char_to_symbol[symbol_idx];
	module.matching_symbols_arr[module.symbol_i] = first_place_symbol;
	module.symbol_i++;

	strcpy(str2,str);
//	matchTableMachine(_machine, str, strlen(str),
//			TRUE, NULL, NULL, NULL, NULL 	//all others are statistics - use NULL
//			,handle_pattern, &module);	//use patterncollector
	MachineStats stats;
	match(_machine,/*(char *)*/ str2, strlen(str), 1, &stats);
	std::cout<<std::endl;
	return true;
	finalize_result(module,result);

	delete str2;
	return true;
}
