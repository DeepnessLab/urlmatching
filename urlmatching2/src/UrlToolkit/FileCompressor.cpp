/*
 * FileCompressor.cpp
 *
 *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  See header file for comments.
 *
 */
#include <fstream>
#include <iostream>
#include <iomanip>

#include "FileCompressor.h"
#include "CodePack.h"


#define BUFFSIZE 255
#define BAD1BAD1 0xBAD1BAD1

#define MAGICSTR "ENDOFFILE"

#ifdef BUILD_DEBUG	//On debug , add hex_verify value between each coded url for easier hex viewing
static uint32_t hex_verify = 0xDEADBEEF;
#endif



inline uint16_t conv_char_size_to_uint32_size(uint32_t char_size) {
	uint16_t uint32_size = char_size / (sizeof(uint32_t));
	uint32_size =
			(char_size % (sizeof(uint32_t)) == 0) ?
					uint32_size : uint32_size + 1;
	return uint32_size;
}

void print_buf(std::ofstream& ofs, uint32_t* buff, uint32_t len) {
	ofs << buff[0] << "/0x" << std::hex << buff[0] << "-";
	ofs << std::setfill('0') << std::setw(2);
	for (uint32_t i = 1; i < len; i++) {
		unsigned char* c = (unsigned char*) &buff[i];
		ofs << std::setfill('0') << std::setw(2) << std::setfill('0')
				<< std::setw(2) << (int) *c << std::setfill('0') << std::setw(2)
				<< (int) *(c + 1) << std::setfill('0') << std::setw(2)
				<< (int) *(c + 2) << std::setfill('0') << std::setw(2)
				<< (int) *(c + 3) << ";";
	}
	ofs << std::setw(0) << std::dec << std::endl;
}


bool FileCompressor::compress(std::string& text_filename,
		std::string& compressed_filename) {
	return compress(text_filename, compressed_filename, default_hh_params,
			false /*, don't care about delimiter*/);
}

bool FileCompressor::compress(std::string& text_filename,
		std::string& compressed_filename, DoubleHeavyHittersParams_t& params,
		bool split_for_LPM, std::string delimiter) {

	/*Format Dictionary|(uint32_t)Num Of Urls in file|
	 * |(uint16_t)bits length|(Big endian) encoded buffer in Bytes|
	 * |(uint16_t)bits length|(Big endian) encoded buffer in Bytes|..
	 * |magic|EOF
	 */

	UrlMatchingModule urlc;

	std::deque<std::string> url_deque;
	if (!urlc.getUrlsListFromFile(text_filename, url_deque)) {
		std::cout << "Error with input file" << STDENDL;
		return false;
	}
	if (url_deque.size() == 0) {
		std::cout << "ERROR: read 0 urls from file" << STDENDL;
		return false;
	}
	std::deque<std::string>* input_for_urlcompressor = &url_deque;

	if (split_for_LPM) {
		std::deque<std::string>* splitted_deque = new std::deque<std::string>;
		urlc.SplitUrlsList(url_deque, *splitted_deque,delimiter);
		input_for_urlcompressor = splitted_deque;
	}

	bool ret = urlc.InitFromUrlsList(url_deque, *input_for_urlcompressor, params, false);
	if (!ret)
		return false;

//	sanityTesting(urlc);
	uint32_t dict_size = urlc.getDictionarySize();
	std::cout << "dict_size=" << dict_size << STDENDL;

	std::ofstream file(compressed_filename.c_str(),
			std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file.is_open()) {
		std::cout << "Error: Failed to open " << compressed_filename
				<< " for writing " << std::endl;
		return false;
	}

	//step A: store compression dictionary
	ret = urlc.StoreDictToFileStream(file);
	if (!ret) {
		std::cout << "Failed to store to " << compressed_filename << std::endl;
		return false;
	}

	char* mem_block;

	//step B: store number of urls
	uint32_t num_of_urls = url_deque.size();
	mem_block = (char *) &num_of_urls;
	file.write(mem_block, sizeof(num_of_urls));

	//step C: store all urls
	uint32_t buff_length = BAD1BAD1;
	uint32_t codedbuff[BUFFSIZE];
	for (uint32_t i = 0; i < num_of_urls; i++) {
		buff_length = BUFFSIZE;
		resetbuff(codedbuff, BUFFSIZE);
		urlc.encode(url_deque[i], codedbuff, buff_length);

		if (codedbuff[0] > UINT16_MAX) {
			std::cout << "ERROR at line " << i + 1 << ": bit counter "
					<< codedbuff[0] << " is bigger than " << UINT16_MAX
					<< STDENDL;
			std::cout<< "  "<<url_deque[i]<< STDENDL;
			continue;
		}

		uint16_t bit_size = codedbuff[0];

		//size
		mem_block = (char *) &bit_size;
		file.write(mem_block, sizeof(bit_size));

		//buffer
		//convert little endian to big endian
		if (!is_big_endian()) {
			conv_LE_BE(&codedbuff[1], buff_length);
		}
//		print_buf(codedbuff,buff_length);
		uint16_t bytes_to_write = conv_bits_to_bytes(bit_size);
//		std::cout<<"write "<<bytes_to_write<<" bytes for bit_size="<<bit_size<<std::endl;
		mem_block = (char *) &codedbuff[1];
		file.write(mem_block, bytes_to_write);
//		file.write(mem_block,sizeof(uint32_t) * (buff_length-1));

#ifdef BUILD_DEBUG
		//Test
		mem_block = (char *) &hex_verify;
		file.write(mem_block, sizeof(hex_verify));
#endif
	}


	std::string magic = MAGICSTR;
	file.write(magic.c_str(), magic.size());
	file.close();

//	std::cout<< "Lost bytes "<<lost_bytes<< "Bytes = " << lost_bytes/1024 << " KB" << STDENDL;

	if (!is_big_endian()) {
		std::cout << "!!!System is little endian!!!" << STDENDL;
	}

	return true;
}

