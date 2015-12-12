/*
 * Huffman.h
 *	Based of huffman source code
 *	  taken from http://rosettacode.org/wiki/Huffman_coding#C.2B.2B
 *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  Please read huffman.h header comment
 *
 */


#include <stdio.h>
#include <assert.h>
#include "Huffman.h"
#include "../common.h"
#include "../logger.h"


INode* Huffman::BuildTree(freqT* frequencies)
{
	assert(frequencies!=0);
	std::priority_queue<INode*, std::vector<INode*>, NodeCmp> trees;

	for (symbolT i = 0; i < _freq_size; ++i)
	{
		if(frequencies[i] != 0)
			trees.push(new LeafNode(frequencies[i], (symbolT)i));
	}
	while (trees.size() > 1)
	{
		INode* childR = trees.top();
		trees.pop();

		INode* childL = trees.top();
		trees.pop();

		INode* parent = new InternalNode(childR, childL);
		trees.push(parent);
	}
	return trees.top();
}

void Huffman::free_encoding_memory() {
		_symbol2codesMap.clear();
}

HuffCode Huffman::encode(symbolT symbol) {
	assert(_is_loaded);
	return _symbol2codesMap[symbol];
}

bool Huffman::decode(HuffCode code, symbolT& symbol) {
	assert(_is_loaded);
	HuffSymbMap::iterator it;
	it = _codes2symbolsMap.find(code);
	if (it == _codes2symbolsMap.end()) {
		return false;
	}
	symbol = it->second;
	return true;
}

void Huffman::GenerateCodes(const INode* node, const HuffCode& prefix, HuffCodeMap& outCodes)
{
	if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
	{
		outCodes[lf->c] = prefix;
	}
	else if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
	{
		HuffCode leftPrefix = prefix;
		leftPrefix.push_back(false);
		leftPrefix.shrink_to_fit();
		GenerateCodes(in->left, leftPrefix, outCodes);

		HuffCode rightPrefix = prefix;
		rightPrefix.push_back(true);
		rightPrefix.shrink_to_fit();
		GenerateCodes(in->right, rightPrefix, outCodes);
	}
}

void Huffman::load(freqT* frequencies, uint32_t size) {
	DBG("Entered Huffman::load size="<<size);
	_freq_size = size;

	INode* root = BuildTree(frequencies);
	Huffman::GenerateCodes(root, HuffCode(), _symbol2codesMap);
	for (HuffCodeMap::const_iterator it = _symbol2codesMap.begin(); it != _symbol2codesMap.end(); ++it)
	{
		_codes2symbolsMap.insert( std::pair<HuffCode,symbolT>(it->second,it->first) );
	}

//	FreeTree(root);
	delete root;
	_is_loaded=true;
}

void Huffman::FreeTree(const INode* node)
{
	if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
	{
		delete lf;
	}
	else if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
	{
		delete in;
	}
}

void Huffman::print() {
	std::cout<<"Huffman codes:"<<std::endl;
	for (HuffCodeMap::const_iterator it = _symbol2codesMap.begin(); it != _symbol2codesMap.end(); ++it)
	{
		std::cout << (symbolT) it->first << " ";
		std::copy(it->second.begin(), it->second.end(),
				std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}
}

//This is lazy size (go over module and counts memory
uint32_t Huffman::size() const
{
	uint32_t size=0;
	for (HuffCodeMap::const_iterator it = _symbol2codesMap.begin(); it != _symbol2codesMap.end(); ++it)
		size+= (sizeof(symbolT) + it->second.capacity());
	size+=_codes2symbolsMap.size() * (sizeof(void *));

	for (HuffSymbMap::const_iterator it = _codes2symbolsMap.begin(); it != _codes2symbolsMap.end(); ++it)
		size+= (sizeof(symbolT) + it->first.capacity());
	std::cout<<"Huffman size = "<<Byte2KB(size)<<"KB"<<STDENDL;
	return size;
}
