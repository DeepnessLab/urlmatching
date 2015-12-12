/*
 * tester.cpp
 *
 *  Created on: 18 December 2014
 *      Author: Daniel Krauthgamer
 *
 *  Tester implementation for the module
 */
#define _GLIBCXX_USE_C99 1

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <assert.h>
#include <ctime>
#include <unordered_map>
#include <cstdlib>

#include "tester.h"
#include "../UrlToolkit/Huffman.h"
#include "../UrlToolkit/UrlMatching.h"
#include "../UrlToolkit/FileCompressor.h"
#include "../HeavyHitters/dhh_lines.h"
#include "../logger.h"
#include "../common.h"


#define BUFFSIZE 500

#ifdef DVAL
#undef DVAL
#endif
#define DVAL(what) #what" = "<< (what)

void run_cmd_main(CmdLineOptions& options) {
	if (options.cmd == CMD_FULLTEST) {
		test_main(options);
	} else if (options.cmd == CMD_BUILDDIC) {
		test_build_dictionary_to_file(options);
	} else if (options.cmd == CMD_ENCODE) {
		test_encode(options);
	} else if (options.cmd == CMD_HASHTABLE) {
		test_hashtable(options);
	} else if (options.cmd == CMD_COMPRESS) {
		test_compress(options);
	} else if (options.cmd == CMD_EXTRACT) {
		test_extract(options);
	} else if (options.cmd == CMD_ARTICLE) {
		test_article(options);
	} else {

		options.usage();

	}
}


struct RunTimeStats {
	uint32_t num_of_urls;
	double time_to_load;
	double time_to_encode;
	double time_to_decode;
	uint32_t dictionary_size;
	int mem_footprint_est;
	uint32_t url_compressor_allocated_memory;
	uint32_t decoded_size;
	uint32_t encoded_size;
	uint32_t decoded_stream_size;
	uint32_t encoded_stream_size;

};


void printRunTimeStats(CmdLineOptions& options, RunTimeStats& stats, bool print_offline_compression);
void printCompressionStats(CmdLineOptions& options, RunTimeStats& s) ;
void printAlgorithmStats(CmdLineOptions& options, const UrlMatchingModuleStats* stats );
void createOptionalDictionaryFile(CmdLineOptions& options, UrlMatchingModule& urlc);
void createOptionalDumpACStatesFile(CmdLineOptions& options, UrlMatchingModule& urlc);

void createOptionalOutputFile(CmdLineOptions& options, RunTimeStats& rt_stat , const UrlMatchingModuleStats* stats );


