/*
 * FileCompressor.h
 *
 *  Created on: 7 αιεμ 2015
 *      Author: Daniel
 */

#ifndef URLTOOLKIT_FILECOMPRESSOR_H_
#define URLTOOLKIT_FILECOMPRESSOR_H_

#include "UrlDictionay.h"


class FileCompressor {
public:
	FileCompressor();
	virtual ~FileCompressor();

	static bool compress(std::string& text_filename, std::string& compressed_filename);

	static bool compress(std::string& text_filename, std::string& compressed_filename
			, HeavyHittersParams_t& params, bool split_for_LPM);
	static bool extract(std::string& compressed_filename, std::string& extracted_filename);
private:
	static void resetbuff(uint32_t* buf, uint32_t size ) {
		for (uint32_t i=0; i< size; i++) {
			buf[i]= 0xEEEEEEEE;
		}
	}
};


#endif /* URLTOOLKIT_FILECOMPRESSOR_H_ */
