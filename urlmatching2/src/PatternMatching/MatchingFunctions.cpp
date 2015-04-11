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
void go_over_all_patterns(const char* delimiter, char* string_with_patterns,
		uint32_t idx_of_last_char) {
	//assuming here the patterns are in decreasing length order (the longest is the first)
	std::cout << "string_with_patterns=" << string_with_patterns << std::endl;
	char* s = new char[strlen(string_with_patterns)];
	strcpy(s, string_with_patterns);
	char* tk = strtok(s, delimiter);
	while (tk != NULL) {
		std::cout << "  at idx=" << idx_of_last_char << ", pattern=" << tk
				<< std::endl;
		tk = strtok(NULL, delimiter);
	}
}

int handle_pattern(char* str, uint32_t idx, void* data) {
	if (str == NULL) {
		return 0;
	}
	urlMatchingType* url_module = (urlMatchingType*) data;

	assert(url_module!=NULL);
	go_over_all_patterns(";", str, idx);
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
