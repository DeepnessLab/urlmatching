/*
 * UrlMatchingFunctions.h
 *
 *  Created on: 7 December 2014
 *      Author: Daniel Krauthgamer
 *
 *  Since the Aho Corasick is written in C, and had no way to return the matching patterns back to caller it was
 *  modified to get `handle function` to execute the all matched string on it.
 *  MatchingFunctions.h contains those `handle pattern functions`
 *
 */

#ifndef PATTERNMATCHING_MATCHINGFUNCTIONS_H_
#define PATTERNMATCHING_MATCHINGFUNCTIONS_H_

#include "../UrlToolkit/UrlMatchingTypes.h"
#include <assert.h>
#include <string.h>
#include <cstring>
#include <iostream>
#include "../logger.h"

extern "C" {
#include "../AhoCorasick/ACBuilder.h"
}

#define MAX_URL_LENGTH 1000

/**
 * Data structure to hold an array of strings and an index
 */
typedef struct StringListDBWithIdxType {
	std::string* list;
	uint32_t index;
	uint32_t size;
} stringlistType;


/**
 * Struct that holds data needed to calculate Vi and Pi arrays
 */
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

std::ostream& operator<<(std::ostream& os, const urlMatchingType& m);


/**
 * Struct that holds data needed to update frequencies
 */
typedef struct frequencyUpdaterType {
	char * input_string;							//original url string
	Symbol2pPatternVec* list;
	symbolTableType* symbolsTable;
} frequencyUpdaterType ;

//This method is very slow since it uses stringstream object, don't use it in Release
void debugPrintModule (urlMatchingType& urlmatching);

int handle_pattern_count_hits(char* str, uint32_t idx, int id, int count, void* data);
int handle_pattern(char* str,uint32_t idx, int id, int count, void* data) ;
int handle_pattern_update_frequencies(char* str, uint32_t idx, int id, int count, void* data);


uint32_t getStringFromList_with_achor_selection(char* out_buff, uint32_t max_size, void* list_struct, ACTree* actree);
uint32_t getStringFromList(char* out_buff, uint32_t max_size, void* list_struct, ACTree* actree);

inline
void calcViPi(urlMatchingType& module) {
	//V[i] = argmin of P[i-l(a)] + p(a) for all matching a's - module.symbols
	uint32_t index4PV = module.index + 1 ;
	DBG("calcViPi:" << DVAL (index4PV));
	ASSERT(module.matching_symbols_idx > 0);

	//calc V[index4PV] according to module.matching_symbols_arr[0]
	ASSERT (index4PV < MAX_URL_LENGTH);
//	symbolT& symbol = module.V[index4PV];	//todo: remove this line
	module.V[index4PV] = module.matching_symbols_arr[0];

	Pattern* p = (*module.list)[module.matching_symbols_arr[0]];
	uint32_t stringlength = p->getStringLength();
	uint32_t huffmanLength = p->getHuffmanLength();
	//P[i=index4PV] = P1 + P2 = P[i-l(ai)] + p(V[i])
	ASSERT (index4PV >= stringlength);
	uint32_t P1 = module.P[index4PV - stringlength];
	uint32_t P2 = huffmanLength;

	uint32_t best_Vi = P1 /*P[i-l(a)]*/ + P2/*p(a)*/;
	DBG(DVAL(best_Vi)<<" = "<<DVAL(P1)<<"+"<<DVAL(P2));
	//look for a better matching symbol
	for (uint32_t i = 1; i < module.matching_symbols_idx ; i++ ) {
		symbolT symbol = module.matching_symbols_arr[i];
		ASSERT(symbol!=0);
		p= (*module.list)[symbol];
		//P[i-l(a)] + p(a) where l(a) - string length of symbol a, p(a) huffman code length of a
		stringlength = p->getStringLength();
		huffmanLength = p->getHuffmanLength();
		ASSERT (index4PV >= stringlength);
		P1 = module.P[index4PV - stringlength];
		P2 = huffmanLength;
		symbolT tmpVi = P1 + P2 ; 		// module.P[index4PV-stringlength] /*P[i-l(a)]*/ + huffmanLength/*p(a)*/;
		DBG(DVAL(tmpVi)<<" = "<<DVAL(P1)<<"+"<<DVAL(P2));
		if (tmpVi < best_Vi) {
			best_Vi =  tmpVi;
			ASSERT (index4PV < MAX_URL_LENGTH);
			module.V[index4PV] = module.matching_symbols_arr[i];
			DBG(DVAL(best_Vi)<<" and symbol "<<module.V[index4PV]);
		}
	}
	ASSERT (index4PV < MAX_URL_LENGTH);
	uint32_t& Pi = module.P[index4PV];
	Pi = best_Vi;
	DBG(DVAL(index4PV)<<":"<<module.P[index4PV] << "," << DVAL(module.V[index4PV])<< " " << DVAL(symbol));
	return;
}

