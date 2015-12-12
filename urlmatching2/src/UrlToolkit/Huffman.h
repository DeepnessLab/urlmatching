/*
 * Huffman.h
 *	Based of huffman source code
 *	  taken from http://rosettacode.org/wiki/Huffman_coding#C.2B.2B
 *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  This Huffman implementation is created to be general, for a large dictionary size.
 *  It can be extended by defining symbolT
 *
 */

#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <iostream>
#include <queue>
#include <map>
#include <climits> // for CHAR_BIT
#include <iterator>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <sstream>
#include "UrlMatchingTypes.h"


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
	virtual ~InternalNode() {
		delete left;
		delete right;
	}
};

class LeafNode: public INode {
public:
	const symbolT c;

	LeafNode(freqT f, uint32_t c) :
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
	Huffman() : _is_loaded(false),_freq_size(0) {}
	virtual ~Huffman()  {}

	void load(freqT* frequencies, uint32_t size);
	void free_encoding_memory();
	void print();

	inline bool isLoaded() {return _is_loaded;}

	HuffCode encode(symbolT symbol) ;
	bool decode(HuffCode code, symbolT& out_decoded_symbol);

	inline
	void printHuffCode(HuffCode* code) {
		std::cout<<"code=";
		std::copy(code->begin(), code->end(),
				std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}

	inline
	std::string HuffCode_str(HuffCode* code) {
		std::stringstream stream;
		std::copy(code->begin(), code->end(),
				std::ostream_iterator<bool>(stream));
		return stream.str();
	}

	uint32_t	size() const;


private:
	INode* BuildTree(freqT* frequencies);
	void FreeTree(const INode* node);
	void GenerateCodes(const INode* node, const HuffCode& prefix,
			HuffCodeMap& outCodes);

	HuffCodeMap _symbol2codesMap;
	HuffSymbMap _codes2symbolsMap;
	bool 		_is_loaded ;
	uint32_t 	_freq_size ;

};

#endif /* HUFFMAN_H_ */
