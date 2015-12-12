/* 
 * File:   cmd_line_parser.cpp
 * Created on: 18 December 2014
 *     Author: Daniel Krauthgamer
 *
 * define the CmdLineOptions class, parsing the cmd parameters
 */

//#define _GLIBCXX_USE_C99 1

#include <stdlib.h>
#include <getopt.h>
#include "cmd_line_options.h"


CmdLineOptions::CmdLineOptions(int argc, char* argv[]) {

	this->custom_output_file	= false;
	this->output_file_path		= "output.txt";
	this->use_dictionary_file	= false;
	this->dictionary_file		= "";
	this->add_header_to_output_file	= false;
	this->kgram_size            = 8;
	this->n1                    = 1500;
	this->n2                    = 1000;
	this->r                     = 0.8;
	this->test_decoding			= false;
	this->split_for_LPM			= false;
	this->LPM_delimiter			= "/";
	this->print_dicionary		= false;
	this->print_dicionary_file 	= "";
	this->dump_ac_statetable	= false;
	this->dump_ac_statetable_filename = "";
	this->line_del 				= '\n';
	this->break_time 			= 0;
	this->logger_config			= "";
	this->factor				= 1;
	this->_parse_command_line(argc, argv);
}

void CmdLineOptions::_parse_command_line(int argc, char* argv[]) {
	int o;

	if (argc < 2) {
		this->usage();
		exit(0);
	}

	this->cmd = argv[1];

	if (!this->cmd_ok()) {
		if (this->cmd == "-h") {
			usage();
		} else {
			std::cout<<"Unknown CMD, use -h for help"<<std::endl;
		}
		exit(0);
	}

	while (-1 != (o = getopt(argc-1, &argv[1], "f:d:o:ak:n:r:l:p:b:vc:x:s"))) {
		switch (o) {
		case 'f':
			this->input_urls_file_path = optarg;
			break;
		case 'd':
			this->use_dictionary_file=true;
			this->dictionary_file = optarg;
			break;
		case 'o':
			this->custom_output_file=true;
			this->output_file_path = optarg;
			break;
		case 'a':
			this->add_header_to_output_file = true;
			break;
		case 'k':
			this->kgram_size = atoi(optarg);
			break;
		case 'n':
			this->n2 = atoi(optarg);
			this->n1 = this->n2 + 1000;
			break;
		case 'r':
			this->r = atof(optarg);
			break;
		case 'l':
			this->split_for_LPM = true;
//			char deli[2];
//			deli = optarg[0];
			this->LPM_delimiter = optarg[0];
			break;
		case 'p':
			this->print_dicionary = true;
			this->print_dicionary_file = optarg;
			break;
		case 'b':
			this->break_time = atoi(optarg);
			break;
		case 'v':
			this->test_decoding = true;
			break;
		case 'c':
			this->logger_config = optarg;
			break;
		case 'x':
			this->factor = atoi(optarg);
			break;
		case 's':
			this->dump_ac_statetable = true;
			break;
		case 'h':
			this->usage();
			exit(0);
		default:
			this->usage();
			exit(0);
		}
	}

	if (this->factor < 1) {
		std::cout<<"Error: -x " << this->factor <<" is lower than 1"<<std::endl;
		exit(0);
	}

	if (this->dump_ac_statetable) {
		this->dump_ac_statetable_filename=this->input_urls_file_path;
		this->dump_ac_statetable_filename+=".n" + std::to_string(this->n2);
		this->dump_ac_statetable_filename+=".k" + std::to_string(this->kgram_size);
		this->dump_ac_statetable_filename+=".r" + std::to_string(this->r).substr(0,4);
	}
	//TODO: verify arguments by cmd

}


bool CmdLineOptions::cmd_ok() {
	if ( (this->cmd == CMD_FULLTEST) ||
			(this->cmd == CMD_BUILDDIC) ||
			(this->cmd == CMD_ENCODE) ||
			(this->cmd == CMD_HASHTABLE) ||
			(this->cmd == CMD_COMPRESS) ||
			(this->cmd == CMD_EXTRACT) ||
			(this->cmd == CMD_ARTICLE)
	)
	{
		return true;
	}
	return false;
}

void CmdLineOptions::PrintParameters(std::ostream& log){
	std::string break_time_string = (this->break_time > 0)? (", \"break_time\": " + this->break_time ): "";
	std::string test_decoding = (this->test_decoding)?", \"Verify by Decode\"" : "";
	std::string delimiter = (this->split_for_LPM)? ", \"Component by "+this->LPM_delimiter+"\"" : "";
	log<< "parameters={"
			<< "input file: \"" <<this->input_urls_file_path
			<< "\", kgram_size: " << this->kgram_size
			<< ", n1: " << this->n1
			<< ", n2: " << this->n2
			<< ", r: " << this->r
			<< break_time_string
			<< test_decoding
			<< delimiter
			<< "}"
			<< std::endl;
}


void CmdLineOptions::usage() {
	std::cout << std::endl;
	std::cout << "Usage: UrlMatchingTester [CMD] [-f urls_path] <options> <compression params>" << std::endl
			<< std::endl
			<< " 1. testing CMD: test, ,testhash, article"<<std::endl
			<< "    -f [String] urls filepath  - required" << std::endl
			<< "    -o [String] ouput filepath		, default: None " << std::endl
			<< "    -a          add header to output_filepath	, default: None " << std::endl
			<< "    -p [String] Print dictionary file path	, default: None" << std::endl
			<< "    -v          Verify by Decode - longer	, default: no" << std::endl
			<< "    -b [Int]    Take break time to measure program memory, Seconds, default: no" << std::endl
			<< std::endl
			<< " 2. Building dictionary and encoding input file using existing dictionary "<<std::endl
			<< "    CMD: build, encode"<<std::endl
			<< "    All of the above flags and:" << std::endl
			<< "    -d [String] dicionary filepath	, default: \"[urlsfile].dict\" " << std::endl
			<< "    -x 0 To skip Offline stage, this yelp wrong compression ratio, default: 1 " << std::endl
			<< std::endl
			<< " 3. compress file CMD: compress, extract -f [input file] -o [output file] <compression params> "<<std::endl
			<< std::endl
			<< " Compression params: "<<std::endl
			<< " ------------------ "<<std::endl
			<< "    -l [char]    Longest Prefix Match - split dictionary by /, default: false" << std::endl
			<< "    -k [Int]     k-gram size			, default: 8" << std::endl
			<< "    -r [Fload]   consecutive k-gram ratio	, default: 0.8" << std::endl
			<< "    -n [Int]     number of pattern anchors	, default: 1000" << std::endl
			<< " Debugging flags:" << std::endl
			<< " ---------------  "<<std::endl
			<< "    -s          dump Aho Corasik state machine	, default: No" << std::endl
			<< "    -c [String] logger config file		, default: None " << std::endl
			<< std::endl
			<< "    -h prints this message" << std::endl;
}

std::string CmdLineOptions::getDictionaryFilename() {
	std::string ret;
	if (!use_dictionary_file) {
		ret = input_urls_file_path + ".dict";
	} else {
		ret = dictionary_file;
	}
	return ret;
}

