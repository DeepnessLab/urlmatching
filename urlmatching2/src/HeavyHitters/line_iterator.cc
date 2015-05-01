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
    std::cerr << "Failed to open file: path=" << filePath << std::endl;
    exit(1);
  }
  _nextBuffer.ptr  = NULL;
  _nextBuffer.size = 0;
}

LineIterator::~LineIterator() {
	_infile.close();
}
