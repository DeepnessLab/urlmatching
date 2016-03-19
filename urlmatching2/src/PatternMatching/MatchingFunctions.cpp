/*
 * MatchingFunctions.cpp

 *
 *  Created on: 8 December 2014
 *      Author: Daniel Krauthgamer
 *
 *	See header file for comments
 */

#include "MatchingFunctions.h"
#include <stdio.h>
#include <iostream>


int handle_pattern_count_hits(char* str, uint32_t idx, int id, int count, void* data)
{
	ASSERT(data != NULL);
	DBG("handle_pattern_count_hits: \""<<str<< "\" at "<<idx <<", hits = "<< *hits);
	uint64_t* hits = (uint64_t*) data;
	(*hits)++;

	return 1;
}

int handle_pattern_update_frequencies(char* str, uint32_t idx, int id, int count, void* data)
{
	ASSERT(data != NULL);
	DBG("handle_pattern_update_frequencies: \""<<str<< "\" at "<<idx <<", hits = "<< *hits);
	frequencyUpdaterType* module = (frequencyUpdaterType*) data;
	uint64_t* hits = (uint64_t*) data;
	(*hits)++;
	symbolT* symbols = module->symbolsTable->table[id][count];
	ASSERT(symbols!=NULL);
	symbolT last_symbol = S_NULL; //skip same copies of the same string
	while (*symbols != S_NULL) {
		if ( last_symbol != *symbols) {
			Pattern* p = module->list->at(*symbols);
			p->_frequency = p->_frequency + 1;
		}
		last_symbol = *symbols;
		symbols++;
	}
	return 1;
}


int handle_pattern(char* str, uint32_t idx, int id, int count, void* data)
{
	urlMatchingType* url_module = (urlMatchingType*) data;
	ASSERT(url_module!=NULL);
	DBG("handle_pattern: \""<<str<< "\" at "<<idx);

	//insert all literals
	for (uint32_t j = url_module->index+1 ; j <=idx ; j++) {
		uint32_t char_at_j = (uint32_t) url_module->input_string[j];
		symbolT s =  url_module->char_to_symbol[char_at_j];
		insert_symbol(*url_module,s,j);
		DBG(" > Added \""<<(char) char_at_j<<"\" at "<<j);
	}
	symbolT* symbols = url_module->symbolsTable->table[id][count];
	ASSERT(symbols!=NULL);
	symbolT last_symbol = S_NULL; //skip same copies of the same string
	while (*symbols != S_NULL) {
		Pattern* p = (*url_module->list)[*symbols];
#define likely(x)      __builtin_expect(!!(x), 1)
		if (likely( last_symbol != *symbols && p->getHuffmanLength() > 0)) {
			insert_symbol(*url_module,*symbols,idx);
		}
		last_symbol = *symbols;
		symbols++;
	}

	return 1;

}

/** used to load all patterns into ac builder
 *
 * @param out_buff
 * @param max_size
 * @param list_struct
 * @return
 */
uint32_t getStringFromList_with_achor_selection(char* out_buff, uint32_t max_size,
		void* list_struct, ACTree* actree) {

	PatternsIterator* list = (PatternsIterator*) list_struct;
	//when optimized - getNext will be sorted by Gain
	Pattern* p = list->getNext() ;
	while (p != 0) {
		if (p->_frequency > 0) {
			typedef long double ld ;
			ld h = p->get_h();			//in bits
			ld gain = p->get_gain();	//in bits
			int num_states = enter_simulate_addional_states(actree, (char*) p->_str, p->_str_len);
			ld cost =  (ld) (num_states * Pattern::C_state /*bytes*/ * 8 /*to bits*/ ) + h;
			p->_coded.length = ((uint16_t) h > 0 ) ? (uint16_t) h : 1;
			if (gain >= cost) {
				const char* chars = p->_str;
				strncpy(out_buff, chars, max_size);
				DBG("adding \"" << chars << "\"; ");
				return strlen(chars);
			}
		}
		p->_frequency=0;
		p->_coded.length=8*MAX_CODED_HUFFMAN_SIZE;
		p = list->getNext();
	}

	DBG(std::endl << "Total of " << list->size
			<< " patterns loaded");
	return 0;

}


/** used to load all patterns into ac builder
 *
 * @param out_buff
 * @param max_size
 * @param list_struct
 * @return
 */
uint32_t getStringFromList(char* out_buff, uint32_t max_size,
		void* list_struct, ACTree* actree) {

	PatternsIterator* list = (PatternsIterator*) list_struct;
	//when optimized - getNext will be sorted by Gain
	Pattern* p = list->getNext() ;

	if (p == 0) {
		DBG(std::endl << "Total of " << list->size
						<< " patterns loaded");
		return 0;
	}
	const char* chars = p->_str;
	strncpy(out_buff, chars, max_size);
	DBG("adding \"" << chars << "\"; ");
	return strlen(chars);
}

void debugPrintModule (urlMatchingType& urlmatching) {
	const char* prefix = "' ";
	DBG(prefix<<"Index of P and V ="<<urlmatching.index+1);
	std::stringstream s1;
	s1 <<prefix<<"P=[";
	for (uint32_t i=0; i<=urlmatching.index+1 ; i++) {
		s1<<urlmatching.P[i]<< ",";
	}
	DBG(s<<"]");

	std::stringstream s2;
	s2 <<prefix<<"V=[";
	for (uint32_t i=0; i<=urlmatching.index+1 ; i++) {
		s2 <<urlmatching.V[i]<< ",";
	}
	DBG(s2<<"]");

	std::stringstream s3;
	s3 <<prefix<<"matching_symbols_arr ("<<urlmatching.matching_symbols_idx<<")=[";
	for (uint32_t i=0; i<urlmatching.matching_symbols_idx ; i++) {
		s3 << urlmatching.matching_symbols_arr[i] <<"-"
				<<(*urlmatching.list)[urlmatching.matching_symbols_arr[i]]->_str << ",";
	}
	DBG(s3<<"]");
}


