/*
 * MatchingFunctions.cpp

 *
 *  Created on: 8 בדצמ 2014
 *      Author: Daniel
 */


#include "MatchingFunctions.h"
#include <stdio.h>
#include <iostream>

int handle_pattern(char* str,uint32_t idx, void* data) {
	urlMatchingType* url_module = (urlMatchingType*) data;
	assert(url_module!=NULL);
//	ACWrapperClassic ac = ACWrapperClassic::getInstance();
	std::string real_str(str);
	std::cout<<"at idx="<<idx<<", pattern="<<real_str<<std::endl;
//	ac.patterns[real_str(str)]=strlen(str);
	return 1;
}

uint32_t getStringFromList(char* out_buff, uint32_t max_size, void* list_struct) {
	stringlistType* list_describtor = (stringlistType*) list_struct;
	if (list_describtor->index == list_describtor->size)
		return 0;
	const char* chars= list_describtor->list[list_describtor->index]->c_str();
	list_describtor->index = list_describtor->index+1;
	strncpy(out_buff,chars,max_size);
	return strlen(chars);
}
