/*
 * UrlMatchingTypes.h
 *
  *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  General types needed for UrlMathchingModule
 */

#ifndef URLDICTIONARYTYPES_H_
#define URLDICTIONARYTYPES_H_


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <map>
#include <cstring>
#include <vector>
#include <algorithm>    // std::sort


#ifndef UINT32_MAX
#define UINT32_MAX  (0xffffffff)
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
struct Pattern {
public:
	Pattern(uint32_t symbol, freqT frequency, const char* str);

	uint32_t inline getStringLength() const { return (_str_len*sizeof(char)); }
	uint32_t inline getHuffmanLength() const { return (_coded.length);	}
	void 	 inline invalidate() {
		_frequency=0;
		_coded.length=8*MAX_CODED_HUFFMAN_SIZE;
	}

	inline 	size_t size() const {
		size_t size = sizeof(Pattern)
				+ _str_len;
		return size;
	}

	//members				(sizes in 64bit OS)
	symbolT _symbol;		//4 bytes
	freqT _frequency; 	//4 bytes
	const char* _str;		//8 bytes
	CodedHuffman _coded;	//12bytes
	uint16_t _str_len;		//2 bytes


//Evaluate anchors methods and members
	static uint32_t total_frequency;
	static char C_state;	//C stat constant (how many bytes per state)
	long double get_h() const ;
	long double get_gain() const;


};


//inline
//std::ostream& operator<<(std::ostream& os, const Pattern& m) {
//	os << "Pattern["
//			<<m._symbol<<","
//			<<m._str
//			<<",f="
//			<<m._frequency
//			<<",len(h)="
//			<<"]";
//	return os;
//}

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

struct greater_than_gain
{
	typedef Pattern* pPattern;
	inline bool operator() (const pPattern& p1, const pPattern& p2)
	{
		long double g1 = p1->get_gain();
		long double g2 = p2->get_gain();
		return (g1 > g2);
	}
};



class PatternsIterator {
public:
	PatternsIterator(uint32_t size, bool optimize) : _pattern_vec()
		, _it(_pattern_vec.begin())
		, _set(false)
		, _optimize(optimize)
	{
		_pattern_vec.clear();
		_pattern_vec.reserve(size);
	}

	PatternsIterator(Symbol2pPatternVec& vec, bool optimize) : _pattern_vec()
		, _it(_pattern_vec.begin())
		, _set(false)
		, _optimize(optimize)
	{
		_pattern_vec.clear();
		_pattern_vec.reserve(vec.size()+1);
		for (Symbol2pPatternVec::iterator it=vec.begin(); it != vec.end(); ++it) {
			Pattern* p = *it;
			if (strlen(p->_str) <= 1)
				continue;
			_pattern_vec.push_back(p);
		}
	}

	virtual ~PatternsIterator() {
	}

	void insert(Pattern* p) {
		_pattern_vec.push_back(p);
	}

	void resetIterator() { _set = false; }

	Pattern* getNext() {
		if (!_set ) {
			std::sort(_pattern_vec.begin(), _pattern_vec.end(), greater_than_gain());
			_it = _pattern_vec.begin();
			_set=true;
		}
		if (_it == _pattern_vec.end())
			return 0;
		Pattern* p = *_it;
		++(_it);
		return p;
	}

	bool shouldOptimize() {return _optimize;}

private:
	Symbol2pPatternVec _pattern_vec;
	Symbol2pPatternVec::iterator _it;
	bool _set;
	bool _optimize;	//optimize patterns according to anchor selection

};

#endif /* URLDICTIONARYTYPES_H_ */