void test_article(CmdLineOptions& options)
{
	using namespace std;
	std::cout<<" --- Test for article mode ---"<<std::endl;
	PREPARE_TIMING;

	RunTimeStats s;

	//step 1: create dicionary file
	//-------
	options.PrintParameters(std::cout);
	//	std::cout<<"urls file path="<<options.input_urls_file_path<<std::endl;
	DoubleHeavyHittersParams_t customParams = {/*n1*/ options.n1, /*n2*/ options.n2, /*r*/ options.r, /*kgrams_size*/ options.kgram_size};
	DoubleHeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlMatchingModule* urlc = new UrlMatchingModule();

	std::deque<std::string> url_deque;
	if (! urlc->getUrlsListFromFile(options.input_urls_file_path, url_deque)) {
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
		urlc->SplitUrlsList(url_deque, *splitted_deque, options.LPM_delimiter);
		input_for_urlcompressor = splitted_deque;
	}

	take_a_break(options.break_time," before creating dicionary");
	START_TIMING;
	bool ret = urlc->InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	STOP_TIMING;
	take_a_break(options.break_time," after creating dicionary (still in memory)");
	s.time_to_load = GETTIMING;
	assert (ret);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}

	if (!options.use_dictionary_file) {
		options.dictionary_file= options.input_urls_file_path + ".dict";
	}

	sanityTesting(*urlc);

	std::string dictionary_filename = options.getDictionaryFilename();
	std::cout<<"Storing to file: "<< dictionary_filename <<std::endl;

	ret = urlc->StoreDictToFile(dictionary_filename );
	if (!ret) {
		std::cout<<"Faild to store to " << dictionary_filename <<std::endl;
		return;
	}
	delete urlc;

	//step 2: tkae offline and online measurements
	//-------
	urlc = new UrlMatchingModule();

	take_a_break(options.break_time," before loading");
	s.mem_footprint_est = get_curr_memsize();
	START_TIMING;
	ret = urlc->InitFromDictFile(dictionary_filename,true);
	STOP_TIMING;
	s.mem_footprint_est = get_curr_memsize() - s.mem_footprint_est;
	s.url_compressor_allocated_memory = urlc->SizeOfTotalAllocatedMemory();
	take_a_break(options.break_time," after loading");
	assert (ret);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	std::cout<<"Preparing buffers for encoding .. "<<std::endl;
	//count urls and prepare coding buffers
	std::deque<std::string> urls;
	std::deque<uint32_t*> codedbuffers;
	for (std::deque<std::string>::iterator it = url_deque.begin(); it != url_deque.end(); ++it) {
		if ( (it->length() == 0 )||(*it == "") ) {
			std::cout<<"Skipping empty url in line " << urls.size() +1<<STDENDL;
		} else{
			urls.push_back(*it);
			uint32_t* codedbuff = new uint32_t[it->length()];
			codedbuffers.push_back(codedbuff);
		}
	}
	uint32_t urls_size = urls.size();


	if (options.factor == 1) {
		std::cout<<"-- Offline Testing  -- "<<STDENDL;
		uint64_t encoded_size_bits = 0;
		s.decoded_size = 0;
		for (uint32_t i = 0 ; i < urls_size; i++ ) {
			uint32_t* codedbuff = codedbuffers[i];
			uint32_t buffsize = (uint32_t) urls[i].length();
			urlc->encode(urls[i], codedbuff, buffsize);
			s.decoded_size+=urls[i].length() + 1 /* for \n at the end of the original line */;
			encoded_size_bits += codedbuff[0] ;
		}
		s.encoded_size = encoded_size_bits/ (8);
		s.encoded_size = (encoded_size_bits % (8) == 0)? s.encoded_size : s.encoded_size + 1;
		std::cout<<"Done. "<<STDENDL;
	} else {
		std::cout<<"-- NO Offline Testing - ignore ratio -- "<<STDENDL;
		s.encoded_size = 1;
		s.decoded_size = 1;
	}

	std::cout<<"-- Online Testing  -- "<<STDENDL;
	uint32_t times = 20;
	uint32_t num_of_sets = 10;
	uint32_t set_size = 10000;
	std::cout<<"Encoding: "<<STDENDL;
	std::cout<<"   Number of sets = "<< num_of_sets<<STDENDL;
	std::cout<<"    | Number of urls in a set = "<< set_size<<STDENDL;
	std::cout<<"       | Times = "<< times<<STDENDL;

	uint16_t* set = new uint16_t[set_size];
	std::srand(std::time(0)); 						// use current time as seed for random generator
	s.decoded_stream_size = 0;
	s.time_to_encode = 0;
	uint32_t encoded_stream_bitsize = 0;
	for (uint32_t set_num = 1 ; set_num <= num_of_sets; set_num ++)
	{
		//create random set
		for (uint32_t n =0 ; n < set_size ; n++) {
			uint32_t idx = (uint32_t) std::rand();
			idx = idx % urls_size;
			set[n] = idx;
		}
		uint32_t buff_size = BUFFSIZE;
		START_TIMING;
		for (uint32_t n = 0 ; n < set_size; n++ ) {
			uint32_t idx = set[n];
			uint32_t* codedbuff = codedbuffers[idx];
			for (uint32_t t = 1 ; t <= times ; t ++) {
				buff_size = BUFFSIZE;
				urlc->encode(urls[idx],codedbuff,buff_size);
				s.decoded_stream_size+=urls[idx].length();
				encoded_stream_bitsize += codedbuff[0] ;
			}
		}
		STOP_TIMING;
		s.time_to_encode += GETTIMING;
	}
	std::cout<<"  passed  "<< num_of_sets * set_size *times << " urls"<<STDENDL;

	if (options.test_decoding) {
		//decode all urls
		std::cout<<"verify correct coding by decoding last set ... ";
		START_TIMING;
		for (uint32_t n = 0; n < set_size; n++ ) {
			uint32_t idx = set[n];
			uint32_t buff_size = BUFFSIZE;
			uint32_t* codedbuff = codedbuffers[idx];
			std::string decoded_str;
			urlc->decode(decoded_str,codedbuff,buff_size);
			if (decoded_str != urls[idx]) {
				std::cout<<std::endl;
				std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
				std::cout<<"  " << DVAL(idx)<< " "<< DVAL(urls[idx])<<" != "<<DVAL(decoded_str)<<STDENDL;
				std::cout<<"  had length "<<DVAL(codedbuff[0])<<STDENDL;
				return;
			}
		}
		STOP_TIMING;
		std::cout<<"100%  "<<STDENDL;
	}
	s.time_to_decode = (options.test_decoding) ? GETTIMING: 0l;


	//free what was never yours
	delete[] set;
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	//prepare results for print and output
	s.num_of_urls = num_of_sets * set_size *times;
	s.encoded_stream_size = encoded_stream_bitsize/ (8);
	s.encoded_stream_size = (encoded_stream_bitsize % (8) == 0)? s.encoded_stream_size : s.encoded_stream_size + 1;

	s.dictionary_size= urlc->getDictionarySize();

	// print results
	std::cout <<"--------------------"<<std::endl;
	printRunTimeStats(options,s,false);
	printCompressionStats(options,s);
	const UrlMatchingModuleStats* urlc_stats = urlc->get_stats();
	printAlgorithmStats(options,urlc_stats);

	// create output files
	createOptionalDictionaryFile(options,*urlc);
	createOptionalDumpACStatesFile(options,*urlc);
	createOptionalOutputFile(options,s,urlc_stats);

	delete urlc;
}

