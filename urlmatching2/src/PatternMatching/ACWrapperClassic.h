/*
 * AhoCrosarikClassic.h
 *
 *  Created on: 14 ���� 2014
 *      Author: Daniel
 */

#ifndef PATTERNMATCHING_ACWRAPPERCLASSIC_H_
#define PATTERNMATCHING_ACWRAPPERCLASSIC_H_

#include <map>
#include <string>
#include "../UrlToolkit/UrlMatchingTypes.h"
#include "PatternMatchingBaseClass.h"

extern "C"
{
#include "../StateMachine/TableStateMachine.h"
#include "../StateMachine/TableStateMachineGenerator.h"
}


class ACWrapperClassic : public PatternMatchingBaseClass {
public:
	//methods

	ACWrapperClassic() ;

	virtual ~ACWrapperClassic();

	virtual inline bool isInit() {
		return (_machine == NULL) ? false : true;
	}
	virtual bool LoadPatterns(std::string filepath);
	virtual bool load_patterns(Symbol2pPatternVec patternsList, uint32_t size);

	virtual bool MatchPatterns(std::string input_str, symbolT* result);

//	virtual std::string* nextPattern();
//	virtual void resetNextPattern();

	//members:
	std::map<std::string,int> patterns;

private:

//members
	Symbol2pPatternVec _patternsList;
//	std::map<std::string,int>::iterator patternIter;
	TableStateMachine* _machine=NULL;
};


#endif /* PATTERNMATCHING_ACWRAPPERCLASSIC_H_ */
