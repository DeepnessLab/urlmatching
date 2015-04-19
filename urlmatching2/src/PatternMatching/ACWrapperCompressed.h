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

	virtual inline bool isInit() { return ( _machine == NULL) ? false : true ;	}

	virtual bool load_patterns(std::string filepath);
	/**
	 * Assign a symbol to every pattern in the patternList and build the pattern matching machine
	 */
	virtual bool load_patterns(Symbol2pPatternArr patternsList, uint32_t size);

	virtual bool find_patterns(std::string input_str, symbolT* result);

	virtual void printDB(std::ostream& os);

//	virtual std::string* nextPattern();
//	virtual void resetNextPattern();

	//members:


private:


	void make_pattern_to_symbol_list();

	//members
	bool is_loaded;
	std::map<std::string,symbolT> patterns;

	Symbol2pPatternArr _patternsList;

	patternsMapType* _patternsMap;

	StateMachine* _machine;
	symbolT _char_to_symbol[MAX_CHAR]; //TODO: replace this static define, with a dynamic allocated

};







#endif /* PATTERNMATCHING_ACWRAPPERCOMPRESSED_H_ */
