/*
 * ACWrapperCompressed.cpp
 *
 *  Created on: 7 ���� 2014
 *      Author: Daniel
 */


/*
 * AhoCrosarikClassic.cpp
 *
 *  Created on: 14 ���� 2014
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
	make_pattern_to_symbol_list();
	return true;

}

bool ACWrapperCompressed::load_patterns(Symbol2pPatternArr patternsList, uint32_t size) {
	//make a copy for Symbol2PatternType list
	_patternsList = patternsList;
	//convert Symbol2PatternType to StringListType
	StringListType list = new std::string*[size];
	uint32_t idx=0;				//store how many string we entered the list

	for (symbolT i=1; i< size; i++) {	// 0 is reserved pattern as NULL
		//go over all patterns, if pattern is single char - set it into _char_to_symbol array
		// if pattern is longer than 1 char - add it to patterns list so AC will load them
		if (strlen ( patternsList[i]->_str.c_str() ) == 1) {
			const uint32_t c = (uint32_t) patternsList[i]->_str.at(0);
			_char_to_symbol[c] = i;
		} else {
			list[idx]=&(patternsList[i]->_str);
			idx++;
		}
	}
	StringListDBWithIdxType db={list,0,idx};
	_machine = createStateMachineFunc(getStringFromList,&db,1000,1000,0);

	delete list;

//	make_pattern_to_symbol_list();
	return true;


}


/* use this function to build a complimentary patterns table for symbols*/
void ACWrapperCompressed::make_pattern_to_symbol_list() {
	char ***patterns;
	patterns=_machine->patternTable->patterns;

	uint32_t size =_machine->patternTable->size;
	symbolT** patterns_as_symbols = new symbolT*[size];

	symbolT symbol_counter = 1; //0 is reserved
	std::cout<<"Print cached patterns table";
	for (uint32_t i = 0; i < size; i++) {
		std::cout<<std::endl;
		std::cout<<"i="<<i<<":";
		if (patterns[i] == NULL)
			patterns_as_symbols[i]=NULL;
			continue;
		uint32_t j = 0;
		//count patterns
		while (patterns[i][j] != NULL) {
			j++; //[11]
		}
		patterns_as_symbols[i] = new symbolT[j];
		j--;
		patterns_as_symbols[i][j]=S_NULL;
		//assign symbols
		for (uint32_t new_j=0;new_j<j;j++) {
			std::cout<<"(j="<<new_j<<",s="<<symbol_counter<<")"<<patterns[i][new_j];
			patterns_as_symbols[i][new_j]=symbol_counter;
			symbol_counter++;
		}
	}
	std::cout<<std::endl;
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
	match(_machine,/*(char *)*/ str2, strlen(str), 1, &stats , handle_pattern, &module);
	std::cout<<std::endl;
	return true;
	finalize_result(module,result);

	delete str2;
	return true;
}
