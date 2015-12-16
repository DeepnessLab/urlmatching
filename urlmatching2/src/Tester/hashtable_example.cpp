/*
 * hashtable_example.cpp
 *
 *  Created on: 18 December 2014
 *      Author: Daniel Krauthgamer
 *
 *  Tester for using the module as hash keys
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <assert.h>
#include <ctime>
#include <unordered_map>



#include "tester.h"
#include "../UrlToolkit/Huffman.h"
#include "../UrlToolkit/UrlMatching.h"
#include "../HeavyHitters/dhh_lines.h"
#include "../UrlToolkit/CodePack.h"
#include "../logger.h"
#include "../common.h"


#define BUFFSIZE 500
int retry_count = 1;
CmdLineOptions* global_options = 0;


typedef CodePack::CodePackT Encoded ;
typedef uint32_t IPv4_t;

namespace std {
template <>
struct hash<Encoded>
{
	size_t operator()(const Encoded& t) const {
		return t.hash();
	}
};
}	//namespace std

struct EncodedHasher
{
	size_t operator()(const Encoded& t) const {
		return t.hash();
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

		decompressed_size = 0;
		insert_time_for_strings=0.0;
		insert_time_for_encoded=0.0;

		lookup_decompressed_size_for_string=0;
		lookup_decompressed_size_for_encoded=0;
		number_oflookups=0;
		lookup_time_for_strings=0.0;
		lookup_time_for_encoded=0.0;
		strings_load_factor = 0.0;
		encoded_load_factor = 0.0;
	}

	uint32_t num_of_urls;
	int mem_footprint_hashtable_strings;
	int mem_allocated_hashtable_strings;
	int mem_footprint_hashtable_encoded;
	int mem_allocated_hashtable_encoded;

	uint32_t url_compressor_allocated_memory;
	uint32_t hashtable_strings_key_size;
	uint32_t hashtable_encoded_key_size;

	uint32_t decompressed_size;
	double insert_time_for_strings;
	double insert_time_for_encoded;

	uint32_t lookup_decompressed_size_for_string;
	uint32_t lookup_decompressed_size_for_encoded;
	uint32_t number_oflookups;
	double lookup_time_for_strings;
	double lookup_time_for_encoded;

	double strings_load_factor;
	double encoded_load_factor;
};

typedef std::unordered_map<std::string,uint32_t>			unordered_map_strings;
typedef std::unordered_map<Encoded,uint32_t,EncodedHasher> 	unordered_map_encoded;

void lookup_strings_hashtable(UrlMatchingModule& urlc, unordered_map_strings hashtable_strings, std::deque<std::string>& urls
		, uint32_t* random_indices, int num_of_lookups, RunTimeStatsHashtable& rt_stats);
void lookup_encoded_hashtable(UrlMatchingModule& urlc, unordered_map_encoded hashtable_encoded
		, std::deque<std::string>& urls, uint32_t* random_indices, int num_of_lookups, RunTimeStatsHashtable& rt_stats);

void createOptionalOutputFile(CmdLineOptions& options, RunTimeStatsHashtable& rt_stat , const UrlMatchingModuleStats* stats );

double ratio(uint64_t top, uint64_t buttom) {
	double ret = double (  (double) top / (double) buttom);
	return ret;
}


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
	global_options = &options;

	PREPARE_TIMING;

	std::cout<<" --- Testing hashtable ---"<<std::endl;
	options.PrintParameters(std::cout);
	DoubleHeavyHittersParams_t customParams = {/*n1*/ options.n1, /*n2*/ options.n2, /*r*/ options.r, /*kgrams_size*/ options.kgram_size};
	DoubleHeavyHittersParams_t& params = customParams; //default_hh_params;

	// ----
	//    Create compression module
	// ------------------------------------

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

	bool retB = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	uint32_t memory_allocated_estimation = urlc.SizeOfTotalAllocatedMemory();
	assert (retB);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}
	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	sanityTesting(urlc, false);

	// ----
	//	Count urls and prepare coding buffers
	// ------------------------------------
	std::cout<<"Preparing: reading file and allocating memory... "<<std::endl;
	std::deque<std::string>& urls = url_deque;
	uint32_t total_input_size = 0;
	uint32_t buff_size = BUFFSIZE;
	for (std::deque<std::string>::iterator it = url_deque.begin(); it != url_deque.end(); ++it) {
		if ( (it->length() == 0 )||(*it == "") ) {
			std::cout<<"Skipping url in line " << urls.size() +1<<STDENDL;
		} else{
			total_input_size += it->length();
		}
	}

	uint32_t howmanytocode = urls.size() ;
	rt_stats.num_of_urls = urls.size() ;

	std::cout<<std::endl;
	std::cout<<"-- Hashtable Testing on " << howmanytocode << " urls --" <<STDENDL;

	// ----
	//	Encode all urls to determine allocator size
	// ------------------------------------
	std::cout<<"Preparing: encoding all urls for statistics"<<std::endl;
	uint32_t decoded_size = 0;
	uint32_t encoded_size = 0;
	uint32_t allocator_size = 0;
	for (uint32_t i = 0 ; i < howmanytocode; i++ ) {
		uint32_t codedbuff[BUFFSIZE];
		buff_size = BUFFSIZE;
		urlc.encode(urls[i],codedbuff,buff_size);
		allocator_size += conv_bits_to_bytes(codedbuff[0]);
		allocator_size += sizeof(CodePack::lenT);

		decoded_size+=urls[i].length();
		encoded_size+=conv_bits_to_bytes(codedbuff[0]);
	}


	//Simulate Hashtable <url, IPv4 (uint32_t)>
	IPv4_t myIPv4=0xDEAD;