inline
void  insert_symbol(urlMatchingType& module,symbolT& symbol, uint32_t& idx) {
	//collect new pattens until idx is advanced - then evaluate the vi pi module
	if (idx > module.index) {
		//new index, consolidate previous index
		DBG("idx("<<idx<<") > module.index("<<module.index<<") -> calcViPi");
		calcViPi(module);
//		ON_DEBUG_ONLY( debugPrintModule(module) );
		module.index = module.index+1;
		module.matching_symbols_idx = 0;
	}
	//gather another symbol
	DBG("updateModule:");
	module.matching_symbols_arr[module.matching_symbols_idx] = symbol;
	module.matching_symbols_idx++;
//	std::cout<<"idx="<<idx<<" " << module<<std::endl;
}



inline
uint32_t finalize_result(urlMatchingType& module, symbolT *result) {

	//insert all literals
	for (uint32_t j = module.index+1 ; module.input_string[j] != '\0' ; j++) {
		uint32_t char_at_j = (uint32_t) module.input_string[j];
		symbolT s =  module.char_to_symbol[char_at_j];
		insert_symbol(module,s,j);
		DBG(" > Added \""<<(char) char_at_j<<"\" at "<<j);
	}


	calcViPi(module);
//	ON_DEBUG_ONLY( debugPrintModule(module) );

	// Finalize the successful patterns
	uint32_t V_idx = module.index+1;
	uint32_t res_idx = V_idx;
	result[res_idx+1]=S_NULL;
	while (true) { //after we handle result[0] we will decrement idx by 1 to -1;
		symbolT symbol = module.V[V_idx];
		DBG( DVAL(res_idx) << " " << (*module.list)[symbol]->_str );
		result[res_idx] = symbol;
		//next idx is idx = idx -l(V[i])
		Pattern* p = (*module.list)[module.V[V_idx]]  ;
		uint32_t length = p->getStringLength();
		V_idx = V_idx - length;
		ASSERT(V_idx >= 0 );
		if (V_idx <= 0)
			break;
		res_idx--;
	}
	//copy result to start at results[0]
	uint32_t i =0;
	DBG( "result symbols:");
	while (result[i+res_idx] != S_NULL) {
		result[i] = result[i+res_idx];
		DBG( DVAL(i) << " " << (*module.list)[result[i]]->_str );
		i++;
	}
	result[i] = S_NULL;
	return i;//length of symbols "string"
}

inline
void initModule(urlMatchingType& module) {
	module.P[0]=0;
	module.V[0]=0;
	module.index=0;
	module.list=NULL;
	module.matching_symbols_idx=0;
}

inline
std::ostream& operator<<(std::ostream& os, const urlMatchingType& m) {
	os<<"Input string: "<<m.input_string<<std::endl;

	std::string st("");
	std::string st2(",");
	std::string* s = &st;
	os<<"MatchingPatterns[";
	s = &st;
	for (uint i = 0; i < m.matching_symbols_idx; i++) {
		Pattern* p =(*m.list)[m.matching_symbols_arr[i]];
		os<<*s<<"("<<(uint64_t) m.matching_symbols_arr[i]<<"|"<<p->_str<<"|h="<<p->getHuffmanLength()<<")";
		s=&st2;
	}
	os<<"]"<<std::endl;
	os<<"P[";
	s = &st;
	for (uint i = 0; i <= m.index ; i++) {
		os<<*s<<m.P[i];
		s=&st2;
	}
	os<<"]"<<std::endl;
	os<<"V[";
	s = &st;
	for (uint i = 0; i <= m.index ; i++) {
		Pattern* p = (*m.list)[m.V[i]];
		os<<*s<<"("<<(uint64_t) m.V[i]<<"|"<<p->_str<<"|h="<<p->getHuffmanLength()<<")";
//		os<<s<<"("<(uint64_t) m.V[i]<<" | "<<p->_str<<")";
		s=&st2;
	}
	os<<"]"<<std::endl;
	return os;
}

#endif /* PATTERNMATCHING_MATCHINGFUNCTIONS_H_ */
