/*
 * AhoCorasikBase.h
 *
 *  Created on: 14 бреб 2014
 *      Author: Daniel
 */

#ifndef WRAPPERS_PATTERNMATCHINGBASECLASS_H_
#define WRAPPERS_PATTERNMATCHINGBASECLASS_H_

#include <string>
#include "../UrlToolkit/UrlDictionaryTypes.h"

//class PatternCollector {
//public:
//	PatternCollector();
//	virtual ~PatternCollector();
//
//	int virtual insert();
//};




class PatternMatchingBaseClass {
public:
	PatternMatchingBaseClass();
	virtual ~PatternMatchingBaseClass();

	virtual bool isLoaded() const =0  ;
	virtual bool load_patterns(std::string filepath)=0;
	virtual bool MatchPatterns(std::string input_str, symbolT* result)=0;

private:

	int add_pattern(char* pattern);
//	uint32_t num_of_patterns;
};

#endif /* WRAPPERS_PATTERNMATCHINGBASECLASS_H_ */
