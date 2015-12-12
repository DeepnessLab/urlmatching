/*
 * FileCompressor.h
 *
 *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  Basic module to compress textfile containing urls using urlmatching framework
 *
 */
#ifndef URLTOOLKIT_FILECOMPRESSOR_H_
#define URLTOOLKIT_FILECOMPRESSOR_H_

#include "UrlMatching.h"


class FileCompressor {
public:
	FileCompressor() {}
	virtual ~FileCompressor() {}

	static bool compress(std::string& text_filename, std::string& compressed_filename);

	static bool compress(std::string& text_filename, std::string& compressed_filename
			, DoubleHeavyHittersParams_t& params, bool split_for_LPM, std::string delimiter="/");
	static bool extract(std::string& compressed_filename, std::string& extracted_filename);
private:
	static void resetbuff(uint32_t* buf, uint32_t size ) {
		for (uint32_t i=0; i< size; i++) {
			buf[i]= 0xEEEEEEEE;
		}
	}
};


#endif /* URLTOOLKIT_FILECOMPRESSOR_H_ */
