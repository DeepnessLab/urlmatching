/*
 * UrlMatchingFunctions.h
 *
 *  Created on: 7 בדצמ 2014
 *      Author: Daniel
 */

#ifndef PATTERNMATCHING_MATCHINGFUNCTIONS_H_
#define PATTERNMATCHING_MATCHINGFUNCTIONS_H_

#include "../UrlDictionaryTypes.h"
#include <assert.h>
#include <string.h>

#define MAX_URL_LENGTH 1000

/**
 * Data structure to hold an array of strings and an index
 */
typedef struct StringListDBWithIdxType {
	std::string** list;
	uint32_t index;
	uint32_t size;
} stringlistType;


/**
 * Struct that holds data needed to calculate Vi and Pi arrays
 */
typedef struct urlMatchingType {
	char * input_string;
	uint32_t P[MAX_URL_LENGTH];
	symbolT V[MAX_URL_LENGTH];
	uint32_t index;									//current index of V and P we are working on
	symbolT matching_symbols_arr[MAX_URL_LENGTH];	//array that collects all symbols relevant to index (current index)
	uint32_t symbol_i;								//last used index of matching_symbols_arr
	Symbol2pPatternArr list;
	std::map<std::string,symbolT>* patternDB;
} urlMatchingType;



//some forward declarations
//inline void  calcViPi(urlMatchingType& module);

int handle_pattern(char* str,uint32_t idx, void* data) ; /*{
	urlMatchingType* url_module = (urlMatchingType*) data;
	assert(url_module!=NULL);
//	ACWrapperClassic ac = ACWrapperClassic::getInstance();
	std::string real_str(str);
	std::cout<<"at idx="<<idx<<", pattern="<<real_str<<std::endl;
//	ac.patterns[real_str(str)]=strlen(str);
	return 1;
}*/

uint32_t getStringFromList(char* out_buff, uint32_t max_size, void* list_struct);

inline
void calcViPi(urlMatchingType& module) {
	//V[i] = argmin of P[i-l(a)] + p(a) for all matching a's - module.symbols
	uint32_t& index = module.index;
	uint32_t minimum_index = module.symbol_i;
	assert(module.symbol_i > 0);
	module.V[index]=UINT32_MAX;
	Pattern* p;
	while (module.symbol_i > 0) {
		p= module.list[module.matching_symbols_arr[module.symbol_i]];
		//P[i-l(a)] + p(a) where l(a) - string length of symbol a, p(a) huffman code length of a
		symbolT tmpVi = module.P[index-p->getStringLength()] /*P[i-l(a)]*/ + p->getHuffmanLength()/*p(a)*/;
		if (tmpVi < module.V[index]) {
			module.V[index] =  tmpVi;
			minimum_index = module.symbol_i;
		}
	}
	p= module.list[module.matching_symbols_arr[minimum_index]];
	//P[i] = P[i-l(ai)] + p(V[i])
	module.P[index] = (index==0)?0 : module.P[index - p->getStringLength()] /*P[i] = P[i-l(ai)]*/
											  + module.list[module.matching_symbols_arr[module.V[index]]]->getHuffmanLength() /*p(V[i])*/;

}


inline
void  updateModule(urlMatchingType& module,symbolT& symbol, uint32_t& idx) {
	//collect new pattens until idx is advanced - then evaluate the vi pi module
	if (idx > module.index) {
		//new index, consolidate previous index
		calcViPi(module);
		module.index = module.index+1;
		module.symbol_i = 0;
	}
	//gather another symbol
	module.matching_symbols_arr[module.symbol_i] = symbol;
	module.symbol_i++;
}



inline
uint32_t finalize_result(urlMatchingType& module, symbolT *result) {

	calcViPi(module);

	// Finalize the successful patterns
	uint32_t V_idx = module.index;
	uint32_t next_idx= module.index;
	result[next_idx+1]=S_NULL;
	while (true) { //after we handle result[0] we will decrement idx by 1 to -1;
		result[next_idx] = module.V[V_idx];
		//next idx is idx = idx -l(V[i])
		uint32_t length =(module.list[module.V[V_idx]])->getStringLength();
		if (V_idx < length)
			break;
		//else
		V_idx = V_idx - length;
		next_idx--;
	}
	//copy result to start at results[0]
	uint32_t i =0;
	do {
		result[i] = result[i+next_idx];
		i++;
	} while (result[i]!=S_NULL);

	return i;//length of symbols "string"
}

inline
void initModule(urlMatchingType& module) {
	module.P[0]=0;
	module.index=0;
	module.list=NULL;
	module.symbol_i=0;

}



#endif /* PATTERNMATCHING_MATCHINGFUNCTIONS_H_ */