void test_encode(CmdLineOptions& options) {
	using namespace std;
	RunTimeStats s;

	PREPARE_TIMING;
	std::cout<<" --- Encode mode ---"<<std::endl;
	//load from stored DB file
	std::string dictionary_filename = options.getDictionaryFilename();

	std::cout<< "Using dictionary from: " << dictionary_filename << STDENDL;
	UrlMatchingModule urlc;

	take_a_break(options.break_time," before loading");
	s.mem_footprint_est = get_curr_memsize();
	START_TIMING;
	bool ret = urlc.InitFromDictFile(dictionary_filename);
	STOP_TIMING;
	s.mem_footprint_est = get_curr_memsize() - s.mem_footprint_est;
	s.url_compressor_allocated_memory = urlc.SizeOfTotalAllocatedMemory();
	take_a_break(options.break_time," after loading");
	assert (ret);

	s.time_to_load = GETTIMING;
	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	sanityTesting(urlc);

	// ----   encode/decode entire urlsfile   ----

	//Read input_file into deque of urls
	std::cout<<"Preparing: reading input file .. "<<std::endl;
	std::deque<std::string> url_deque;
	if (! urlc.getUrlsListFromFile(options.input_urls_file_path, url_deque)) {
		std::cout<<"Error with input file"<<STDENDL;
		exit (1);
	}
	if (url_deque.size() == 0) {
		std::cout<<"ERROR: read 0 urls from file"<<STDENDL;
		exit (1);
	}

	//count urls and prepare coding buffers
	std::deque<std::string> urls;
	std::deque<uint32_t*> codedbuffers;
	uint32_t total_input_size = 0;
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

	s.num_of_urls = urls.size() ;
	uint32_t& num_of_urls = s.num_of_urls;

	uint32_t status_every = ((num_of_urls /10) > 0 ) ? (num_of_urls /10) : 1;

	std::cout<<std::endl;
	std::cout<<"-- Online Testing on " << num_of_urls << " urls --" <<STDENDL;

	//encode all urls
	std::cout<<"encoding ... "<<std::endl;

	s.decoded_size = 0;
	s.decoded_stream_size = 0;

	s.encoded_size = 0;
	uint64_t encoded_size_bits = 0;
	uint64_t encoded_stream_bitsize = 0;

	uint32_t buff_size = BUFFSIZE;
	START_TIMING;
	for (uint32_t i = 0 ; i < num_of_urls; i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		for (int j=0; j < options.factor; j++) {
			buff_size = BUFFSIZE;
			urlc.encode(urls[i],codedbuff,buff_size);
			s.decoded_stream_size+=urls[i].length();
			encoded_stream_bitsize += codedbuff[0] ;
		}
	}
	STOP_TIMING;
	s.time_to_encode = GETTIMING;

	//calculate decoded and encoded size
	for (uint32_t i = 0 ; i < num_of_urls; i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		s.decoded_size += urls[i].length()+1;
		encoded_size_bits += codedbuff[0] ;
	}


	if (options.test_decoding) {
		//decode all urls
		std::cout<<"decoding ... \r";
		START_TIMING;
		for (uint32_t i = 0; i < num_of_urls; i++ ) {
			buff_size = BUFFSIZE;
			uint32_t* codedbuff = codedbuffers[i];
			std::string decoded_str;
			urlc.decode(decoded_str,codedbuff,buff_size);
			if (decoded_str != urls[i]) {
				std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
				std::cout<<"  " << DVAL(i)<< " "<< DVAL(urls[i])<<" != "<<DVAL(decoded_str)<<STDENDL;
				std::cout<<"  had length "<<DVAL(codedbuff[0])<<STDENDL;
				return;
			}
			if (i%status_every == 0)
				std::cout<<"decoding ... "<<(100*(i+1))/num_of_urls<<"%\r";
		}
		STOP_TIMING;
		std::cout<<"decoding ... 100%"<<std::endl;
	}
	s.time_to_decode = (options.test_decoding) ? GETTIMING: 0l;

	//free what was never yours
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	//prepare results for print and output
	s.encoded_stream_size = encoded_stream_bitsize/ (8);
	s.encoded_stream_size = (encoded_stream_bitsize % (8) == 0)? s.encoded_stream_size : s.encoded_stream_size + 1;
	s.encoded_size = encoded_size_bits/ (8);
	s.encoded_size = (encoded_size_bits % (8) == 0)? s.encoded_size : s.encoded_size + 1;
	s.dictionary_size= urlc.getDictionarySize();

	// print results
	std::cout <<"--------------------"<<std::endl;
	printRunTimeStats(options,s,false);
	printCompressionStats(options,s);
	const UrlMatchingModuleStats* urlc_stats = urlc.get_stats();
	printAlgorithmStats(options,urlc_stats);

	// create output files
	createOptionalDictionaryFile(options,urlc);
	createOptionalDumpACStatesFile(options,urlc);
	createOptionalOutputFile(options,s,urlc_stats);


}

void test_build_dictionary_to_file(CmdLineOptions& options) {
	using namespace std;
	std::cout<<" --- Build dictionary mode ---"<<std::endl;

	PREPARE_TIMING;
	options.PrintParameters(std::cout);
	//	std::cout<<"urls file path="<<options.input_urls_file_path<<std::endl;
	DoubleHeavyHittersParams_t customParams = {/*n1*/ options.n1, /*n2*/ options.n2, /*r*/ options.r, /*kgrams_size*/ options.kgram_size};
	DoubleHeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlMatchingModule urlc;

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

	uint32_t num_of_urls = input_for_urlcompressor->size();
	uint32_t decoded_size = 0;
	for (std::deque<std::string>::iterator it=input_for_urlcompressor->begin();
			it != input_for_urlcompressor->end(); ++it) {
		decoded_size += it->size();
	}

	take_a_break(options.break_time," before loading");
	uint32_t mem_footprint = get_curr_memsize();
	START_TIMING;
	bool retB = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	STOP_TIMING;
	mem_footprint = get_curr_memsize() - mem_footprint;
	take_a_break(options.break_time," after loading");
	double time_to_load = GETTIMING;
	assert (retB);


	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}


	if (!options.use_dictionary_file) {
		options.dictionary_file= options.input_urls_file_path + ".dict";
	}

	std::string dictionary_filename = options.getDictionaryFilename();
	std::cout<<" storing to file: "<< dictionary_filename <<std::endl;

	retB = urlc.StoreDictToFile(dictionary_filename );
	if (!retB) {
		std::cout<<"Faild to store to " << dictionary_filename <<std::endl;
		return;
	}

	std::cout<<STDENDL;
	//printing stats
	// remember 1 B/ms == 1KB / sec
	uint32_t dict_size = urlc.getDictionarySize();

	std::cout<<"------------------"<<std::endl;
	std::cout<<"Runtime Statistics: for "<<num_of_urls<<" urls"<<std::endl;
	std::cout<<"------------------"<<std::endl;
	std::cout<<"Loading: for "<<num_of_urls << " urls" << STDENDL;
	std::cout<<"  Time = " <<time_to_load << "s,  Throughput= "<< double(decoded_size/time_to_load)*8/1024/1024  <<" Mb/s" << STDENDL;
	std::cout<<"  Memory footprint (linux only) ~ "<<mem_footprint<< "Bytes = "<< double((double)mem_footprint / 1024) <<"KB"<< STDENDL;
	std::cout<<"  UrlCompressor internal memory ~ "<<urlc.SizeOfTotalAllocatedMemory()<< "Bytes = "<< double((double)urlc.SizeOfTotalAllocatedMemory()/ 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(dict_size)<< " Bytes = "<< Byte2KB(dict_size)<<"KB"<< STDENDL;
	printAlgorithmStats(options,urlc.get_stats());
	std::cout<<"------------------"<<std::endl;

	createOptionalDictionaryFile(options,urlc);
	createOptionalDumpACStatesFile(options,urlc);

	return;
}

