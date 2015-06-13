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

LineIterator* LineIterator::getLineIterator(const char *filePath) {
	return (new LineIteratorFile(filePath));
}

LineIterator* LineIterator::getLineIterator(const char *filePath, const char delim) {
	return (new LineIteratorFile(filePath, delim));
}

LineIterator* LineIterator::getLineIterator(const std::deque<std::string> *deque) {
	return (new LineIteratorDeque(deque));
}


LineIteratorFile::LineIteratorFile(const char *filePath) {

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

LineIteratorFile::LineIteratorFile(const char *filePath, const char delim) {

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

LineIteratorFile::~LineIteratorFile() {
	_infile.close();
}
