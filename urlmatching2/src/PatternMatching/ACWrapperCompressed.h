/*
 * ACWrapperCompressed.h
 *
 *  Created on: 7 December 2014
 *      Author: Daniel Krauhgamer
 *
 *  A C++ Wrapper module for Compressed Aho-Corasick written in C.
 *  It is performance enhanced for memory efficiency. It does not hold a copy of the pattern themselves.
 */

#ifndef PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_
#define PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_

#include <map>
#include <string>
#include <cstring>
#include "../UrlToolkit/UrlMatchingTypes.h"
#include "../UrlToolkit/SerialAllocator.h"
#include "PatternMatchingBaseClass.h"

extern "C"
{
#include "../StateMachine/StateMachine.h"
#include "../StateMachine/StateMachineGenerator.h"
#include "../AhoCorasick/ACBuilder.h"
}

#define MAX_CHAR 1000


int special_handle_pattern(char* str,uint32_t idx, void* data) ;

class ACWrapperCompressed : public PatternMatchingBaseClass {
public:
	//methods

	ACWrapperCompressed() ;
	virtual ~ACWrapperCompressed();

	//Build pattern matching machine out of patterns vector
	virtual bool LoadPatterns(Symbol2pPatternVec* patternsList, uint32_t size
			, bool optimizeAnchors = false);

	//Load Patterns from file (not in use)
	virtual bool LoadPatterns(std::string filepath);

	/**
	 * @param input_str - input string to find matches
	 * @param result - output S_NULL terminated symbolT array with symbols of
	 * patterns covering the input string.
	 *  i.e. 1-AB, 2-C, 3-DE, and input_str ABCDE then result = {1,2,3,0}
	 * @return true is succeed, false for fail
	 */
	virtual bool MatchPatterns(std::string input_str, symbolT* result);
	virtual bool MatchPatternsNoPatterns(std::string input_str, symbolT* result);

	//Dump the state machine and reload it from dump file since it should take less memory
	void optimize_statemachine();

	virtual void printDB(std::ostream& os);
	void dump_states(std::string filename) const;

	inline uint32_t size() const {
		uint32_t size = 0;
		size+=MAX_CHAR;
		size+= sizeof(_machine);
		size+=_symbolsTable.size * sizeof(symbolT*);
		size+=_symbolsTableLevel1->capacity();
		size+=_symbolsTableLevel2->capacity();
		size+=sizeof(_statemachine_size);
		size+=sizeof(_size);
		size+=sizeof(_delimiter) + 2;
		return size;
	}
	inline uint32_t getStateMachineSize() const { return _statemachine_size; }

	virtual inline
	bool isLoaded() const { return _is_loaded; }


private:

	//Methods
	void make_pattern_to_symbol_list(bool verbose = false);
	inline	symbolT* create_symb_string (const char* c_string);

	//Members
	bool _is_loaded;
	bool _has_patterns;

	Symbol2pPatternVec* _patternsList; //input during load
	patternsMapType* _patternsMap;	//temporary map used during LoadPatterns

	//patterns_as_symbols is an array of array of "symbolT string" i.e S_NULL terminated
	// patterns_as_symbols[i] is an array of dynamic size where the last pointer is NULL
	// 	i.e patterns_as_symbols[i] 		= {symbolT*,symbolT*,..,NULL}
	//		patterns_as_symbols [i][j] 	= {symbol1,symbol2,..,S_NULL}
	symbolTableType _symbolsTable;
	SerialAllocator<symbolT*>* 	_symbolsTableLevel1;
	SerialAllocator<symbolT>* 	_symbolsTableLevel2;

	StateMachine* _machine;
	symbolT _char_to_symbol[MAX_CHAR]; //TODO: replace this static define, with a dynamic allocated

	uint32_t _size;
	uint32_t _statemachine_size; //only in linux systems

	const char* _delimiter = "`";

};


#endif /* PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_ */
