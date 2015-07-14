/*
 * ACWrapperCompressed.h
 *
 *  Created on: 7 בדצמ 2014
 *      Author: Daniel
 */

#ifndef PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_
#define PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_

#include <map>
#include <string>
#include <cstring>
#include "../UrlToolkit/UrlDictionaryTypes.h"
#include "PatternMatchingBaseClass.h"

extern "C"
{
#include "../StateMachine/StateMachine.h"
#include "../StateMachine/StateMachineGenerator.h"
}

#define MAX_CHAR 1000


int special_handle_pattern(char* str,uint32_t idx, void* data) ;

class ACWrapperCompressed : public PatternMatchingBaseClass {
public:
	//methods

	ACWrapperCompressed() ;
	virtual ~ACWrapperCompressed();

	//Build pattern matching machine out of patterns vector
	virtual bool load_patterns(Symbol2pPatternVec* patternsList, uint32_t size);

	//Load Patterns from file (not in use)
	virtual bool load_patterns(std::string filepath);

	/**
	 * @param input_str - input string to find matches
	 * @param result - output S_NULL terminated symbolT array with symbols of
	 * patterns covering the input string.
	 *  i.e. 1-AB, 2-C, 3-DE, and input_str ABCDE then result = {1,2,3,0}
	 * @return true is succeed, false for fail
	 */
	virtual bool find_patterns(std::string input_str, symbolT* result);

	virtual void printDB(std::ostream& os);
	void dump_states(std::string filename) const;

	inline uint32_t size() { return _size; }

	virtual inline
	bool isLoaded() const { return is_loaded; }


private:
	void make_pattern_to_symbol_list();

	inline
	symbolT* create_symb_string (const char* c_string);

	//members
	bool is_loaded;

	//input during load
	Symbol2pPatternVec* _patternsList;

	patternsMapType* _patternsMap;

	//patterns_as_symbols is an array of array of "symbolT string" i.e S_NULL terminated
	// patterns_as_symbols[i] is an array of dynamic size where the last pointer is NULL
	// 	i.e patterns_as_symbols[i] 		= {symbolT*,symbolT*,..,NULL}
	//		patterns_as_symbols [i][j] 	= {symbol1,symbol2,..,S_NULL}
	symbolTableType _symbolsTable;

	StateMachine* _machine;
	symbolT _char_to_symbol[MAX_CHAR]; //TODO: replace this static define, with a dynamic allocated

	uint32_t _size;

};







#endif /* PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_ */
