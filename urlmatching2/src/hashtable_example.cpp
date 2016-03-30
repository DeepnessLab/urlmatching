/*
 * hashtable_example.cpp
 *
 *  Created on: 21 ���� 2015
 *      Author: Daniel
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <assert.h>
#include <ctime>
#include <unordered_map>



#include "tester.h"
#include "UrlToolkit/Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "UrlToolkit/UrlDictionay.h"
#include "HeavyHitters/dhh_lines.h"
#include "logger.h"
#include "common.h"


#define BUFFSIZE 500

struct Opaque {
	uint32_t len;
	uint32_t* buf;

	bool operator==(const Opaque &other) const 	{
		if (this->len != other.len)
			return false;
		uint32_t* left = buf;
		uint32_t* right = other.buf;
		for (uint32_t i = 0; i <= len ; i=i+(8*sizeof(uint32_t))) {
			if (*left != *right)
				return false;
			left++;
			right++;
		}
		return true;
	}
};


typedef Opaque Encoded ;

namespace std {

template <>
struct hash<Encoded>
{
	size_t operator()(const Opaque& t) const {
		size_t ret = 0;
		uint32_t* curr = t.buf;
		for (uint32_t i = 0; i < t.len ; i=i+(8*sizeof(uint32_t))) {
			ret = ret + *curr ; // overloading
			curr++;
		}
		return ret;
	}
};

}

struct OpaqueHasher {
	size_t operator()(const Opaque& t) const {
		size_t ret = 0;
		uint32_t* curr = t.buf;
		for (uint32_t i = 0; i < t.len ; i=i+(8*sizeof(uint32_t))) {
			ret = ret + *curr ; // overloading
			curr++;
		}
		return ret;
	}
};




struct RunTimeStatsHashtable {

	RunTimeStatsHashtable() {
		num_of_urls = 0;
		mem_footprint_hashtable_strings=0;
		mem_allocated_hashtable_strings=0;
		mem_footprint_hashtable_encoded=0;
		mem_allocated_hashtable_encoded=0;

		url_compressor_allocated_memory=0;
		hashtable_strings_key_size=0;
		hashtable_encoded_key_size=0;
	}

	uint32_t num_of_urls;
	int mem_footprint_hashtable_strings;
	int mem_allocated_hashtable_strings;
	int mem_footprint_hashtable_encoded;
	int mem_allocated_hashtable_encoded;

	uint32_t url_compressor_allocated_memory;
	uint32_t hashtable_strings_key_size;
	uint32_t hashtable_encoded_key_size;

//	double time_to_load;
//	double time_to_encode;
//	double time_to_decode;
//
//	uint32_t dictionary_size;
//	uint32_t encoded_size;
//	uint32_t decoded_stream_size;
//	uint32_t encoded_stream_size;

};


void test_hashtable(CmdLineOptions& options) {
	/* Parameters in use:
	   -----------------
	options.input_urls_file_path = input url list
	options.n1 = number of hashtable entries
	options.split_for_LPM = Longest prefix match
	options.print_dicionary = should print dictionary
	*/

	using namespace std;
	RunTimeStatsHashtable rt_stats;


	std::cout<<" --- Testing hashtable ---"<<std::endl;
	options.PrintParameters(std::cout);
	HeavyHittersParams_t customParams = {/*n1*/ options.n1, /*n2*/ options.n2, /*r*/ options.r, /*kgrams_size*/ options.kgram_size};
	HeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlCompressor urlc;

	std::deque<std::string> url_deque;
	if (! urlc.getUrlsListFromFile(options.input_urls_file_path, url_deque)) {
		std::cout<<"Error with input file"<<STDENDL;
		exit (1);
	}
	if (url_deque.size() == 0) {
		std::cout<<"ERROR: read 0 urls from file"<<STDENDL;
		exit (1);
	}
	std::deque<std::string>* input_for_urlcompressor = &url_deque;

	if (options.split_for_LPM) {
		std::deque<std::string>* splitted_deque = new std::deque<std::string>;
		urlc.SplitUrlsList(url_deque, *splitted_deque, options.LPM_delimiter);
		input_for_urlcompressor = splitted_deque;
	}

	bool retB = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	uint32_t memory_footprint_estimation = urlc.SizeOfTotalAllocatedMemory();
	assert (retB);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}
	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	sanityTesting(urlc, false);

	// ----   encode/decode entire urlsfile   ----

	//count urls and prepare coding buffers
	std::cout<<"Preparing: reading file and allocating memory... "<<std::endl;
	std::deque<std::string>& urls = url_deque;
	std::deque<uint32_t*> codedbuffers;
	uint32_t total_input_size = 0;
	uint32_t buff_size = BUFFSIZE;
	for (std::deque<std::string>::iterator it = url_deque.begin(); it != url_deque.end(); ++it) {
		if ( (it->length() == 0 )||(*it == "") ) {
			std::cout<<"Skipping url in line " << urls.size() +1<<STDENDL;
		} else{
			uint32_t* codedbuff = new uint32_t[it->length()];
			codedbuffers.push_back(codedbuff);
			total_input_size += it->length();
		}
	}

	uint32_t howmanytocode = urls.size() ;
	rt_stats.num_of_urls = urls.size() ;

	std::cout<<std::endl;
	std::cout<<"-- Online Testing on " << howmanytocode << " urls --" <<STDENDL;

	//encode all urls
	std::cout<<"encoding ... "<<std::endl;
	uint32_t decoded_size = 0;
	uint32_t encoded_size = 0;

	for (uint32_t i = 0 ; i < howmanytocode; i++ ) {
		buff_size = BUFFSIZE;
		uint32_t* codedbuff = codedbuffers[i];
		urlc.encode_2(urls[i],codedbuff,buff_size);
		decoded_size+=urls[i].length();
		encoded_size+=buff_size * sizeof(uint32_t);
	}


	//Simulate Hashtable <url, IPv4 (uint32_t)>
	uint32_t myIPv4=0xEEEE;

	// Test hashtable using simple strings
	rt_stats.mem_footprint_hashtable_strings = get_curr_memsize();
	std::unordered_map<std::string,uint32_t> hashtable_string;
	hashtable_string.reserve( howmanytocode);
	std::cout<<"inserting all urls to a hashtable_string (std::unordered_map) .. "<<std::endl;
	for (uint32_t i = 0 ; i < howmanytocode; i++ ) {
		myIPv4=i;
		std::string str (urls[i]);	//new allocation
		rt_stats.hashtable_strings_key_size += str.length() + 1 /*'\0'*/ + sizeof(str);
		hashtable_string[str] = myIPv4;
	}
	rt_stats.mem_allocated_hashtable_strings+=rt_stats.hashtable_strings_key_size;
	rt_stats.mem_allocated_hashtable_strings+=hashtable_string.size() * sizeof(myIPv4);
	rt_stats.mem_footprint_hashtable_strings = get_curr_memsize() - rt_stats.mem_footprint_hashtable_strings;
	std::cout<<"hashtable_string.size() = " << hashtable_string.size() <<std::endl;

	// Test hashtable using encoded buffer enclosed by Opaque
	rt_stats.mem_footprint_hashtable_encoded = get_curr_memsize();
	std::unordered_map<Encoded,uint32_t,OpaqueHasher> hashtable_encoded;
	hashtable_encoded.reserve( howmanytocode);

	std::cout<<"inserting all urls to a hashtable_encoded (std::unordered_map) .. "<<std::endl;
	for (uint32_t i = 0 ; i <  howmanytocode; i++ ) {
		myIPv4=i;
		uint32_t* codedbuff = codedbuffers[i];
		Encoded my_opaque;
		my_opaque.len = codedbuff[0];
		uint32_t u32_len = conv_bits_to_uin32_size(my_opaque.len);
		my_opaque.buf = new uint32_t[u32_len]; 	//new allocation
		memcpy(my_opaque.buf, &(codedbuff[1]), u32_len * sizeof(uint32_t) );
//		rt_stats.hashtable_encoded_key_size += sizeof(Encoded)+u32_len;
		rt_stats.hashtable_encoded_key_size += sizeof(Encoded)+conv_bits_to_bytes(my_opaque.len);
		hashtable_encoded[my_opaque] = myIPv4;
	}
	rt_stats.mem_allocated_hashtable_encoded+= rt_stats.hashtable_encoded_key_size;
	rt_stats.mem_allocated_hashtable_encoded+=hashtable_encoded.size() * sizeof(myIPv4);
	rt_stats.mem_footprint_hashtable_encoded = get_curr_memsize() - rt_stats.mem_footprint_hashtable_encoded;
	std::cout<<"hashtable_encoded.size() = " << hashtable_encoded.size() <<std::endl;

	if (options.test_decoding) {
		std::cout<<"reading all urls from hashtable(std::unordered_map) and verify them.. "<<std::endl;
		for (auto it = hashtable_encoded.begin(); it != hashtable_encoded.end(); ++it) {
			myIPv4 = it->second;
			uint32_t idx = myIPv4;
			Encoded opaque = it->first;

			uint32_t* codedbuff = codedbuffers[idx];
			codedbuff[0] = opaque.len;
			memcpy(&(codedbuff[1]), opaque.buf, conv_bits_to_bytes(opaque.len) * sizeof(uint32_t));

			std::string decoded_str;
			urlc.decode(decoded_str,codedbuff,urls[idx].length());

			if (decoded_str != urls[idx]) {
				std::cout<<"ERROR: Retrieved wrong string from hashtable"<<STDENDL;
				std::cout<<"  found(i="<<idx<<"): " << decoded_str     <<STDENDL;
				std::cout<<"  expected: " << urls[idx] <<STDENDL;
				std::cout<<"  had bit length "<<DVAL(opaque.len)<<STDENDL;
				return;
			}
		}

	}


	//free what was never yours
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	uint32_t size = urls.size();

	std::cout<<STDENDL;
	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<" ---"<<std::endl;
	std::cout<<"Runtime Statistics: for "<<size<<" urls"<<std::endl;
	std::cout<<"------------------"<<std::endl;
	std::cout<<" URL compressor allocated memory = "<<Byte2KB(memory_footprint_estimation)<<"KB"<<std::endl;
	std::cout<<"Memory Allocated:"<<std::endl;
	std::cout<<" hashtable strings = "<<Byte2KB(rt_stats.mem_allocated_hashtable_strings)<<"KB"<<std::endl;
	std::cout<<" hashtable encoded = "<<Byte2KB(rt_stats.mem_allocated_hashtable_encoded)<<"KB"<<std::endl;
	std::cout<<" Ratio = "<<double ( (double) rt_stats.mem_allocated_hashtable_encoded / (double)rt_stats.mem_allocated_hashtable_strings)<<std::endl;

	std::cout<<"Memory footprint:"<<std::endl;
	std::cout<<" hashtable strings = "<<Byte2KB(rt_stats.mem_footprint_hashtable_strings)<<"KB"<<std::endl;
	std::cout<<" hashtable encoded = "<<Byte2KB(rt_stats.mem_footprint_hashtable_encoded)<<"KB"<<std::endl;
	std::cout<<" Ratio = "<<double ( (double) rt_stats.mem_footprint_hashtable_encoded / (double) rt_stats.mem_footprint_hashtable_strings)<<std::endl;

	std::cout<<"Memory keysize comparison:"<<std::endl;
	std::cout<<" hashtable strings = "<<Byte2KB(rt_stats.hashtable_strings_key_size)<<"KB"<<std::endl;
	std::cout<<" hashtable encoded = "<<Byte2KB(rt_stats.hashtable_encoded_key_size)<<"KB"<<std::endl;
	std::cout<<" Ratio = "<<double ( (double) rt_stats.hashtable_encoded_key_size / (double) rt_stats.hashtable_strings_key_size)<<std::endl;

	std::cout<<"------------------"<<std::endl;
	std::cout<<"Algorithm Statistics:"<<STDENDL;
	std::cout<<"--------------------"<<std::endl;
	const UrlCompressorStats* stats = urlc.get_stats();
	stats->print(std::cout);

	if (options.print_dicionary) {
		ofstream printout_file;
		printout_file.open (options.print_dicionary_file.c_str(),std::ofstream::out );
		urlc.print_database(printout_file);
		printout_file.close();
		std::cout <<std::endl<< "Dicionary outputed to: "<<options.print_dicionary_file<<std::endl;
	}
}
