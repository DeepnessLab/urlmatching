/*
 * FileCompressor.cpp
 *
 *  Created on: 7 αιεμ 2015
 *      Author: Daniel
 */

#include <fstream>
#include <iostream>

#include "FileCompressor.h"
#include "UrlDictionay.h"


#define BUFFSIZE 255
#define BAD1BAD1 0xBAD1BAD1

#define MAGICSTR "ENDOFFILE"

static uint32_t hex_verify = 0xAAAAFFFF;

FileCompressor::FileCompressor() {
	// TODO Auto-generated constructor stub

}

bool
FileCompressor::compress(std::string& text_filename, std::string& compressed_filename) {
	return compress(text_filename, compressed_filename, default_hh_params, false);
}

bool
FileCompressor::compress(std::string& text_filename, std::string& compressed_filename, HeavyHittersParams_t& params, bool split_for_LPM)
{

	/*Format Dictionary|Num Of Urls in file|
	 * |(Byte)Bufflength|encoded buffer|(Byte)Bufflength|encoded buffer|..
	 * |magic|EOF
	 */

	UrlCompressor urlc;

	std::deque<std::string> url_deque;
	if (! urlc.getUrlsListFromFile(text_filename, url_deque)) {
		std::cout<<"Error with input file"<<STDENDL;
		return false;
	}
	if (url_deque.size() == 0) {
		std::cout<<"ERROR: read 0 urls from file"<<STDENDL;
		return false;
	}
	std::deque<std::string>* input_for_urlcompressor = &url_deque;

	if (split_for_LPM) {
		std::deque<std::string>* splitted_deque = new std::deque<std::string>;
		urlc.SplitUrlsList(url_deque, *splitted_deque);
		input_for_urlcompressor = splitted_deque;
	}

	bool ret = urlc.InitFromUrlsList(*input_for_urlcompressor, params, false);
	if (!ret)
		return false;

//	sanityTesting(urlc);
	uint32_t dict_size = urlc.getDictionarySize();
	std::cout<<"dict_size="<<dict_size<<STDENDL;

	std::ofstream file (compressed_filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file.is_open()) {
		std::cout<<"Error: Failed to open " << compressed_filename <<" for writing " << std::endl;
		return false;
	}

	//step A: store compression dictionary
	ret = urlc.StoreDictToFileStream(file);
	if (!ret) {
		std::cout<<"Failed to store to " << compressed_filename <<std::endl;
		return false;
	}

	char* mem_block;

	//step B: store number of urls
	uint32_t num_of_urls = url_deque.size() ;
	mem_block = (char *) &num_of_urls;
	file.write(mem_block,sizeof(num_of_urls));

	uint32_t lost_bytes = 0;

	//step C: store all urls
	uint32_t buff_length = BAD1BAD1;
	uint32_t codedbuff[BUFFSIZE];
	for (uint32_t i = 0 ; i < num_of_urls; i++ ) {
		buff_length = BUFFSIZE;
		resetbuff(codedbuff,BUFFSIZE);
		urlc.encode_2(url_deque[i],codedbuff,buff_length);
		if (buff_length > BUFFSIZE) {
			std::cout<< "ERROR: at line "<<i+1<<STDENDL;
			std::cout<< " buff length "<<buff_length<< " is bigger than "<<BUFFSIZE
					<< ", bit counter "<<codedbuff[0]<<STDENDL;
			std::cout<< "  "<<url_deque[i]<< STDENDL;
			continue;
		}
		lost_bytes+=( (buff_length-1) * sizeof(uint32_t) ) - conv_bits_to_bytes(codedbuff[0]);

		//size
		uchar size = buff_length;
		mem_block = (char *) &size;
		file.write(mem_block,sizeof(size));
		//buffer
		mem_block = (char *) &codedbuff[1];
		file.write(mem_block,sizeof(uint32_t) * (buff_length-1));

#ifdef BUILD_DEBUG
		//Test
		mem_block = (char *) &hex_verify;
		file.write(mem_block,sizeof(hex_verify));
#endif
	}

	std::string magic = MAGICSTR;
	file.write(magic.c_str(),magic.size());
	file.close();

	std::cout<< "Lost bytes "<<lost_bytes<< "Bytes = " << lost_bytes/1024 << " KB" << STDENDL;

	return true;
}


bool
FileCompressor::extract(std::string& compressed_filename, std::string& extracted_filename) {

	/*Format Dictionary|Num Of Urls in file|
	 * |(Byte)Bufflength|encoded buffer|(Byte)Bufflength|encoded buffer|..
	 * |magic|EOF
	 */

	std::ifstream file (compressed_filename.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	std::ofstream outfile;
	outfile.open (extracted_filename.c_str(),std::ofstream::out );
	if (!outfile.is_open()) {
		file.close();
		return false;
	}

	UrlCompressor urlc;
	bool ret = false;
	//step A: read compression dictionary
	ret = urlc.InitFromDictFileStream(file);
	if (!ret)
		return false;

	if (!urlc.sanity()) {
		std::cout<<"Failed sanity"<<STDENDL;
	}

	char* mem_block;

	//step B: read number of urls
	uint32_t num_of_urls = BAD1BAD1;
	mem_block = (char *) &num_of_urls;
	file.read(mem_block,sizeof(num_of_urls));

	uint32_t buff[BUFFSIZE];
	//step C: read all urls
	for (uint32_t i = 0; i < num_of_urls; i++) {
		//read buff size
		uchar size = 0;
		mem_block = (char *) &size ;
		file.read(mem_block,sizeof(size));
		uint32_t bufflength = size;
		//read buffer
		mem_block = (char *) buff;
		file.read(mem_block,sizeof(uint32_t) * bufflength);
		//decode
		std::string text;
		urlc.decode(text,buff,bufflength);
		//write to file
		outfile << text << std::endl;

#ifdef BUILD_DEBUG
		//Test
		uint32_t hex = BAD1BAD1;
		mem_block = (char *) &hex;
		file.read(mem_block,sizeof(hex_verify) );
		if (hex != hex_verify) {
			std::cout<< "ERROR at line "<<i+1<< ": hex="<<std::hex<<hex<< ", hex_verify="<<hex_verify<<std::dec<<<<STDENDL;
			outfile.close();
			file.close();
			return false;
		}
#endif

	}

	std::string magic = MAGICSTR;
	char cbuf[20] = "01234567890123456";
	file.read(cbuf,magic.size());

	if (strncmp(magic.c_str(),cbuf,magic.size()) != 0) {
		cbuf[19]='\0';
		std::cout<<"Error: magic was wrong, expected: "<<magic<<" ,found: " << cbuf<< STDENDL;
	}

	outfile.close();
	file.close();
	return true;
}

FileCompressor::~FileCompressor() {
	// TODO Auto-generated destructor stub
}

