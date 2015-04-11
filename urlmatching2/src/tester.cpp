/*
 * tester.cpp
 *
 *  Created on: 18 бреб 2014
 *      Author: Daniel
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <assert.h>

#include "tester.h"
#include "Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "UrlDictionay.h"


void test_url_matching() {

//	UrlCompressor dict;
//	build_simple_dict(&dict);

	std::string patterns_file = "d:\\Temp\\project\\patterns_abc.txt";
	std::string my_string = "shaloma";

	UrlCompressor urlc;
	bool ret = urlc.initFromStoredDBFile(patterns_file);
	assert (ret);
	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl;

	std::cout<<"matching string="<<my_string<<std::endl;

	symbolT result[1000];
	urlc.algo.find_patterns(my_string,result);
//	ACWrapperClassic aho=ACWrapperClassic::getInstance();



}

void test_aho_compressed() {

//	UrlCompressor dict;
//	build_simple_dict(&dict);

	std::string patterns_file = "d:\\Temp\\project\\patterns_only.txt";
	std::string my_string = "shaloma";

	ACWrapperCompressed ac;
	ac.load_patterns(patterns_file);
	std::cout<<"matching string="<<my_string<<std::endl;
//	std::cout.flush();
	symbolT result[100];
	ac.find_patterns(my_string,result);
	fflush(stdout);
	std::cout<<std::endl<<"finished matching"<<std::endl;

}


void build_simple_dict(UrlCompressor* dict) {
	std::map<std::string,int> strings_and_freqs;
	int i=1;
	for (char c='a'; c<='z'; c++) {
		std::string str (1,c);
		strings_and_freqs[str]=i;
		i++;
	}
	dict->load_strings_and_freqs(&strings_and_freqs);
	dict->_huffman.print();
	dict->print_strings_and_codes();
}


void test_huffman() {
	// Build frequency table
		const char* SampleString = "this is an example for huffman encoding";
	//	const char* SampleString = "aaaabbcd";
		std::map<std::string,int> strings_and_freqs;

		for (uint32_t i=0;i<strlen((char*) SampleString);i++) {
			std::string str (1,SampleString[i]);
			int freq = strings_and_freqs[str];
	//		std::cout<<str<<", freq="<<freq<<std::endl;
			strings_and_freqs[str]=freq+1;
		}
		std::cout<<std::endl;

		UrlCompressor dict;
		dict.load_strings_and_freqs(&strings_and_freqs);

		dict._huffman.print();

		dict.print_strings_and_codes();

	//
	//	int frequencies[UniqueSymbols] = {0};
	//	const char* ptr = SampleString;
	//	while (*ptr != '\0')
	//		++frequencies[*ptr++];
	//
	//	INode* root = Huffman::BuildTree(frequencies);
	//
	//	HuffCodeMap codes;
	//	Huffman::GenerateCodes(root, HuffCode(), codes);
	//	delete root;
	//
	//	for (HuffCodeMap::const_iterator it = codes.begin(); it != codes.end(); ++it)
	//	{
	//		std::cout << it->first << " ";
	//		std::copy(it->second.begin(), it->second.end(),
	//				std::ostream_iterator<bool>(std::cout));
	//		std::cout << std::endl;
	//	}
}
