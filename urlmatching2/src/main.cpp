/*
 * main.cpp
 *
 *  Created on: 1 2014
 *      Author: Daniel
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "tester.h"
#include "UrlDictionay.h"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP



int main()
{

	std::string path;
	getWorkingDirectory(path);
	std::cout<<"running from path="<<path<<std::endl;

	// Load configuration from file
	std::string logger_config_file("src/easylogging.conf");
	logger_config_file = path + "/" + logger_config_file;
    el::Configurations conf(logger_config_file.c_str());
    // Reconfigure single logger
//    el::Loggers::reconfigureLogger("default", conf);
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);
    // Now all the loggers will use configuration from file

////	test_huffman();
//	std::cout<<"starting now"<<std::endl;
//	test_aho_compressed();

//	RUN_TEST(test_aho_compressed);
//	RUN_TEST(test_url_matching);
//	RUN_TEST(test_url_dictionary);
//	RUN_TEST(test_LLH);
	RUN_TEST(test_url_dictionary_load_from_url_txt_file);
	std::cout<<"test ended"<<std::endl;



	return 0;
}
