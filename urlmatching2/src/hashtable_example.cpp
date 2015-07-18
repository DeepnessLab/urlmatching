/*
 * hashtable_example.cpp
 *
 *  Created on: 21 αιεπ 2015
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

void test_hashtable(CmdLineOptions& options) {
	/* Parameters in use:
	   -----------------
	options.input_urls_file_path = input url list
	options.n1 = number of hashtable entries
	options.split_for_LPM = Longest prefix match
	options.print_dicionary = should print dictionary
	*/

	using namespace std;

	PREPARE_TIMING;
	std::cout<<"######### Testing hashtable #########"<<std::endl;
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

	START_TIMING;
	bool retB = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	STOP_TIMING;
	double time_to_load = GETTIMING;
	uint32_t memory_footprint_estimation = urlc.SizeOfMemory();
	assert (retB);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}
	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	//Sanity testing - encode/decode a single string
	std::string my_string = "http://www.besound.com/pushead/home.html";
	std::cout<<"Sanity testing on \""<<my_string<<"\""<<std::endl;
	uint32_t buff_size = BUFFSIZE;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode_2(my_string,codedbuff,buff_size);
	std::string decoded_str;
	int ret = urlc.decode(decoded_str,codedbuff,buff_size);
	if (ret != STATUS_OK)
		std::cout<<"ERROR DECODING: "<<ret<<STDENDL;
	if (my_string != decoded_str) {
		std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
		std::cout<<DVAL(my_string)<<" != "<<DVAL(decoded_str)<<STDENDL;
		std::cout<<" had length "<<DVAL(codedbuff[0])<<STDENDL;
		return;
	}
	delete[] codedbuff;

	// ----   encode/decode entire urlsfile   ----

	//count urls and prepare coding buffers
	std::cout<<"Preparing: reading file and allocating memory... "<<std::endl;
	std::deque<std::string> urls;
	std::deque<uint32_t*> codedbuffers;
	uint32_t total_input_size = 0;
	buff_size = BUFFSIZE;
	for (std::deque<std::string>::iterator it = url_deque.begin(); it != url_deque.end(); ++it) {
		if ( (it->length() == 0 )||(*it == "") ) {
			std::cout<<"Skipping url in line " << urls.size() +1<<STDENDL;
		} else{
			urls.push_back(*it);
			uint32_t* codedbuff = new uint32_t[it->length()];
			codedbuffers.push_back(codedbuff);
			total_input_size += it->length();
		}
	}

	uint32_t start_at = 0;
	uint32_t howmanytocode = urls.size() ;

	std::cout<<std::endl;
	std::cout<<"-- Online Testing on " << howmanytocode << " urls --" <<STDENDL;

	//encode all urls
	std::cout<<"encoding ... "<<std::endl;
	uint32_t decoded_size = 0;
	uint32_t encoded_size = 0;

	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		buff_size = BUFFSIZE;
		uint32_t* codedbuff = codedbuffers[i];
		urlc.encode_2(urls[i],codedbuff,buff_size);
		decoded_size+=urls[i].length();
		encoded_size+=buff_size;
	}
	STOP_TIMING;
	double time_to_encode = GETTIMING;

	//Test hashtable
	std::unordered_map<Encoded,std::string,OpaqueHasher> hashtable;
	hashtable.reserve(start_at + howmanytocode);

	std::cout<<"inserting all urls to a hashtable (std::unordered_map) .. "<<std::endl;
	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		buff_size = BUFFSIZE;
		uint32_t* codedbuff = codedbuffers[i];
		Encoded opaque;
		opaque.len = codedbuff[0];
		opaque.buf = &(codedbuff[1]);
		hashtable[opaque] = urls[i];
	}
	STOP_TIMING;

	double time_to_insert = GETTIMING;


	std::cout<<"reading all urls from hashtable(std::unordered_map) and verify them.. "<<std::endl;
	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		buff_size = BUFFSIZE;
		uint32_t* codedbuff = codedbuffers[i];
		Encoded opaque;
		opaque.len = codedbuff[0];
		opaque.buf = &(codedbuff[1]);
		std::string str = hashtable[opaque];
		if (str != urls[i]) {
			std::cout<<"ERROR: Retrieved wrong string from hashtable"<<STDENDL;
			std::cout<<"  found(i="<<i<<"): " << str     <<STDENDL;
			std::cout<<"  expected: " << urls[i] <<STDENDL;
			return;
		}
	}
	STOP_TIMING;

	double time_to_verify = GETTIMING;


	//free what was never yours
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	uint32_t size = urls.size();
	uint32_t dict_size = urlc.getDictionarySize();
	uint32_t encoded_and_dict = encoded_size + dict_size;

	std::cout<<STDENDL;
	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<" ---"<<std::endl;
	std::cout<<"Runtime Statistics: for "<<size<<" urls"<<std::endl;
	std::cout<<"------------------"<<std::endl;

	std::cout<<"Loading: for "<<howmanytocode << " urls" << STDENDL;
	std::cout<<"  Time= " <<time_to_load << "s, Bandwidth= "<< double(size/time_to_load)*8/1024/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;
	std::cout<<"  Memory footprint est.="<<memory_footprint_estimation<< "Bytes = "<< double((double)memory_footprint_estimation / 1024) <<"KB"<< STDENDL;

	std::cout<<"Online hashing: on "<<howmanytocode << " urls, hashtable had " <<howmanytocode<< " buckets "<< STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "s, \t Bandwidth= "<< double(decoded_size/time_to_encode)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" Time to insert to hashtable "<<time_to_insert << "s, \t Bandwidth= "<< double(encoded_size/time_to_insert)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_insert/howmanytocode) <<"ms"<< STDENDL;
	double combined = time_to_encode + time_to_insert;
	std::cout<<" Time to encode & insert to hashtable "<<combined << "s,  Bandwidth= "<< double(decoded_size/combined)*8/1024/1024 <<" Mb/s"
				<< "  average/url="<< double((double) combined/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" Time to get from hashtable and verify "<<time_to_verify << "s, Bandwidth= "<< double(decoded_size/time_to_verify)*8/1024/1024 <<" Mb/s"
				<< "  average/url="<< double((double) time_to_verify/howmanytocode) <<"ms"<< STDENDL;

	std::cout<<"Offline compression (load & encode all urls)\n  ~ "
			<< double((double) ( (double) size/(time_to_load + ( time_to_encode * (double) size / howmanytocode) )))* 8 /1024/1024
			<<" Mb/s"<<STDENDL;

	std::cout<<" ---"<<std::endl;

	std::cout<<"Compression Statistics:"<<STDENDL;
	std::cout<<"----------------------"<<std::endl;
	std::cout<<DVAL(decoded_size)<< "Bytes = "<< double((double)decoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(encoded_size)<< "Bytes = "<< double((double)encoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(dict_size)<< "Bytes = "<< double((double)dict_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(encoded_and_dict)<< 	"Bytes = " << double((double)encoded_and_dict / 1024) <<"KB"<< STDENDL;
	std::cout<<"coding ratio (encoded_size/decoded_size) = "<< double((double)encoded_size/(double)decoded_size) * 100 << "%"<<STDENDL;
	std::cout<<"coding ratio (encoded_size+dict_size/decoded_size) = "<< double((double)(encoded_and_dict)/(double)decoded_size) * 100 << "%"<<STDENDL;
	std::cout<<" ---"<<std::endl;
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
