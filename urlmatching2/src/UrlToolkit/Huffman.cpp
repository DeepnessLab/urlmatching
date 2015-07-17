/*
 * Huffman.cpp
 *
 *  Created on: 1 ���� 2014
 *      Author: Daniel
 */

#include <stdio.h>
#include <assert.h>

#include "Huffman.h"
#include "../common.h"
#include "../logger.h"

Huffman::Huffman() :
_is_loaded(false),_freq_size(0), _size(0){
	// TODO Auto-generated constructor stub

}

Huffman::~Huffman() {
	// TODO Auto-generated destructor stub
}

INode* Huffman::BuildTree()
{
	std::priority_queue<INode*, std::vector<INode*>, NodeCmp> trees;

	for (uint32_t i = 0; i < _freq_size; ++i)
	{
		if(_frequencies[i] != 0)
			trees.push(new LeafNode(_frequencies[i], (uint32_t)i));
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
//		_codes2symbolsMap.clear();
//		_is_loaded = false;
}

HuffCode Huffman::encode(uint32_t symbol) {
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

void Huffman::load(uint32_t* frequencies, uint32_t size) {
	DBG("Entered Huffman::load size="<<size);
	_freq_size = size;

	if (_frequencies!=NULL) {
		DELETE_AND_NULL(_frequencies);
	}

	_frequencies = frequencies;

	//debug print
//	std::cout<<"Input frequencies for symbols:"<<std::endl;
//	for (uint32_t i =0 ; i< _freq_size ; i++) {
//		DBG("frequencies["<<i<<"]="<<frequencies[i]);
//	}
	INode* root = BuildTree();

	Huffman::GenerateCodes(root, HuffCode(), _symbol2codesMap);

	for (HuffCodeMap::const_iterator it = _symbol2codesMap.begin(); it != _symbol2codesMap.end(); ++it)
	{
		_codes2symbolsMap.insert( std::pair<HuffCode,uint32_t>(it->second,it->first) );
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
		std::cout << (uint32_t) it->first << " ";
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