bool FileCompressor::extract(std::string& compressed_filename,
		std::string& extracted_filename)
{

	/*Format Dictionary|(uint32_t)Num Of Urls in file|
	 * |(uint16_t)bits length|(Big endian) encoded buffer in Bytes|
	 * |(uint16_t)bits length|(Big endian) encoded buffer in Bytes|..
	 * |magic|EOF
	 */

	std::ifstream file(compressed_filename.c_str(),
			std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	std::ofstream outfile;
	outfile.open(extracted_filename.c_str(), std::ofstream::out);
	if (!outfile.is_open()) {
		file.close();
		return false;
	}

	UrlMatchingModule urlc;
	bool ret = false;
	//step A: read compression dictionary
	ret = urlc.InitFromDictFileStream(file);
	if (!ret)
		return false;

	if (!urlc.sanity()) {
		std::cout << "Failed sanity" << STDENDL;
	}

	char* mem_block;

	//step B: read number of urls
	uint32_t num_of_urls = BAD1BAD1;
	mem_block = (char *) &num_of_urls;
	file.read(mem_block, sizeof(num_of_urls));
	std::cout << "Reading " << num_of_urls << std::endl;

	uint32_t codedbuff[BUFFSIZE + 1];
	//step C: read all urls
	for (uint32_t i = 0; i < num_of_urls; i++) {
		//bit size
		uint16_t bit_size;
		mem_block = (char *) &bit_size;
		file.read(mem_block, sizeof(bit_size));
		codedbuff[0] = bit_size;
		//buffer
		uint16_t bytes_to_read = conv_bits_to_bytes(bit_size);
		mem_block = (char *) &codedbuff[1];
		uint32_t buff_length = conv_char_size_to_uint32_size(bytes_to_read) + 1;
		codedbuff[buff_length] = 0xEEEEEEEE;
		codedbuff[buff_length - 1] = 0;
		file.read(mem_block, bytes_to_read);

		//decode
		std::string text;
		//convert little endian to big endian
		if (!is_big_endian()) {
			conv_LE_BE(&codedbuff[1], buff_length);
		}
		urlc.decode(text, codedbuff, buff_length);
		//write to file
		outfile << text << std::endl;

#ifdef BUILD_DEBUG
		//Test
		uint32_t hex = BAD1BAD1;
		mem_block = (char *) &hex;
		file.read(mem_block, sizeof(hex_verify));
		if (hex != hex_verify) {
			std::cout << "ERROR at line " << i + 1 << ": hex=" << std::hex
					<< hex << ", hex_verify=" << hex_verify << std::dec
					<< STDENDL;
			outfile.close();
			file.close();
			return false;
		}
#endif

	}

	std::string magic = MAGICSTR;
	char cbuf[20] = "01234567890123456";
	file.read(cbuf, magic.size());

	if (strncmp(magic.c_str(), cbuf, magic.size()) != 0) {
		cbuf[19] = '\0';
		std::cout << "Error: magic was wrong, expected: " << magic
				<< " ,found: " << cbuf << STDENDL;
	}

	outfile.close();
	file.close();
	if (!is_big_endian()) {
		std::cout << "!!!System is little endian!!!" << STDENDL;
	}
	return true;
}

