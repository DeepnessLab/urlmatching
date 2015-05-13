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

#include "tester.h"
#include "Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "UrlDictionay.h"
#include "HeavyHitters/dhh_lines.h"
#include "logger.h"

void test_url_dictionary_load_from_url_txt_file() {

	char pBuf[1000];
//	int bytes = GetModuleFileName(NULL, pBuf, 1000);
//	std::string path(pBuf, bytes);
	GetCurrentDir(pBuf, 1000);
	std::string path(pBuf);
	std::cout<<"running from path="<<path<<std::endl;

	int last_slash = path.find_last_of("/\\");
	int another = path.find_last_of("/\\",last_slash-1);
	path=path.substr(0,another+1);
	std::string urls_file = "test_files/9000_urls.txt";
	path = path + urls_file;

	std::cout<<"test file path="<<path<<std::endl;
	HeavyHittersParams_t customParams = {n1: 1000, n2: 1000, r: 0.5, kgrams_size: 4};
	HeavyHittersParams_t& params = customParams; //default_hh_params;

	UrlCompressor urlc;
	bool ret = urlc.LoadUrlsFromFile(urls_file, params, false);
	assert (ret);

	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	std::string my_string = "http://www.google.com/gmail/drive";
	std::cout<<"matching string="<<my_string<<std::endl;

//	symbolT result[1000];
//	urlc.algo.find_patterns(my_string,result);

	std::cout<<"encode string="<<my_string<<std::endl;
	uint32_t buff_size = 100;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode(my_string,codedbuff,buff_size);

	std::string decoded_str;
	urlc.decode(decoded_str,codedbuff,100);
	std::cout<<"dencoded string="<<decoded_str<<std::endl;

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
	LDHH ldhh(urls_file, 1000, 1000, 0.1, 8 );
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
	/*
	 * LDHH peace_dhh(options.peace_pcap_file_path, options.n1, options.n2, options.r, options.kgram_size);
    peace_dhh.run();
    STOPWATCH_MESSURE2("** peace time traffic DHH", utils::run_logfile);

    std::list<signature_t>& peace_signatures = peace_dhh.get_signatures();
    size_t                  pckt_count       = peace_dhh.get_pckt_count();
    std::cout << "** scanned " << pckt_count << " packets" << std::endl << std::endl;
    utils::run_logfile << "** scanned " << pckt_count << " packets" << std::endl << std::endl;

    std::string file_path = utils::log_dir + "/peace_signatures";
    utils::signatures_to_file(file_path, peace_signatures, pckt_count);
    std::cout << "** stored " << peace_signatures.size() << " peace signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
    utils::run_logfile << "** stored " << peace_signatures.size() << " peace signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
    std::list<signature_t> white_list;
    std::list<signature_t> maybe_white_list;

    _categorize_peace_signatures(peace_signatures, pckt_count,
                                 options.peace_high_threshold, options.peace_low_threshold,
                                 white_list, maybe_white_list);

    file_path = utils::log_dir + "/white_list_signatures";
    utils::signatures_to_file(file_path, white_list, pckt_count);
    std::cout << "** stored " << white_list.size() << " white signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
    utils::run_logfile << "** stored " << white_list.size() << " white signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
    file_path = utils::log_dir + "/maybe_white_list_signatures";
    utils::signatures_to_file(file_path, maybe_white_list, pckt_count);
    std::cout << "** stored " << maybe_white_list.size() << " maybe white signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
    utils::run_logfile << "** stored " << maybe_white_list.size() << " maybe white signatures in: " << relpath_to_abspath(file_path) << std::endl << std::endl;
	 *
	 */

}


void test_url_dictionary_load_from_stored_DB() {

//	UrlCompressor dict;
//	build_simple_dict(&dict);

	std::string patterns_file = "d:\\Temp\\project\\patterns_abc.txt";
	std::string my_string = "shaloma";

	UrlCompressor urlc;
	bool ret = urlc.LoadStoredDBFromFiled(patterns_file);
	assert (ret);
	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

	std::cout<<"matching string="<<my_string<<std::endl;

	symbolT result[1000];
	urlc.algo.find_patterns(my_string,result);

	my_string = "shalomashalomashalomashalomashaloma";
	std::cout<<"encode string="<<my_string<<std::endl;
	uint32_t buff_size = 100;
	uint32_t* codedbuff = new uint32_t[buff_size];
	urlc.encode(my_string,codedbuff,buff_size);

	std::string decoded_str;
	urlc.decode(decoded_str,codedbuff,buff_size);
	std::cout<<"dencoded string="<<decoded_str<<std::endl;

	delete codedbuff;
//	exit(1);

}




void test_url_matching() {

//	UrlCompressor dict;
//	build_simple_dict(&dict);

	std::string patterns_file = "d:\\Temp\\project\\patterns_abc.txt";
	std::string my_string = "shaloma";

	UrlCompressor urlc;
	bool ret = urlc.LoadStoredDBFromFiled(patterns_file);
	assert (ret);
	urlc.print_database(true /*print codes*/);

	std::cout<<" -------> finished loading <------- "<<std::endl<<std::endl;

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
