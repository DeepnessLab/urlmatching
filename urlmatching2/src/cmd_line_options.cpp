/* 
 * File:   cmd_line_parser.cpp
 * Author: golanp
 * 
 * Created on December 7, 2013, 4:46 PM
 * Edited by Michal
 * Added the option of string use.
 */

#include <stdlib.h>
#include <getopt.h>
#include "cmd_line_options.h"


CmdLineOptions::CmdLineOptions(int argc, char* argv[]) {

	this->output_file_path		= "output.txt";
	this->add_header_to_output_file	= false;
	this->kgram_size            = 8;
	this->n1                    = 1000;
	this->n2                    = 1000;
	this->r                     = 0.8;
	this->test_decoding			= false;
	this->split_for_LPM		= false;
	this->print_dicionary		= false;
	this->dicionary_output_file = "";
	this->line_del = '\n';
	this->break_time 			= 0;
	this->logger_config			= "";
	this->_parse_command_line(argc, argv);
}

void CmdLineOptions::_parse_command_line(int argc, char* argv[]) {
	int o;

	if (argc < 2) {
		this->usage();
		exit(0);
	}

	while (-1 != (o = getopt(argc, &argv[0], "f:o:ak:1:2:r:lp:b:dc:"))) {
		switch (o) {
		case 'f':
			this->input_urls_file_path = optarg;
			break;
		case 'o':
			this->output_file_path = optarg;
			break;
		case 'a':
			this->add_header_to_output_file = true;
			break;
		case 'k':
			this->kgram_size = atoi(optarg);
			break;
		case '1':
			this->n1 = atoi(optarg);
			break;
		case '2':
			this->n2 = atoi(optarg);
			break;
		case 'r':
			this->r = atof(optarg);
			break;
		case 'l':
			this->split_for_LPM = true;
			break;
		case 'p':
			this->print_dicionary = true;
			this->dicionary_output_file = optarg;
			break;
		case 'b':
			this->break_time = atoi(optarg);
			break;
		case 'd':
			this->test_decoding = true;
			break;
		case 'c':
			this->logger_config = optarg;
			break;
		case 'h':
			this->usage();
			exit(0);
		default:
			this->usage();
			exit(0);
		}
	}

}

void CmdLineOptions::PrintParameters(std::ostream& log){
	std::string break_time_string = (this->break_time > 0)? (", \"break_time\": " + this->break_time ): "";
	std::string test_decoding = (this->test_decoding)?", \"Verify by Decode\"" : "";
	log<< "parameters={"
			<< "input file: \"" <<this->input_urls_file_path
			<< "\", kgram_size: " << this->kgram_size
			<< ", n1: " << this->n1
			<< ", n2: " << this->n2
			<< ", r: " << this->r
			<< break_time_string
			<< test_decoding
			<< "}"
			<< std::endl;
}


void CmdLineOptions::usage() {
	std::cout << "Usage: urlcompressor -f <urls_path> [options]" << std::endl
			<< std::endl
			<< "                -f String[urls filepath]  - required" << std::endl
			<< "                -o String[ouput filepath], default: output.txt " << std::endl
			<< "                -a       [add header to output_filepath], default: None " << std::endl
			<< "                -k Int   [k-gram size], default: 8" << std::endl
			<< "                -1 Int   [heavy hitters count for HH1], default: 1000" << std::endl
			<< "                -2 Int   [heavy hitters count for HH2], default: 1000" << std::endl
			<< "                -r fload [consecutive k-gram ratio], default: 0.8" << std::endl
			<< "                -l       [Longest Prefix Match - split dictionary by /], default: false" << std::endl
			<< "                -p String[Print dictionary file path], default: None" << std::endl
			<< "                -b Int   [Take break time to measure program memory, Seconds], default: no" << std::endl
			<< "                -d       [Verify by Decode - longer], default: no" << std::endl
			<< "                -c String[logger config file], default: None " << std::endl
			<< std::endl
			<< "        -h prints this message" << std::endl;
}

