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
#include "../UrlDictionaryTypes.h"
#include "../CommonCTypes.h"
#include "PatternMatchingBaseClass.h"

extern "C"
{
#include "../StateMachine/StateMachine.h"
#include "../StateMachine/StateMachineGenerator.h"
}

int special_handle_pattern(char* str,uint32_t idx, void* data) ;

class ACWrapperCompressed : public PatternMatchingBaseClass {
public:
	//methods

	ACWrapperCompressed() ;

	virtual ~ACWrapperCompressed();

	virtual inline bool isInit() {
		return (_machine == NULL) ? false : true;
	}
	virtual bool load_patterns(std::string filepath);
	/**
	 * Assign a symbol to every pattern in the patternList and build the pattern matching machine
	 */
	virtual bool load_patterns(Symbol2pPatternArr patternsList, uint32_t size);

	virtual bool find_patterns(std::string input_str, symbolT* result);

//	virtual std::string* nextPattern();
//	virtual void resetNextPattern();

	//members:
	std::map<std::string,int> patterns;

private:

	void make_pattern_to_symbol_list();

//members
	Symbol2pPatternArr _patternsList;
//	std::map<std::string,int>::iterator patternIter;
	StateMachine* _machine=NULL;
	symbolT _char_to_symbol[1000]; //TODO: replace this static define
};







#endif /* PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_ */
