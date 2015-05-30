/* 
 * Author: Michal
 * 
 * Created on August 11, 2014, 6:00 PM
 */

#include "line_iterator.h"
#include <stdlib.h>
#include <iostream>

#include <iomanip>

using std::ifstream;

LineIterator::LineIterator(const char *filePath) {

	_infile.open(filePath);
	if (!_infile.is_open()){
		_isFileOpenedSuccessfully = false;
		std::cerr << "Failed to open file: path=" << filePath << std::endl;
	}
	_isFileOpenedSuccessfully = true;
	_nextBuffer.ptr  = NULL;
	_nextBuffer.size = 0;
	_delim = NULL_DELIMITER;
}

LineIterator::LineIterator(const char *filePath, const char delim) {

	_infile.open(filePath, std::ifstream::in);
	if (!_infile.is_open()){
		_isFileOpenedSuccessfully = false;
		std::cerr << "Failed to open file: path=" << filePath << std::endl;
	}
	_isFileOpenedSuccessfully = true;
	_nextBuffer.ptr  = NULL;
	_nextBuffer.size = 0;
	_delim = delim;
}

LineIterator::~LineIterator() {
	_infile.close();
}
