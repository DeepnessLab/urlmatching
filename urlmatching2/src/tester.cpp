/*
 * tester.cpp
 *
 *  Created on: 18 ���� 2014
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

#ifdef DVAL
#undef DVAL
#endif
#define DVAL(what) #what" = "<< (what)

void test_main(CmdLineOptions& options) {
	using namespace std;

	PREPARE_TIMING;
	std::cout<<"##################"<<std::endl;
	options.PrintParameters(std::cout);
//	std::cout<<"urls file path="<<options.input_urls_file_path<<std::endl;
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
		urlc.SplitUrlsList(url_deque, *splitted_deque);
		input_for_urlcompressor = splitted_deque;
	}

	take_a_break(options.break_time," before loading");
	START_TIMING;
	bool retB = urlc.LoadUrlsFromList(*input_for_urlcompressor, params, false);
	STOP_TIMING;
	take_a_break(options.break_time," after loading");
	double time_to_load = GETTIMING;
	uint32_t memory_footprint_estimation = urlc.SizeOfMemory();
	assert (retB);

	if (options.split_for_LPM) {	//free unecessary memory
		delete input_for_urlcompressor;
	}

//	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	//Sanity testing - encode/decode a single string
	std::string my_string = "http://www.besound.com/pushead/home.html";
	std::cout<<"Sanity testing on \""<<my_string<<"\""<<std::endl;
	uint32_t buff_size = BUFFSIZE;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode_2(my_string,codedbuff,buff_size);
	std::cout<<"encoding length= "<<codedbuff[0]<<" "<<DVAL(buff_size)<< std::endl;

	std::string decoded_str;
	int ret = urlc.decode(decoded_str,codedbuff,buff_size);
	if (ret != STATUS_OK)
		std::cout<<"ERROR DECODING: "<<ret<<STDENDL;
	std::cout<<"dencoded string=\""<<decoded_str<<"\""<<std::endl;
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
	uint32_t status_every = ((howmanytocode /10) > 0 ) ? (howmanytocode /10) : 1;

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
//		if (i%status_every == 0)
//			std::cout<<"  encoding passed "<<i<<std::endl;
	}
	STOP_TIMING;
	double time_to_encode = GETTIMING;

	if (options.test_decoding) {
		//decode all urls
		std::cout<<"decoding ... "<<std::endl;
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
				std::cout<<"  decoding passed "<<i<<std::endl;
		}
		STOP_TIMING;
	}
	double time_to_decode = (options.test_decoding) ? GETTIMING: 0l;

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
	std::cout<<"  Time= " <<time_to_load << "s,  Bandwidth= "<< double(size/time_to_load)*8/1024/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;
	std::cout<<"  Memory footprint est.="<<memory_footprint_estimation<< "Bytes = "<< double((double)memory_footprint_estimation / 1024) <<"KB"<< STDENDL;

	std::cout<<"Online compression: on "<<howmanytocode << " urls" << STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "s, Bandwidth= "<< double(decoded_size/time_to_encode)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	if (options.test_decoding) {
		std::cout<<" "<<DVAL(time_to_decode )<< "s, Bandwidth= "<< double(encoded_size/time_to_decode)*8/1024/1024 <<" Mb/s"
				<< "  average/url="<< double((double) time_to_decode/howmanytocode) <<"ms"<< STDENDL;
	}

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
		printout_file.open (options.dicionary_output_file.c_str(),std::ofstream::out );
		urlc.print_database(printout_file);
		printout_file.close();
		std::cout <<std::endl<< "Dicionary outputed to: "<<options.dicionary_output_file<<std::endl;
	}

	ofstream out_file;
	out_file.open (options.output_file_path.c_str(),ios::app );
	if (options.add_header_to_output_file) {
		out_file << "filename," <<"#urls,"
				<<"n1,"<<"n2," <<"r,"<<"kgram,"
				<<"#symbols,"<<"#patterns,"
				<<"loading time sec," << "decoded size B," << "encoded size B, " << "encoding time sec,"
				<<"memory_footprint_estimation B,"<<"dictionary size B"
				<< std::endl;
	}
	out_file << options.input_urls_file_path <<"," <<stats->number_of_urls<<","
			<<stats->params.n1<<","<<stats->params.n2<<","<<stats->params.r<<","<<stats->params.kgrams_size
			<<","<<stats->number_of_symbols<<","<<stats->number_of_patterns
			<<","<<time_to_load<<","<<decoded_size<<","<<encoded_size<<","<<time_to_encode
			<<","<<memory_footprint_estimation<<","<<dict_size
			<< std::endl;
	out_file.close();
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
	HeavyHittersParams_t customParams = {/*n1*/ n, /*n2*/ n, /*r*/ 0.8, /*kgrams_size*/ 8};
	HeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlCompressor urlc;
	int break_time=0;
	take_a_break(break_time," before loading");
	std::deque<std::string> url_deque;
	urlc.getUrlsListFromFile(urls_file, url_deque);
	std::deque<std::string> splitted_deque;
	urlc.SplitUrlsList(url_deque, splitted_deque);
	START_TIMING;
//	bool retB = urlc.LoadUrlsFromFile(urls_file, params, false);
	bool retB = urlc.LoadUrlsFromList(splitted_deque, params, false);
	STOP_TIMING;
	take_a_break(break_time," after loading");
	double time_to_load = GETTIMING;
	uint32_t memory_footprint_estimation = urlc.SizeOfMemory();
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
	urlc.encode_2(my_string,codedbuff,buff_size);
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
		urlc.encode_2(urls[i],codedbuff,buff_size);
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
	std::cout<<DVAL(time_to_load) 	<< "s,  Bandwidth= "<< double(size/time_to_load)*8/1024/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;

	std::cout<<"Online compression: on "<<howmanytocode << " urls" << STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "s, Bandwidth= "<< double(decoded_size/time_to_encode)*8/1024/1024 <<" Mb/s"
			<< "  average/url="<< double((double) time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" "<<DVAL(time_to_decode )<< "s, Bandwidth= "<< double(encoded_size/time_to_decode)*8/1024/1024 <<" Mb/s"
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
	const UrlCompressorStats* stats = urlc.get_stats();
	stats->print(std::cout);

/*
	//printing stats
	// remember 1 B/ms == 1KB / sec
	std::cout<<"Printing stats: for "<<size<<" urls"<<std::endl;
	std::cout<<"--------------"<<std::endl;
	std::cout<<DVAL(time_to_load) 	<< "ms,  Bandwidth= "<< double(size/time_to_load)*8/1024  <<" Mb/s"
			<< "  average/url="<< double(time_to_load/size) 	<<"ms"<< STDENDL;
	std::cout<<"Online compression: on "<<howmanytocode << " urls" << STDENDL;
	std::cout<<" "<<DVAL(time_to_encode) << "ms, Bandwidth= "<< double(size/time_to_encode)*8/1024 <<" Mb/s"
			<< "  average/url="<< double(time_to_encode/howmanytocode) <<"ms"<< STDENDL;
	std::cout<<" "<<DVAL(time_to_decode )<< "ms, Bandwidth= "<< double(size/time_to_decode)*8/1024 <<" Mb/s"
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
