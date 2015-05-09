/*
 * Huffman.h
 *
 *  Created on: 1 ���� 2014
 *      Author: Daniel
 */

#ifndef HUFFMAN_H_
#define HUFFMAN_H_

//#include <stddef.h>
//#include <stdint.h>
//#include <map>
//#include <vector>


#include <iostream>
#include <queue>
#include <map>
#include <climits> // for CHAR_BIT
#include <iterator>
#include <algorithm>
#include <stdint.h>
#include "UrlDictionaryTypes.h"


//const int UniqueSymbols = 1 << CHAR_BIT;
//const char* SampleString = "this is an example for huffman encoding";

typedef std::vector<bool> HuffCode;
typedef std::map<uint32_t, HuffCode> HuffCodeMap;
typedef std::map<HuffCode, uint32_t> HuffSymbMap;

class INode {
public:
	const uint32_t f;
	virtual ~INode() {
	}
protected:
	INode(uint32_t f) :
			f(f) {
	}
};

class InternalNode: public INode {
public:
	INode * const left;
	INode * const right;

	InternalNode(INode* c0, INode* c1) :
			INode(c0->f + c1->f), left(c0), right(c1) {
	}
	~InternalNode() {
		delete left;
		delete right;
	}
};

class LeafNode: public INode {
public:
	const uint32_t c;

	LeafNode(uint32_t f, uint32_t c) :
			INode(f), c(c) {
	}
};

struct NodeCmp {
	bool operator()(const INode* lhs, const INode* rhs) const {
		return lhs->f > rhs->f;
	}
};

class Huffman {
public:
	Huffman();
	virtual ~Huffman();

	void load(uint32_t* frequencies, uint32_t size);
	void print();

	HuffCode encode(uint32_t symbol);
	uint32_t decode(HuffCode code);

	inline void printHuffCode(HuffCode* code) {
		std::cout<<"code=";
		std::copy(code->begin(), code->end(),
				std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}


private:
	INode* BuildTree();
	void GenerateCodes(const INode* node, const HuffCode& prefix,
			HuffCodeMap& outCodes);

	HuffCodeMap _codes;
	HuffSymbMap _codes2symbols;
	bool _is_loaded ;
	uint32_t* _frequencies = NULL;
	uint32_t _freq_size ;

};

#endif /* HUFFMAN_H_ */