//	howmanytocode = 100000;

	// ----
	// 	Test hashtable using <std::string,IPV4>
	// ------------------------------------
	rt_stats.mem_footprint_hashtable_strings = get_curr_memsize();
	unordered_map_strings hashtable_string;
	hashtable_string.reserve( urls.size()*2);
	std::cout<<"inserting all urls to a hashtable_string (std::unordered_map) .. "<<std::endl;
	START_TIMING;
	for (uint32_t i = 0 ; i < howmanytocode; i++ ) {
		myIPv4=i;
		std::string str (urls[i].c_str());	//new allocation
		uint32_t keysize = str.length() + 1 /*'\0'*/+ sizeof(str);
		rt_stats.hashtable_strings_key_size += keysize;
		rt_stats.mem_allocated_hashtable_strings += keysize;
		rt_stats.mem_allocated_hashtable_strings += sizeof(myIPv4);
		hashtable_string[str] = myIPv4;
		rt_stats.decompressed_size+=urls[i].length();
	}
	STOP_TIMING;
	rt_stats.insert_time_for_strings = GETTIMING;
	rt_stats.mem_footprint_hashtable_strings = get_curr_memsize() - rt_stats.mem_footprint_hashtable_strings;
	for (uint32_t i = 0 ; i < hashtable_string.bucket_count(); i++ ) {
		rt_stats.mem_allocated_hashtable_strings += 8;
		rt_stats.mem_allocated_hashtable_strings += hashtable_string.bucket_size(i) * 8;
	}
	rt_stats.strings_load_factor = hashtable_string.load_factor();

	// ----
	//	Test hashtable using <Encoded,IPV4>
	// ------------------------------------
	rt_stats.mem_footprint_hashtable_encoded = get_curr_memsize();
	unordered_map_encoded hashtable_encoded;
	hashtable_encoded.reserve( urls.size()*2);

	SerialAllocator<char>* _charsAllocator = new SerialAllocator<char>(allocator_size + 10 );

	std::cout<<"inserting all urls to a hashtable_encoded (std::unordered_map) .. "<<std::endl;
	START_TIMING;
	for (uint32_t i = 0 ; i <  howmanytocode /*howmanytocode*/; i++ ) {
		myIPv4=i;
		uint32_t codedbuff[BUFFSIZE];
		buff_size = BUFFSIZE;
		urlc.encode(urls[i],codedbuff,buff_size);	//redo the encoding process
		Encoded packed;
		packed.Pack(codedbuff[0], &(codedbuff[1]) , _charsAllocator);
		rt_stats.hashtable_encoded_key_size += sizeof(Encoded) ;
		rt_stats.mem_allocated_hashtable_encoded+= sizeof(myIPv4); //we add the allocator at the end
		hashtable_encoded[packed] = myIPv4;
	}
	STOP_TIMING;
	rt_stats.insert_time_for_encoded = GETTIMING;
	rt_stats.hashtable_encoded_key_size = _charsAllocator->capacity();
	rt_stats.mem_allocated_hashtable_encoded+= rt_stats.hashtable_encoded_key_size;
	rt_stats.mem_footprint_hashtable_encoded = get_curr_memsize() - rt_stats.mem_footprint_hashtable_encoded;

	for (uint32_t i = 0 ; i < hashtable_encoded.bucket_count(); i++ ) {
		rt_stats.mem_allocated_hashtable_encoded += 8;
		rt_stats.mem_allocated_hashtable_encoded += hashtable_encoded.bucket_size(i) * 8;
	}
	rt_stats.encoded_load_factor = hashtable_encoded.load_factor();

	// ----
	//	Lookup testing preparation step
	// ------------------------------------
	std::srand(std::time(0)); 						// use current time as seed for random generator
	int num_of_lookups = (options.y != 0) ? options.y : 10000;		//use -y to use different number of lookups than 100000
	retry_count = (options.z != 0) ? options.z : 20; 				//use -z to use different number of lookups than 100000
	uint32_t* random_indices = new uint32_t[num_of_lookups];
	for (int i=0; i < num_of_lookups; i++) {
		uint32_t idx = (uint32_t) std::rand();
		idx = idx % urls.size();
		random_indices[i] = idx;
	}
	rt_stats.number_oflookups = num_of_lookups;

	if (options.use_url_test_file) {
		url_deque.clear();
		if (! urlc.getUrlsListFromFile(options.url_test_file, url_deque)) {
			std::cout<<"Error with input file"<<STDENDL;
			exit (1);
		}
	}

	// ----
	//	Lookup in string hashtable
	// ------------------------------------
	lookup_strings_hashtable(urlc, hashtable_string, urls, random_indices, num_of_lookups, rt_stats);
	hashtable_string.clear();
	// ----
	//	Lookup in encoded hashtable
	// ------------------------------------
	lookup_encoded_hashtable(urlc, hashtable_encoded, urls, random_indices, num_of_lookups, rt_stats);


	if (options.test_decoding) {
		std::cout<<"reading all urls from hashtable(std::unordered_map) and verify them.. "<<std::endl;
		for (auto it = hashtable_encoded.begin(); it != hashtable_encoded.end(); ++it) {
			myIPv4 = it->second;
			uint32_t idx = myIPv4;
			Encoded packed = it->first;
			uint32_t codedbuff[BUFFSIZE];
			codedbuff[0] = packed.getBitLen();
			packed.UnPack(&(codedbuff[1]));

			std::string decoded_str;
			urlc.decode(decoded_str,codedbuff,urls[idx].length());

			if (decoded_str != urls[idx]) {
				std::cout<<"ERROR: Retrieved wrong string from hashtable"<<STDENDL;
				std::cout<<"  found(i="<<idx<<"): " << decoded_str     <<STDENDL;
				std::cout<<"  expected: " << urls[idx] <<STDENDL;
				std::cout<<"  had bit length "<<DVAL(packed.getBitLen())<<STDENDL;
				return;
			}
		}

	}

	uint32_t size = urls.size();

	std::cout<<STDENDL;
	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<" ---"<<std::endl;
	std::cout<<"Hashtable Statistics: for "<<size<<" urls"<<std::endl;
	std::cout<<"------------------"<<std::endl;
	std::cout<<" URL compressor allocated memory = "<<Byte2KB(memory_allocated_estimation)<<"KB"<<std::endl;

	std::cout<<"Hashtable Load factor:"<<std::endl;
	std::cout<<" strings = "<< rt_stats.strings_load_factor<<"\t";
	std::cout<<" encoded = "<<rt_stats.encoded_load_factor<<std::endl;

	std::cout<<"Hashtable Memory Allocated:"<<std::endl;
	std::cout<<" strings = "<<Byte2KB(rt_stats.mem_allocated_hashtable_strings)<<"KB"<<"\t";
	std::cout<<" encoded = "<<Byte2KB(rt_stats.mem_allocated_hashtable_encoded)<<"KB"<<"\t";
	std::cout<<" Ratio = "<<std::setprecision(3)<<double ( (double) rt_stats.mem_allocated_hashtable_encoded / (double)rt_stats.mem_allocated_hashtable_strings)<<std::endl;
	std::cout<<std::setprecision(6);

	//The linux memory footprint is not accurate, some buffers are allocated before the begining of measure