void test_main(CmdLineOptions& options) {
	using namespace std;
	RunTimeStats s;

	PREPARE_TIMING;
	std::cout<<" --- Test mode ---"<<std::endl;
	options.PrintParameters(std::cout);
//	std::cout<<"urls file path="<<options.input_urls_file_path<<std::endl;
	DoubleHeavyHittersParams_t customParams = {/*n1*/ options.n1, /*n2*/ options.n2, /*r*/ options.r, /*kgrams_size*/ options.kgram_size};
	DoubleHeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlMatchingModule urlc;

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

	take_a_break(options.break_time," before loading");
	s.mem_footprint_est = get_curr_memsize();
	START_TIMING;
	bool ret = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false, true);
	STOP_TIMING;
	s.mem_footprint_est = get_curr_memsize() - s.mem_footprint_est;
	s.url_compressor_allocated_memory = urlc.SizeOfTotalAllocatedMemory();
	take_a_break(options.break_time," after loading");
	s.time_to_load = GETTIMING;
	assert (ret);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	sanityTesting(urlc);

		// ----   encode/decode entire urlsfile   ----
	//count urls and prepare coding buffers
	std::cout<<"Preparing: reading file and allocating memory... "<<std::endl;
	std::deque<std::string> urls;
	std::deque<uint32_t*> codedbuffers;
	uint32_t buff_size = BUFFSIZE;
	for (std::deque<std::string>::iterator it = url_deque.begin(); it != url_deque.end(); ++it) {
		if ( (it->length() == 0 )||(*it == "") ) {
			std::cout<<"Skipping url in line " << urls.size() +1<<STDENDL;
		} else{
			urls.push_back(*it);
			uint32_t* codedbuff = new uint32_t[it->length()];
			codedbuffers.push_back(codedbuff);
		}
	}

	s.num_of_urls = urls.size() ;

	uint32_t start_at = 0;
	uint32_t howmanytocode = urls.size() ;
	uint32_t status_every = ((howmanytocode /10) > 0 ) ? (howmanytocode /10) : 1;

	std::cout<<std::endl;
	std::cout<<"-- Online Testing on " << howmanytocode << " urls --" <<STDENDL;

	//encode all urls
	std::cout<<"encoding ... "<<std::endl;
	s.decoded_size = 0;
	s.decoded_stream_size = 0;

	s.encoded_size = 0;
	uint64_t encoded_size_bits = 0;
	uint32_t encoded_stream_bitsize = 0;

	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		for (int j=0; j < options.factor; j++) {
			buff_size = BUFFSIZE;
			urlc.encode(urls[i],codedbuff,buff_size);
			s.decoded_stream_size+=urls[i].length();
			encoded_stream_bitsize += codedbuff[0] ;
		}
	}
	STOP_TIMING;
	s.time_to_encode = GETTIMING;

	//calculate decoded and encoded size
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		s.decoded_size += urls[i].length()+1;
		encoded_size_bits += codedbuff[0] ;
	}

	if (options.test_decoding) {
		//decode all urls
		std::cout<<"decoding ... \r";
		START_TIMING;
		for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
			buff_size = BUFFSIZE;
			uint32_t* codedbuff = codedbuffers[i];
			std::string decoded_str;
			urlc.decode(decoded_str,codedbuff,buff_size);
			if (decoded_str != urls[i]) {
				std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
				std::cout<<"  " << DVAL(i)<< " "<< DVAL(urls[i])<<" != "<<DVAL(decoded_str)<<STDENDL;
				std::cout<<"  had length "<<DVAL(codedbuff[0])<<STDENDL;
				return;
			}
			if (i%status_every == 0)
				std::cout<<"decoding ... "<<(100*(i+1))/(start_at + howmanytocode)<<"%\r";
		}
		STOP_TIMING;
		std::cout<<"decoding ... 100%"<<std::endl;
	}
	s.time_to_decode = (options.test_decoding) ? GETTIMING: 0l;

	//free what was never yours
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	//prepare results for print and output
	s.encoded_stream_size = encoded_stream_bitsize/ (8);
	s.encoded_stream_size = (encoded_stream_bitsize % (8) == 0)? s.encoded_stream_size : s.encoded_stream_size + 1;
	s.encoded_size = encoded_size_bits/ (8);
	s.encoded_size = (encoded_size_bits % (8) == 0)? s.encoded_size : s.encoded_size + 1;
	s.dictionary_size= urlc.getDictionarySize();

	// print results
	std::cout <<"--------------------"<<std::endl;
	printRunTimeStats(options,s,true);
	printCompressionStats(options,s);
	const UrlMatchingModuleStats* urlc_stats = urlc.get_stats();
	printAlgorithmStats(options,urlc_stats);

	// create output files
	createOptionalDictionaryFile(options,urlc);
	createOptionalDumpACStatesFile(options,urlc);
	createOptionalOutputFile(options,s,urlc_stats);
}


