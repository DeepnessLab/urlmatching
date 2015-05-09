/*
 * MatchingFunctions.cpp

 *
 *  Created on: 8 בדצמ 2014
 *      Author: Daniel
 */

#include "MatchingFunctions.h"
#include <stdio.h>
#include <iostream>

inline
void update_for_all_patterns(const char* delimiter, char* string_with_patterns,
		uint32_t idx_of_last_char, urlMatchingType& url_module) {
	//assuming here the patterns are in decreasing length order (the longest is the first)
	std::cout << "go_over_all_patterns for \"" << string_with_patterns <<"\""<< std::endl;
	char* s = new char[strlen(string_with_patterns)];
	strcpy(s, string_with_patterns);
	char* tk = strtok(s, delimiter);
	while (tk != NULL) {
		patternsMapType::iterator ret = url_module.patternDB->find(tk);
		assert (ret != url_module.patternDB->end());
		symbolT s = ret->second;
		updateModule(url_module,s,idx_of_last_char);
		std::cout << " > Added \"" << tk <<"\" at "<< idx_of_last_char << std::endl;
		tk = strtok(NULL, delimiter);
	}
}

int handle_pattern(char* str, uint32_t idx, void* data) {
	if (str == NULL) {
		DBG("** handled str=NULL at "<<DVAL(idx)<<" **");
		return 0;
	}
	urlMatchingType* url_module = (urlMatchingType*) data;
	std::cout<<"handle_pattern: \""<<str<< "\" at "<<idx<<std::endl;

	//insert all literals
	for (uint32_t j = url_module->index+1 ; j <=idx ; j++) {
		uint32_t char_at_j = (uint32_t) url_module->input_string[j];
		symbolT s =  url_module->char_to_symbol[char_at_j];
		updateModule(*url_module,s,j);
		std::cout<<" > Added \""<<(char) char_at_j<<"\" at "<<j<<std::endl;
	}

	assert(url_module!=NULL);
	update_for_all_patterns(";", str, idx,*url_module);
	std::string real_str(str);

	return 1;

}

/** used to load all patterns into ac builder
 *
 * @param out_buff
 * @param max_size
 * @param list_struct
 * @return
 */
uint32_t getStringFromList(char* out_buff, uint32_t max_size,
		void* list_struct) {
	StringListDBWithIdxType* list_describtor =
			(StringListDBWithIdxType*) list_struct;
	if (list_describtor->index == list_describtor->size) {
		std::cout << std::endl << "Total of " << list_describtor->size
				<< " patterns loaded" << std::endl;
		return 0;
	}
	const char* chars = list_describtor->list[list_describtor->index]->c_str();
	list_describtor->index = list_describtor->index + 1;
	strncpy(out_buff, chars, max_size);
	std::cout << "adding \"" << chars << "\"; ";
	return strlen(chars);
}

void printModule (urlMatchingType& urlmatching) {
	const char* prefix = "' ";
	std::cout<<prefix<<"Index of P and V ="<<urlmatching.index+1<<STDENDL;
	std::cout<<prefix<<"P=[";
	for (uint32_t i=0; i<=urlmatching.index+1 ; i++) {
		std::cout<<urlmatching.P[i]<< ",";
	}
	std::cout<<"]"<<std::endl;

	std::cout<<prefix<<"V=[";
	for (uint32_t i=0; i<=urlmatching.index+1 ; i++) {
		std::cout<<urlmatching.V[i]<< ",";
	}
	std::cout<<"]"<<std::endl;

	std::cout<<prefix<<"matching_symbols_arr ("<<urlmatching.matching_symbols_idx<<")=[";
	for (uint32_t i=0; i<urlmatching.matching_symbols_idx ; i++) {
		std::cout<<urlmatching.matching_symbols_arr[i] <<"-"
				<<(*urlmatching.list)[urlmatching.matching_symbols_arr[i]]->_str << ",";
	}
	std::cout<<"]"<<std::endl;
//	patternsMapType* patternDB;						//map consist of <char* pattern , symbol >
//		symbolT* char_to_symbol;						//array [char] = symbols

}


