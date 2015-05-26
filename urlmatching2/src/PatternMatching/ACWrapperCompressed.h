/*
 * ACWrapperCompressed.h
 *
 *  Created on: 7 ���� 2014
 *      Author: Daniel
 */

#ifndef PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_
#define PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_

#include <map>
#include <string>
#include <cstring>
#include "../UrlDictionaryTypes.h"
#include "../CommonCTypes.h"
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

	//Load Patterns from file
	virtual bool load_patterns(std::string filepath);

	//map every pattern to symbol in patternList and build the pattern matching machine
	virtual bool load_patterns(Symbol2pPatternVec* patternsList, uint32_t size);


	/**
	 * @param input_str - input string to find matches
	 * @param result - output S_NULL terminated symbolT array with symbols of
	 * patterns covering the input string.
	 *  i.e. 1-AB, 2-C, 3-DE, and input_str ABCDE then result = {1,2,3,0}
	 * @return true is succeed, false for fail
	 */
	virtual bool find_patterns(std::string input_str, symbolT* result);

	virtual void printDB(std::ostream& os);

	virtual inline
	bool isLoaded() { return is_loaded; }

//	virtual std::string* nextPattern();
//	virtual void resetNextPattern();

	//members:

	void make_pattern_to_symbol_list();

private:

	symbolT* create_symb_string (const char* c_string);

	//members
	bool is_loaded;
	std::map<std::string,symbolT> patterns;

	//input during load
	Symbol2pPatternVec* _patternsList;

	patternsMapType* _patternsMap;

	StateMachine* _machine;
	symbolT _char_to_symbol[MAX_CHAR]; //TODO: replace this static define, with a dynamic allocated

};







#endif /* PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_ */
