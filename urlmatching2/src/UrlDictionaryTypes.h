/*
 * types.h
 *
 *  Created on: 5 בדצמ 2014
 *      Author: Daniel
 */

#ifndef URLDICTIONARYTYPES_H_
#define URLDICTIONARYTYPES_H_


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <map>
#include <cstring>
#include <vector>

#ifndef UINT32_MAX
#define UINT32_MAX  (0xffffffff)	//this is redefined
#endif

#define STDENDL std::endl

typedef std::map<std::string,int> Strings2FreqMap;
typedef std::map<std::string,uint32_t> Strings2SymbolsMap;


/**
 * Defines a single pattern: <Pattern,Symbol,Frequency,Huffman length>
 */
class Pattern {
public:
	Pattern(uint32_t symbol, uint32_t frequency, std::string str);
	virtual ~Pattern() { /* Empty */}

	uint32_t inline getStringLength() { return (_str.length()*sizeof(char)); }
	uint32_t inline getHuffmanLength() { return (_huffman_length);	}

	//members
	uint32_t _symbol;
	uint32_t _frequency;
	std::string _str;
	uint32_t _huffman_length;

};

typedef std::vector<Pattern*> Symbol2pPatternArr;
typedef std::string** StringListType;
typedef uint32_t symbolT;
#define S_NULL 0

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return (std::strcmp(a, b) < 0);
   }
};
typedef std::map<const char*, symbolT, cmp_str> patternsMapType;







#endif /* URLDICTIONARYTYPES_H_ */