//	std::cout<<"Hashtable Memory footprint (linux only):"<<std::endl;
//	std::cout<<" hashtable strings = "<<Byte2KB(rt_stats.mem_footprint_hashtable_strings)<<"KB"<<"\t";
//	std::cout<<" hashtable encoded = "<<Byte2KB(rt_stats.mem_footprint_hashtable_encoded)<<"KB"<<"\t";
//	std::cout<<" Ratio = "<<std::setprecision(3)<<double ( (double) rt_stats.mem_footprint_hashtable_encoded / (double) rt_stats.mem_footprint_hashtable_strings)<<std::endl;
//	std::cout<<std::setprecision(6);

	std::cout<<"Keysize Memory allocated:"<<std::endl;
	std::cout<<" strings = "<<Byte2KB(rt_stats.hashtable_strings_key_size)<<"KB"<<"\t";
	std::cout<<" encoded = "<<Byte2KB(rt_stats.hashtable_encoded_key_size)<<"KB"<<"\t";
	std::cout<<" Ratio = "<<std::setprecision(3)<<double ( (double) rt_stats.hashtable_encoded_key_size / (double) rt_stats.hashtable_strings_key_size)<<std::endl;
	std::cout<<std::setprecision(6);

	std::cout<<"Insertion timing:"<<std::endl;
	std::cout<<" strings = "<<std::setprecision(3)<<rt_stats.insert_time_for_strings<<"sec"<<"\t";
	std::cout<<" encoded = "<<std::setprecision(3)<<rt_stats.insert_time_for_encoded<<"sec"<<"\t";
	std::cout<<" Ratio = "<<std::setprecision(3)<<double ( rt_stats.insert_time_for_encoded / rt_stats.insert_time_for_strings)<<std::endl;
	std::cout<<" strings = "<<std::setprecision(5)
	<<(long double) ( (long double) rt_stats.decompressed_size / rt_stats.insert_time_for_strings) *8/1024/1024<<"Mbps"<<"\t";
	std::cout<<" encoded = "<<std::setprecision(5)
	<<(long double) ( (long double) rt_stats.decompressed_size / rt_stats.insert_time_for_encoded) *8/1024/1024<<"Mbps"<<std::endl;
	std::cout<<std::setprecision(6);


	std::cout<<"Lookup time on size: "<<Byte2KB(rt_stats.lookup_decompressed_size_for_string) <<"KB:"<<std::endl;
	std::cout<<" strings = "<<std::setprecision(3)<<rt_stats.lookup_time_for_strings<<"sec"<<"\t";
	std::cout<<" encoded = "<<std::setprecision(3)<<rt_stats.lookup_time_for_encoded<<"sec"<<"\t";
	std::cout<<" Ratio = "<<std::setprecision(3)<<double ( rt_stats.lookup_time_for_encoded / rt_stats.lookup_time_for_strings)<<std::endl;

	long double lookup_strings_throughput = getMbps(rt_stats.lookup_decompressed_size_for_string, rt_stats.lookup_time_for_strings );
	std::cout<<" strings = "<<std::setprecision(5)
	<< lookup_strings_throughput <<" Mbps"<<"\t";
	long double lookup_encoded_throughput = getMbps(rt_stats.lookup_decompressed_size_for_encoded,rt_stats.lookup_time_for_encoded);
	std::cout<<" encoded = "<<std::setprecision(5)
	<<lookup_encoded_throughput<<" Mbps"<<"\t";
	std::cout<<" Ratio = "<<std::setprecision(3)<< lookup_encoded_throughput / lookup_strings_throughput <<std::endl;
	std::cout<<std::setprecision(6);

	std::cout<<"------------------"<<std::endl;
	std::cout<<"Algorithm Statistics:"<<STDENDL;
	std::cout<<"--------------------"<<std::endl;
	const UrlMatchingModuleStats* stats = urlc.get_stats();
	std::cout<<" number_of_patterns = "<<stats->number_of_patterns<<std::endl;