void test_compress (CmdLineOptions& options) {
	using namespace std;
	std::cout<<" --- Compress file mode ---"<<std::endl;

	options.PrintParameters(std::cout);
	std::cout<<std::endl<<"Compresing file: "<<options.input_urls_file_path<<std::endl;

	if (!options.custom_output_file)
		options.output_file_path=options.input_urls_file_path + ".compress";

	PREPARE_TIMING;
	START_TIMING;
	FileCompressor::compress(options.input_urls_file_path,options.output_file_path);
	STOP_TIMING;
	double time_to_compress = GETTIMING;

	std::cout<<std::endl<<"Compresed file: "<<options.output_file_path<<std::endl;
	std::cout<<"------------------"<<std::endl;
	std::cout<<"Time = " <<time_to_compress << STDENDL;

}

void test_extract (CmdLineOptions& options) {
	using namespace std;
	std::cout<<" --- Extracting file mode ---"<<std::endl;

	options.PrintParameters(std::cout);
	std::cout<<std::endl<<"Extracting file: "<<options.input_urls_file_path<<std::endl;

	if (!options.custom_output_file)
		options.output_file_path=options.input_urls_file_path + ".extract";

	PREPARE_TIMING;
	START_TIMING;
	FileCompressor::extract(options.input_urls_file_path,options.output_file_path);
	STOP_TIMING;
	double time_to_extract = GETTIMING;

	std::cout<<std::endl<<"Extracted file: "<<options.output_file_path<<std::endl;
	std::cout<<"------------------"<<std::endl;
	std::cout<<"Time = " <<time_to_extract << STDENDL;
}


