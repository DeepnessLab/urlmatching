/*
 * PrintUrlMatching.h
 *
 *  Created on: Mar 14, 2016
 *      Author: daniel
 */

#ifndef SRC_URLTOOLKIT_PRINTURLMATCHING_H_
#define SRC_URLTOOLKIT_PRINTURLMATCHING_H_

#include <stdio.h>
#include"UrlMatchingTypes.h"
#include "../PatternMatching/MatchingFunctions.h"

inline
std::ostream& operator<<(std::ostream& os, const symbolT* symbol_p)
{
	const symbolT* s=symbol_p;
	os << "[";
	std::string st("");
	std::string st2(",");
	while (*s != S_NULL) {
		os<<st<<(uint64_t) *s;
		s++;
		st=st2;
	}
	os << "]";
	return os;
}






	/*

typedef struct urlMatchingType {
	char * input_string;							//original url string
	uint32_t P[MAX_URL_LENGTH];
	symbolT V[MAX_URL_LENGTH];
	uint32_t index;									//current index of V and P we are working on
	symbolT matching_symbols_arr[MAX_URL_LENGTH];	//array that collects all symbols relevant to index (current index)
	uint32_t matching_symbols_idx;								//last used index of matching_symbols_arr
	Symbol2pPatternVec* list;
	symbolT* char_to_symbol;						//array [char] = symbols
	symbolTableType* symbolsTable;
} urlMatchingType;
	 */


#endif /* SRC_URLTOOLKIT_PRINTURLMATCHING_H_ */
