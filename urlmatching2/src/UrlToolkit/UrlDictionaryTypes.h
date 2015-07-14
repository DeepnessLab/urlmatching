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

#define SIZEOFPOINTER sizeof(void *)

typedef struct {
	uint32_t* buf;
	uint16_t length;
} CodedHuffman;

// return the min. uint32_t size that contains 'bits' bits
inline
uint16_t conv_bits_to_uin32_size(uint32_t bits) {
	uint16_t byte_size = bits / (8*sizeof(uint32_t));
	byte_size = (bits % (8*sizeof(uint32_t)) == 0)? byte_size : byte_size + 1;
	return byte_size;
}

inline
uint16_t conv_bits_to_bytes(uint32_t bits) {
	uint16_t byte_size = bits / (8);
	byte_size = (bits % (8) == 0)? byte_size : byte_size + 1;
	return byte_size;
}


/**
 * Defines a single pattern: <Pattern,Symbol,Frequency,Huffman length>
 */
class Pattern {
public:
	Pattern(uint32_t symbol, uint32_t frequency, std::string str);
	virtual ~Pattern() ;

	uint32_t inline getStringLength() { return (_str.length()*sizeof(char)); }
	uint32_t inline getHuffmanLength() { return (_coded.length);	}

	inline 	uint32_t size() {
		uint32_t size = sizeof(_symbol)
				+ sizeof(_frequency)
				+ sizeof(std::string)
				+ _str.size()
				+ sizeof(CodedHuffman)
				+ ( conv_bits_to_uin32_size(getHuffmanLength()) ) ;
		return size;
	}

	//members
	uint32_t _symbol;
	uint32_t _frequency;
	std::string _str;
	CodedHuffman _coded;

};

typedef std::vector<Pattern*> Symbol2pPatternVec;
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


typedef struct {
	symbolT*** table;
	uint32_t size;
} symbolTableType;

typedef struct {
	uint32_t version;
	uint32_t num_of_patterns;
} FileHeader;

typedef struct {
	uint32_t symbol;
	uint32_t frequency;
	uint16_t str_length;
	uint16_t huffman_length; 	//in bits !
} FlatPattern;



#endif /* URLDICTIONARYTYPES_H_ */
