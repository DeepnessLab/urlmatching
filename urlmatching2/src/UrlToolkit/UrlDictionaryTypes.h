/*
 * types.h
 *
 *  Created on: 5 ���� 2014
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
#define S_NULL 0

typedef std::string* StringListType;
typedef uint32_t symbolT;
typedef uint32_t freqT;

#define MAX_CODED_HUFFMAN_SIZE 2
typedef struct {
	uint32_t buf[MAX_CODED_HUFFMAN_SIZE];
	uint16_t length;	//in bits
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
	Pattern(uint32_t symbol, freqT frequency, const char* str);
	virtual ~Pattern() ;

	uint32_t inline getStringLength() { return (_str_len*sizeof(char)); }
	uint32_t inline getHuffmanLength() { return (_coded.length);	}

	inline 	size_t size() {
		size_t size = sizeof(Pattern)
				+ _str_len;
		return size;
	}

	//members				(sizes in 64bit OS)
	symbolT _symbol;		//4 bytes
	uint16_t _str_len;		//2 bytes
	const char* _str;		//8 bytes
	CodedHuffman _coded;	//12bytes
	freqT _frequency; 	//4 bytes

};

typedef std::vector<Pattern*> Symbol2pPatternVec;


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
	uint32_t chars_allocator_size;
} FileHeader;

typedef struct {
	symbolT	 symbol;
	freqT	 frequency;
	uint16_t str_length;
	uint16_t huffman_length; 	//in bits !
} FlatPattern;



#endif /* URLDICTIONARYTYPES_H_ */