//	std::cout<<"Static sizes:"<<STDENDL;
//	std::cout<<" "<<DVAL(sizeof(myIPv4))<<std::endl;
//	std::cout<<" "<<DVAL(sizeof(Encoded))<<std::endl;
//	std::cout<<" "<<DVAL(sizeof(std::string))<<std::endl;
//	stats->print(std::cout);

	if (options.print_dicionary) {
		ofstream printout_file;
		printout_file.open (options.print_dicionary_file.c_str(),std::ofstream::out );
		urlc.print_database(printout_file);
		printout_file.close();
		std::cout <<std::endl<< "Dicionary outputed to: "<<options.print_dicionary_file<<std::endl;
	}
	createOptionalOutputFile(options,rt_stats,stats);
}



void lookup_strings_hashtable(UrlMatchingModule& urlc, unordered_map_strings hashtable_strings, std::deque<std::string>& urls
		, uint32_t* random_indices, int num_of_lookups, RunTimeStatsHashtable& rt_stats)
{
	rt_stats.lookup_decompressed_size_for_string=0;
	IPv4_t verifier=0;
	std::cout<<"Performing lookups on hashtable_strings (std::unordered_map) .. "<<std::endl;
	uint32_t num = 0;
	uint32_t idx = random_indices[0] % (1000000);
//	uint32_t idx = 0;
	TimerUtil lookup_strings_timer(false);
	lookup_strings_timer.start();
	//VERSION 3
	if (global_options->use_url_test_file) {
		for (uint32_t i=0; i < urls.size(); i++) {
			std::string str = urls[i];
			unordered_map_strings::const_iterator it = hashtable_strings.find(str);
			IPv4_t ip = i;
			verifier += (ip - i);
			rt_stats.lookup_decompressed_size_for_string+=urls[i].length();
			num++;
		}
	} else {
		//VERSION 2
		for (int time=0; time < retry_count; time++) {
			for (uint32_t i=idx; i < idx + num_of_lookups; i++) {
				std::string str = urls[i];
				IPv4_t ip = hashtable_strings[str];
				verifier += (ip - i);
				rt_stats.lookup_decompressed_size_for_string+=urls[i].length();
				num++;
			}
			idx+= num_of_lookups / 10;
		}
	}

	//VERSION 1
//	for (int time=0; time < retry_count; time++) {
//		for (int i=0; i < num_of_lookups; i++) {
//			uint32_t idx = random_indices[i];
//			std::string str = urls[idx];
//			IPv4_t ip = hashtable_strings[str];
//			verifier += (ip - random_indices[i]);
//			rt_stats.lookup_decompressed_size_for_string+=urls[idx].length();
//			num++;
//		}
//	}
	lookup_strings_timer.stop();
	rt_stats.lookup_time_for_strings = lookup_strings_timer.get_seconds();
	if (verifier!=0)
		std::cout<<"Some wrong IPs were retrieved " <<STDENDL;

	std::cout<<" |-encoding Timer break down for " << num << " lookups "<<std::endl;
	std::cout<<"  |- "<<std::setprecision(3)<< "Lookup time = " << lookup_strings_timer.get_milseconds()<< " ms, "
			<<std::setprecision(5)<<getMbps(rt_stats.lookup_decompressed_size_for_string, rt_stats.lookup_time_for_strings)<<"Mbps, "
			<<std::setprecision(6)
			<<Byte2KB(rt_stats.lookup_decompressed_size_for_string) << " KB"<<std::endl;
}


