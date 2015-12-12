/*
 * AhoCorasikBase.h
 *
 *  Created on: 14 ���� 2014
 *      Author: Daniel
 */

#ifndef WRAPPERS_PATTERNMATCHINGBASECLASS_H_
#define WRAPPERS_PATTERNMATCHINGBASECLASS_H_

#include <string>
#include "../UrlToolkit/UrlMatchingTypes.h"


class PatternMatchingBaseClass {
public:
	PatternMatchingBaseClass() {}
	virtual ~PatternMatchingBaseClass() {}

	virtual bool isLoaded() const = 0  ;
	virtual bool LoadPatterns(std::string filepath)= 0;
	virtual bool MatchPatterns(std::string input_str, symbolT* result) = 0;

};

#endif /* WRAPPERS_PATTERNMATCHINGBASECLASS_H_ */