void test_url_dictionary_load_from_url_txt_file() {
	using namespace std;

	PREPARE_TIMING;

	std::string path;
	getWorkingDirectory(path);
	std::cout<<"running from path="<<path<<std::endl;

//	std::string urls_file = "test_files/9000_urls_100times.txt";
//	std::string urls_file = "test_files/9000_urls.txt";
	std::string urls_file = "test_files/blacklist_syn.txt";
//	std::string urls_file = "test_files/bigfile_syn.txt";
	path = path + urls_file;

	std::cout<<"test file path="<<path<<std::endl;
	int n = 1000;
	DoubleHeavyHittersParams_t customParams = {/*n1*/ n, /*n2*/ n, /*r*/ 0.8, /*kgrams_size*/ 8};
	DoubleHeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlMatchingModule urlc;
	int break_time=0;
	take_a_break(break_time," before loading");
	std::deque<std::string> url_deque;
	urlc.getUrlsListFromFile(urls_file, url_deque);
	std::deque<std::string> splitted_deque;
	urlc.SplitUrlsList(url_deque, splitted_deque, "/");
	START_TIMING;
//	bool retB = urlc.LoadUrlsFromFile(urls_file, params, false);
	bool retB = urlc.InitFromUrlsList(url_deque, splitted_deque, params, false);
	STOP_TIMING;
	take_a_break(break_time," after loading");
	double time_to_load = GETTIMING;
	uint32_t memory_footprint_estimation = urlc.SizeOfTotalAllocatedMemory();
	assert (retB);

//	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	//static test - encode/decode single string
//	std::string my_string = "http://www.google.com/gmail/drive";
	std::string my_string = "http://www.besound.com/pushead/home.html";
	std::cout<<"matching string="<<my_string<<std::endl;

	std::cout<<"encode string= "<<my_string<<std::endl;
	uint32_t buff_size = BUFFSIZE;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode(my_string,codedbuff,buff_size);
	std::cout<<"encoding length= "<<codedbuff[0]<<" "<<DVAL(buff_size)<< std::endl;

	std::string decoded_str;
	int ret = urlc.decode(decoded_str,codedbuff,buff_size);
	if (ret != STATUS_OK)
		std::cout<<"ERROR DECODING: "<<ret<<STDENDL;
	std::cout<<"dencoded string="<<decoded_str<<std::endl;
	if (my_string != decoded_str) {
		std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
		std::cout<<DVAL(my_string)<<" != "<<DVAL(decoded_str)<<STDENDL;
		std::cout<<" had length "<<DVAL(codedbuff[0])<<STDENDL;
		return;
	}

	delete[] codedbuff;



	//encode/decode entire urlsfile
	//count urls and prepare coding buffers
	std::cout<<"reading file and allocating memory... "<<std::endl;
	std::deque<std::string> urls;
	std::deque<uint32_t*> codedbuffers;
	uint32_t total_input_size = 0;
	buff_size = BUFFSIZE;
	{
		LineIteratorFile lit(urls_file.c_str(),'\n');
		while (lit.has_next() ) {
			const raw_buffer_t &pckt = lit.next();
			std::string str((const char *) pckt.ptr,pckt.size);
			if ( (str.length() == 0 )||(str == "") ) {
				std::cout<<"Skipping url in line" << urls.size() <<STDENDL;
			} else{
				urls.push_back(str);
				uint32_t* codedbuff = new uint32_t[str.length()];
				codedbuffers.push_back(codedbuff);
				total_input_size += str.length();
			}
		}
	}

	uint32_t start_at = 0;
	uint32_t howmanytocode;
//	howmanytocode = 1000;
	howmanytocode = urls.size()-1 ;
	howmanytocode = (howmanytocode>urls.size()) ? urls.size() - 1  : howmanytocode;	//protection

	uint32_t status_every = ((howmanytocode /10) > 0 ) ? (howmanytocode /10) : 1;

	std::cout<<"-- Online Testing on " << howmanytocode << " urls --" <<STDENDL;

	//encode all urls
	std::cout<<"encoding ... "<<std::endl;
	uint32_t decoded_size = 0;
	uint32_t encoded_size = 0;

	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {
		buff_size = BUFFSIZE;
		uint32_t* codedbuff = codedbuffers[i];
		urlc.encode(urls[i],codedbuff,buff_size);
		decoded_size+=urls[i].length();
		encoded_size+=buff_size;
		if (i%status_every == 0)
			std::cout<<"  encoding passed "<<i<<std::endl;
	}
	STOP_TIMING;
	double time_to_encode = GETTIMING;

	//decode all urls
	std::cout<<"decoding ... "<<std::endl;
	START_TIMING;
	for (uint32_t i = start_at ; i < start_at + howmanytocode; i++ ) {

//		buff_size = BUFFSIZE;
//		uint32_t* codedbuff = codedbuffers[i];
//		std::string decoded_str;
//		urlc.decode(decoded_str,codedbuff,buff_size);
//		if (decoded_str != urls[i]) {
//			std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
//			std::cout<<"  " << DVAL(i)<< " "<< DVAL(urls[i])<<" != "<<DVAL(decoded_str)<<STDENDL;
//			std::cout<<"  had length "<<DVAL(codedbuff[0])<<STDENDL;
//			return;
//		}
		if (i%status_every == 0)
					std::cout<<"  decoding passed "<<i<<std::endl;
	}
	STOP_TIMING;
	double time_to_decode = GETTIMING;

	//free what was never yours
	for (uint32_t i = 0 ; i < urls.size(); i++ ) {
		uint32_t* codedbuff = codedbuffers[i];
		delete[] codedbuff;
	}

	uint32_t size = urls.size();
	uint32_t encoded_and_memory = encoded_size + memory_footprint_estimation;

	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<"Printing stats: for "<<size<<" urls"<<std::endl;
	std::cout<<"--------------"<<std::endl;
	std::cout<<DVAL(time_to_load) 	<< "s,  Throughput= "<< double(size/time_to_load)*8/1024/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;

	std::cout<<"Online compression: on "<<howmanytocode << " urls" << STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "s, Throughput= "<< double(decoded_size/time_to_encode)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" "<<DVAL(time_to_decode )<< "s, Throughput= "<< double(encoded_size/time_to_decode)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_decode/howmanytocode) <<"ms"<< STDENDL;

	std::cout<<"Offline compression (load & encode all urls)\n  ~ "
			<< double((double) ( (double) size/(time_to_load + ( time_to_encode * (double) size / howmanytocode) )))* 8 /1024/1024
			<<" Mb/s"<<STDENDL;
	std::cout<<DVAL(decoded_size)<< "Bytes = "<< double((double)decoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(encoded_size)<< "Bytes = "<< double((double)encoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(memory_footprint_estimation)<< 	"Bytes = " << double((double)memory_footprint_estimation / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(encoded_and_memory)<< 	"Bytes = " << double((double)encoded_and_memory / 1024) <<"KB"<< STDENDL;
	std::cout<<"coding ratio (encoded_size/decoded_size) = "<< double((double)encoded_size/(double)decoded_size) * 100 << "%"<<STDENDL;
	std::cout<<"coding ratio (encoded_size+memory_foot_print/decoded_size) = "<< double((double)(encoded_and_memory)/(double)decoded_size) * 100 << "%"<<STDENDL;
	const UrlMatchingModuleStats* stats = urlc.get_stats();
	stats->print(std::cout);

/*
	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<"Printing stats: for "<<size<<" urls"<<std::endl;
	std::cout<<"--------------"<<std::endl;
	std::cout<<DVAL(time_to_load) 	<< "ms,  Throughput= "<< double(size/time_to_load)*8/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;
	std::cout<<"Online compression: on "<<howmanytocode << " urls" << STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "ms, Throughput= "<< double(size/time_to_encode)*8/1024 <<" Mb/s"
			<< "  average/url="<< double(time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" "<<DVAL(time_to_decode )<< "ms, Throughput= "<< double(size/time_to_decode)*8/1024 <<" Mb/s"
			<< "  average/url="<< double(time_to_decode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<"Offline compression (load & encode all urls)\n  ~ "
			<< double((double) ( (double) total_input_size / (time_to_load + ( time_to_encode * (double) size / howmanytocode) )))*8/1024
			<<" Mb/s"<<STDENDL;
	std::cout<<DVAL(decoded_size)<< "Bytes ="<< double((double)decoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<DVAL(encoded_size)<< "Bytes ="<< double((double)encoded_size / 1024) <<"KB"<< STDENDL;
	std::cout<<"coding ratio (encoded_size/decoded_size) = "<< double((double)encoded_size/(double)decoded_size) * 100 << "%"<<STDENDL;
	const HeavyHittersStats* stats = urlc.get_stats();
	stats->print();*/
}


void test_LLH() {
/*
    this->kgram_size            = 8;
    this->n1                    = 3000;
    this->n2                    = 3000;
    this->n3                    = 100;
    this->r                     = 0.1;
    this->attack_high_threshold = 0.50;
    this->peace_high_threshold  = 0.07;
    this->peace_low_threshold   = 0.03;
    this->peace_time_delta      = 0.90;
    this->use_pcaps		= true;
    this->_parse_command_line(argc, argv);
    this->line_del = '\n';
 */
	std::string urls_file = "D:\\Temp\\project\\patterns_url.txt";
	LineIteratorFile lit(urls_file.c_str(),'\n');
	LDHH ldhh(lit, 1000, 1000, 0.1, 8 );
	ldhh.run();

	std::list<signature_t>& peace_signatures = ldhh.get_signatures();
	size_t                  pckt_count       = ldhh.get_pckt_count();
	std::cout << "** scanned " << pckt_count << " packets" << std::endl << std::endl;

	int counter = 0;
	for (std::list<signature_t>::iterator it = peace_signatures.begin(); it != peace_signatures.end(); ++it) {

	    signature_t& sig = *it;
		std::string url;

		const char* str =(const char *) &sig.data[0];
		url.assign(str,  sig.data.size());

	    std::list<signature_t>::size_type data_size = sig.data.size();

//	    ofs.write((const char *)&data_size,                 sizeof(data_size));
//	    ofs.write((const char *)&sig.data[0],               sig.data.size());
//	    ofs.write((const char *)&sig.hh_count,              sizeof(int));
//	    ofs.write((const char *)&sig.real_count,            sizeof(int));
//	    ofs.write((const char *)&sig.real_count_all_series, sizeof(int));
//	    ofs.write((const char *)&sig.src_count,             sizeof(int));
//	    ofs.write((const char *)&sig.cover_rate,            sizeof(double));

		std::cout << counter++ <<": " << url << STDENDL;
		std::cout << "\t" << DVAL(sig.hh_count)
				<< ", " << DVAL(sig.real_count)
				<< ", " << DVAL(sig.real_count_all_series)
				<< ", " << DVAL(sig.src_count)
				<< ", " << DVAL(sig.cover_rate)
				<< STDENDL;
	}
	std::cout << "total of "<< counter <<" patterns were found"<< STDENDL;
}


void take_a_break(int seconds, std::string why) {
	if (seconds == 0)
		return;
	std::cout<<"sleep "<<seconds<<" sec: "<< ((why.length()>0)?why:""  )<<std::endl;
	sleep(seconds);
	std::cout<<" --> continuing"<<STDENDL;
}

bool sanityTesting(UrlMatchingModule& urlc , bool verbose) {

	//Sanity testing - encode/decode a single string
	std::string my_string = "http://www.besound.com/pushead/home.html";
	if (verbose)
		std::cout<<"Sanity testing on \""<<my_string<<"\""<<std::endl;
	uint32_t buff_size = BUFFSIZE;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode(my_string,codedbuff,buff_size);
	if (verbose)
		std::cout<<"encoding length= "<<codedbuff[0]<<" "<<DVAL(buff_size)<< std::endl;

	std::string decoded_str;
	int ret = urlc.decode(decoded_str,codedbuff,buff_size);
	if (ret != STATUS_OK)
		std::cout<<"ERROR DECODING: "<<ret<<STDENDL;
	if (verbose)
		std::cout<<"dencoded string=\""<<decoded_str<<"\""<<std::endl;
	if (my_string != decoded_str) {
		std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
		std::cout<<DVAL(my_string)<<" != "<<DVAL(decoded_str)<<STDENDL;
		std::cout<<" had length "<<DVAL(codedbuff[0])<<STDENDL;
		return false;
	}
	delete[] codedbuff;
	std::cout<<"Sanity.. PASSED"<<STDENDL;
	return true;
}


void printRunTimeStats(CmdLineOptions& options, RunTimeStats& stats, bool print_offline_compression) {

	std::ostream& ofs=std::cout;

	ofs <<"Runtime Statistics: for "<<stats.num_of_urls<<" urls"<<std::endl;
	ofs <<"------------------"<<std::endl;
	ofs <<"Loading: " << STDENDL;
	ofs <<"  Time = " <<stats.time_to_load << "s,  Throughput = "<< double(stats.decoded_size/stats.time_to_load)*8/1024/1024  <<" Mb/s" << STDENDL;
	ofs <<"  Memory footprint (linux only) ~ "<<stats.mem_footprint_est<< "Bytes = "<< double((double)stats.mem_footprint_est / 1024) <<"KB"<< STDENDL;
	ofs <<"  UrlCompressor total allocated memory ~ "<<stats.url_compressor_allocated_memory<< "Bytes = "<< double((double)stats.url_compressor_allocated_memory / 1024) <<"KB"<< STDENDL;
	ofs <<"Online compression:" << STDENDL;
	ofs <<"  time_to_encode = "<<stats.time_to_encode << "s, Throughput= "<< double((stats.decoded_stream_size)/stats.time_to_encode)*8/1024/1024 <<" Mb/s" << STDENDL;
	if (options.test_decoding) {
		ofs <<" time_to_decode ="<<stats.time_to_decode << "s, Throughput= "<< double(stats.encoded_stream_size/stats.time_to_decode)*8/1024/1024 <<" Mb/s" << STDENDL;
	}
	if (print_offline_compression) {
		ofs <<"Offline compression (create dictionary & encode all urls):"<<STDENDL
				<<"  Time = " <<stats.time_to_load + (stats.time_to_encode/options.factor) <<",  Throughput ~"
				<< double(stats.decoded_size/ (stats.time_to_load + (stats.time_to_encode/options.factor)) )*8/1024/1024 <<" Mb/s"<<STDENDL;
	}
	ofs <<"------------------"<<std::endl;

}

void printCompressionStats(CmdLineOptions& options, RunTimeStats& s) {

	uint32_t encoded_and_dict = s.encoded_size + s.dictionary_size;
	std::ostream& ofs = std::cout ;

	ofs <<"Compression Statistics:"<<STDENDL;
	ofs <<"----------------------"<<std::endl;
	if (options.split_for_LPM)
		ofs <<"Splitted to components by \""<<options.LPM_delimiter<<"\""<<STDENDL;
	ofs <<"Decoded size = "<< s.decoded_size<< " Bytes = "<< double((double)s.decoded_size / 1024) <<"KB"<< STDENDL;
	ofs <<"Encoded size = "<< s.encoded_size<< " Bytes = "<< double((double)s.encoded_size / 1024) <<"KB"<< STDENDL;
	ofs <<"Dictionary size = "<< s.dictionary_size<< " Bytes = "<< double((double)s.dictionary_size / 1024) <<"KB"<< STDENDL;
	ofs <<"coding ratio (encoded_size/decoded_size) = "<< double((double)s.encoded_size/(double)s.decoded_size) * 100 << "%"<<STDENDL;
	ofs <<"coding ratio (encoded_size+dict_size/decoded_size) = "<< double((double)(encoded_and_dict)/(double)s.decoded_size) * 100 << "%"<<STDENDL;
	ofs <<"----------------------"<<std::endl;

}

void printAlgorithmStats(CmdLineOptions& options, const UrlMatchingModuleStats* stats ) {
	std::ostream& ofs=std::cout;
	ofs <<"Algorithm's dicionary Statistics:"<<STDENDL;
	ofs <<"--------------------------------"<<std::endl;
	stats->print(ofs);
	ofs <<"--------------------------------"<<std::endl;
}

void createOptionalOutputFile(CmdLineOptions& options, RunTimeStats& rt_stat , const UrlMatchingModuleStats* stats ) {
	using namespace std;
	if (!options.custom_output_file)
		return;
	std::deque<pair<std::string,std::string>> outmap;
	std::string filename = options.input_urls_file_path;
	if (options.split_for_LPM)
		filename = filename+"<"+options.LPM_delimiter+">";

	outmap.push_back(std::pair<std::string,std::string>("filename",(filename)));
	outmap.push_back(std::pair<std::string,std::string>("urls",std::to_string(stats->number_of_urls)));
	outmap.push_back(pair<string,string>("n1",std::to_string(stats->params.n1)));
	outmap.push_back(pair<string,string>("n2",std::to_string(stats->params.n2)));
	outmap.push_back(pair<string,string>("r",std::to_string(stats->params.r)));
	outmap.push_back(pair<string,string>("kgram",std::to_string(stats->params.kgrams_size)));
	outmap.push_back(pair<string,string>("#symbols",std::to_string(stats->number_of_symbols)));
	outmap.push_back(pair<string,string>("#patterns",std::to_string(stats->number_of_patterns)));
	outmap.push_back(pair<string,string>("loading time sec",std::to_string(rt_stat.time_to_load)));
	outmap.push_back(pair<string,string>("decoded size Bytes",std::to_string(rt_stat.decoded_size)));
	outmap.push_back(pair<string,string>("encoded size Bytes",std::to_string(rt_stat.encoded_size)));
	outmap.push_back(pair<string,string>("total decoded size Bytes",std::to_string(rt_stat.decoded_stream_size)));
	outmap.push_back(pair<string,string>("total encoded size Bytes",std::to_string(rt_stat.encoded_stream_size)));
	outmap.push_back(pair<string,string>("encoding time sec",std::to_string(rt_stat.time_to_encode)));
	outmap.push_back(pair<string,string>("dictionary size Bytes",std::to_string(rt_stat.dictionary_size)));
	outmap.push_back(pair<string,string>("Total mem footprint Bytes",std::to_string(stats->memory_footprint)));
	outmap.push_back(pair<string,string>("AC module mem footprint Bytes",std::to_string(stats->ac_memory_footprint)));
	outmap.push_back(pair<string,string>("AC module mem allocated Bytes",std::to_string(stats->ac_memory_allocated)));
	outmap.push_back(pair<string,string>("AC statemachine mem footprint Bytes",std::to_string(stats->ac_statemachine_footprint)));
	outmap.push_back(pair<string,string>("AC statemachine mem allocated Bytes",std::to_string(stats->getACMachineEstSize())));
	outmap.push_back(pair<string,string>("UrlC mem allocated Bytes",std::to_string(stats->memory_allocated)));

	ofstream out_file;
	out_file.open (options.output_file_path.c_str(),ios::app );
	if (options.add_header_to_output_file) {
		for (std::deque<std::pair<std::string,std::string>>::iterator it = outmap.begin(); it != outmap.end(); ++it) {
			out_file<<it->first<<",";
		}
		out_file<<std::endl;
	}
	for (std::deque<std::pair<std::string,std::string>>::iterator it = outmap.begin(); it != outmap.end(); ++it) {
		out_file<<it->second<<",";
	}
	out_file<<std::endl;
	out_file.close();
}

void createOptionalDictionaryFile(CmdLineOptions& options, UrlMatchingModule& urlc) {
	using namespace std;
	if (!options.print_dicionary)
		return;
	ofstream printout_file;
	printout_file.open (options.print_dicionary_file.c_str(),std::ofstream::out );
	urlc.print_database(printout_file);
	printout_file.close();
	std::cout << "Dicionary outputed to: "<<options.print_dicionary_file<<std::endl;
}

void createOptionalDumpACStatesFile(CmdLineOptions& options, UrlMatchingModule& urlc) {
	using namespace std;
	if (options.dump_ac_statetable) {
		urlc.dump_ac_states(options.dump_ac_statetable_filename);
		std::cout << "Aho Corasic states dump: "<<options.dump_ac_statetable_filename<<std::endl;
	}
}