void lookup_encoded_hashtable(UrlMatchingModule& urlc, unordered_map_encoded hashtable_encoded, std::deque<std::string>& urls
		, uint32_t* random_indices, int num_of_lookups, RunTimeStatsHashtable& rt_stats)
{
	rt_stats.lookup_decompressed_size_for_encoded=0;
	// ----
	//	Lookup in encoded hashtable
	// ------------------------------------
	SerialAllocator<char> charsAllocator2(rt_stats.decompressed_size);
	IPv4_t verifier=0;
	uint32_t num=0;
	std::cout<<"Performing lookups on hashtable_encoded (std::unordered_map) .. "<<std::endl;
	TimerUtil lookup_encoded_timer(false);
	uint32_t idx = random_indices[0] % (1000000);
//	uint32_t idx = 0;
	lookup_encoded_timer.start();
	if (global_options->use_url_test_file) {
		//VERSION 3
		for (uint32_t i=0; i < urls.size(); i++) {
			std::string str = urls[i];
			uint32_t codedbuff[BUFFSIZE];
			uint32_t buff_size = BUFFSIZE;
			urlc.encode(str,codedbuff,buff_size);	//redo the encoding process
			Encoded packed;
			packed.Pack(codedbuff[0], &(codedbuff[1]) , &charsAllocator2);
			unordered_map_encoded::const_iterator it = hashtable_encoded.find(packed);
			IPv4_t ip = i;
			verifier += (ip - i);
			rt_stats.lookup_decompressed_size_for_encoded+=urls[i].length();
			num++;
		}
	} else {
		//	VERSION 2
		for (int time=0; time < retry_count; time++) {
			for (uint32_t i=idx; i < idx + num_of_lookups; i++) {
				std::string str = urls[i];
				uint32_t codedbuff[BUFFSIZE];
				uint32_t buff_size = BUFFSIZE;
				urlc.encode(str,codedbuff,buff_size);	//redo the encoding process
				Encoded packed;
				packed.Pack(codedbuff[0], &(codedbuff[1]) , &charsAllocator2);
				IPv4_t ip = hashtable_encoded[packed];
				verifier += (ip - i);
				rt_stats.lookup_decompressed_size_for_encoded+=urls[i].length();
				num++;
			}
			idx+= num_of_lookups / 10;
		}
	}
	//VERSION 1
//	for (int time=0; time < retry_count; time++) {
//		for (int i=0; i < num_of_lookups; i++) {
//			uint32_t idx = random_indices[i];
//			std::string str = urls[idx];
//			uint32_t codedbuff[BUFFSIZE];
//			uint32_t buff_size = BUFFSIZE;
//			urlc.encode(str,codedbuff,buff_size);	//redo the encoding process
//			Encoded packed;
//			packed.Pack(codedbuff[0], &(codedbuff[1]) , &charsAllocator2);
//			IPv4_t ip = hashtable_encoded[packed];
//			verifier += (ip - random_indices[i]);
//			rt_stats.lookup_decompressed_size_for_encoded+=urls[idx].length();
//			num++;
//		}
//	}
	lookup_encoded_timer.stop();
	rt_stats.lookup_time_for_encoded = lookup_encoded_timer.get_seconds();
	if (verifier!=0)
		std::cout<<"Some wrong IPs were retrieved " <<STDENDL;

	std::cout<<" |-encoding Timer break down for " << num << " lookups"<<std::endl;
	std::cout<<"  |- "<<std::setprecision(3)<< "Lookup time = " << lookup_encoded_timer.get_milseconds()<< " ms, "
			<<std::setprecision(5)<< getMbps(rt_stats.lookup_decompressed_size_for_encoded ,rt_stats.lookup_time_for_encoded) <<" Mbps, "
			<<std::setprecision(6)
		<<Byte2KB(rt_stats.lookup_decompressed_size_for_encoded) << " KB"<<std::endl;
}


