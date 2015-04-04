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

#ifndef UINT32_MAX
#define UINT32_MAX  (0xffffffff)	//this is redefined
#endif


typedef std::map<std::string,int> Strings2FreqType;
//typedef std::map<std::string,int> Strings2FreqType;
typedef std::map<std::string,uint32_t> Strings2SymbolsType;


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

typedef Pattern** Symbol2PatternType;
typedef std::string** StringListType;
typedef uint32_t symbolT;
#define S_NULL 0




#endif /* URLDICTIONARYTYPES_H_ */
