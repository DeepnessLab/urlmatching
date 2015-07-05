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

#include "UrlToolkit/Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "tester.h"
#include "UrlToolkit/UrlDictionay.h"
#include "cmd_line_options.h"
#include "easylogging++.h"


#ifdef BUILD_DEBUG
INITIALIZE_EASYLOGGINGPP
#endif


int main(int argc, char* argv[])
{

	std::string path;
	getWorkingDirectory(path);
	//	std::cout<<"running from path="<<path<<std::endl;

	CmdLineOptions options(argc, argv);
	if (options.logger_config == "") {
		options.logger_config = path + "/src/easylogging.conf";
	}

#ifdef BUILD_DEBUG
	// Load configuration from file
	std::string logger_config_file(options.logger_config.c_str());
    el::Configurations conf(logger_config_file.c_str());
    // Reconfigure single logger
//    el::Loggers::reconfigureLogger("default", conf);
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);

    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    // Now all the loggers will use configuration from file
#endif

    clock_t begin,end;
    begin = std::clock(); ;
	run_cmd_main(options);
//	test_main(options);			//todo remove this
//	test_hashtable(options);	//todo remove this
	end = std::clock();;
	double test_time = double(end - begin) / (CLOCKS_PER_SEC);
	std::cout<<"TEST ENDED, took " << test_time / 60 << " min."<< std::endl;

	return 0;
}