void createOptionalOutputFile(CmdLineOptions& options, RunTimeStatsHashtable& rt_stat , const UrlMatchingModuleStats* stats ) {
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
	outmap.push_back(pair<string,string>("strings load facor",std::to_string(rt_stat.strings_load_factor)));
	outmap.push_back(pair<string,string>("encoded load facor",std::to_string(rt_stat.encoded_load_factor)));

	outmap.push_back(pair<string,string>("strings Allocated Bytes",std::to_string(rt_stat.mem_allocated_hashtable_strings)));
	outmap.push_back(pair<string,string>("encoded Allocated Bytes",std::to_string(rt_stat.mem_allocated_hashtable_encoded)));
	outmap.push_back(pair<string,string>("ratio",std::to_string(ratio (rt_stat.mem_allocated_hashtable_encoded, rt_stat.mem_allocated_hashtable_strings))));

	outmap.push_back(pair<string,string>("strings Insertion time ms",std::to_string(rt_stat.insert_time_for_strings / 1000)));
	outmap.push_back(pair<string,string>("encoded Insertion time ms",std::to_string(rt_stat.insert_time_for_encoded / 1000)));
	long double throughput_s = getMbps(rt_stat.decompressed_size, rt_stat.insert_time_for_strings);
	outmap.push_back(pair<string,string>("strings Insertion throughput Mbps",std::to_string(throughput_s)));
	long double throughput_e = getMbps(rt_stat.decompressed_size, rt_stat.insert_time_for_encoded);
	outmap.push_back(pair<string,string>("encoeded Insertion throughput Mbps",std::to_string(throughput_e)));
	outmap.push_back(pair<string,string>("ratio",std::to_string(ratio (throughput_e, throughput_s))));

	outmap.push_back(pair<string,string>("strings Lookup size KB",std::to_string(Byte2KB(rt_stat.lookup_decompressed_size_for_string))));
	outmap.push_back(pair<string,string>("encoded Lookup size KB",std::to_string(Byte2KB(rt_stat.lookup_decompressed_size_for_encoded))));
	outmap.push_back(pair<string,string>("strings Lookup time ms",std::to_string(rt_stat.lookup_time_for_strings / 1000)));
	outmap.push_back(pair<string,string>("encoded Lookup time ms",std::to_string(rt_stat.lookup_time_for_encoded / 1000)));
	throughput_s = getMbps(rt_stat.lookup_decompressed_size_for_string, rt_stat.lookup_time_for_strings);
	outmap.push_back(pair<string,string>("strings Lookup throughput Mbps",std::to_string(throughput_s)));
	throughput_e = getMbps(rt_stat.lookup_decompressed_size_for_encoded, rt_stat.lookup_time_for_encoded);
	outmap.push_back(pair<string,string>("encoeded Lookup throughput Mbps",std::to_string(throughput_e)));
	outmap.push_back(pair<string,string>("ratio",std::to_string(ratio (throughput_e, throughput_s))));

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

