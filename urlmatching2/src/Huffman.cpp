/*
 * Huffman.cpp
 *
 *  Created on: 1 бреб 2014
 *      Author: Daniel
 */

#include <stdio.h>
#include <assert.h>

#include "Huffman.h"
#include "macros.h"


Huffman::Huffman() :
_is_loaded(false),_freq_size(0){
	// TODO Auto-generated constructor stub

}

Huffman::~Huffman() {
	// TODO Auto-generated destructor stub
}

INode* Huffman::BuildTree()
{
    std::priority_queue<INode*, std::vector<INode*>, NodeCmp> trees;

    for (int i = 0; i < _freq_size; ++i)
    {
        if(_frequencies[i] != 0)
            trees.push(new LeafNode(_frequencies[i], (char)i));
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

HuffCode Huffman::encode(uint32_t symbol) {
	assert(_is_loaded);
	return _codes[symbol];
}

uint32_t Huffman::decode(HuffCode code) {
	assert(_is_loaded);
	HuffSymbMap::iterator it;
	it = _codes2symbols.find(code);
	if (it == _codes2symbols.end()) {
		return S_NULL;
	}
	return it->second;
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
		GenerateCodes(in->left, leftPrefix, outCodes);

		HuffCode rightPrefix = prefix;
		rightPrefix.push_back(true);
		GenerateCodes(in->right, rightPrefix, outCodes);
	}
}

void Huffman::load(int* frequencies, long size) {
	std::cout<<"Entered Huffman::load size="<<size<<std::endl;
	_freq_size = size;

	if (_frequencies!=NULL) {
		DELETE_AND_NULL(_frequencies);
	}

	_frequencies = frequencies;

	//debug print
	std::cout<<"Input frequencies for symbols:"<<std::endl;
	for (int i =0 ; i< _freq_size ; i++) {
		std::cout<<"frequencies["<<i<<"]="<<frequencies[i]<<std::endl;
	}
	INode* root = BuildTree();

	Huffman::GenerateCodes(root, HuffCode(), _codes);

	for (HuffCodeMap::const_iterator it = _codes.begin(); it != _codes.end(); ++it)
	{
		_codes2symbols.insert( std::pair<HuffCode,uint32_t>(it->second,it->first) );
	}

//	for (HuffCodeMap::const_iterator it = _codes.begin(); it != _codes.end(); ++it)
//	{
//		std::cout << it->first << " ";
//		std::copy(it->second.begin(), it->second.end(),
//				std::ostream_iterator<bool>(std::cout));
//		std::cout << std::endl;
//	}
	_is_loaded=true;
}

void Huffman::print() {
	std::cout<<"Huffman codes:"<<std::endl;
	for (HuffCodeMap::const_iterator it = _codes.begin(); it != _codes.end(); ++it)
	{
		std::cout << (uint32_t) it->first << " ";
		std::copy(it->second.begin(), it->second.end(),
				std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}

}
