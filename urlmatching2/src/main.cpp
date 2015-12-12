/*
 * main.cpp
 *
 *  Created on: 18 December 2014
 *      Author: Daniel Krauthgamer
 *
 *  The system contains a centralized logging infrastructure, located in logger.h
 *  Debug logs are enabled only when DEBUG_BUILD label is set.
 *
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "UrlToolkit/Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "tester.h"
#include "UrlToolkit/UrlMatching.h"
#include "cmd_line_options.h"
#include "logger.h"


#if IS_EASYLOGGING_DEFINED
INITIALIZE_EASYLOGGINGPP
#endif

void init_easy_logging(std::string configuration_file_path)
{
#if IS_EASYLOGGING_DEFINED

	// Load configuration from file
	std::string logger_config_file(configuration_file_path.c_str());
    el::Configurations conf(configuration_file_path.c_str());
    // Reconfigure single logger
//    el::Loggers::reconfigureLogger("default", conf);
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);

    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    // Now all the loggers will use configuration from file
#endif
    return;
}


int main(int argc, char* argv[])
{

	std::string path;
	getWorkingDirectory(path);

	CmdLineOptions options(argc, argv);
	if (options.logger_config == "") {
		options.logger_config = path + "/src/easylogging.conf";
	}

	init_easy_logging(options.logger_config);

    clock_t begin,end;
    begin = std::clock(); ;
	run_cmd_main(options);
	end = std::clock();;
	double test_time = double(end - begin) / (CLOCKS_PER_SEC);
	std::cout<<"TEST ENDED, took " << test_time / 60 << " min."<< std::endl;

	return 0;
}
