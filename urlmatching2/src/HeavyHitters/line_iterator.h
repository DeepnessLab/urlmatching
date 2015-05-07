/* 
 * File:   LineIterator.h
 * Author: Michal
 *
* Created on August 11, 2014, 6:00 PM

 *
 */

#ifndef LINEITERATOR_H
#define	LINEITERATOR_H

#include <fstream>
#include "common.h"

using std::ifstream;

class LineIterator {
    
public:
    LineIterator(const char *filePath);
    
    virtual ~LineIterator();
    
    //Note: Only call has_next() after finished processing the buffer returned by a previous call to next().
    //      This is done to reduce memory allocations.
    inline bool has_next() {
      return (_infile.eof() == false);
    }

    //inline set delim

    inline const raw_buffer_t& next() {
      char delim = '\n'; //0xff;	//TODO: which delimiter should be used ? NULL or '\n'
      if (std::getline((this->_infile), (this->line), delim) != 0){
        this->_nextBuffer.ptr  =  (unsigned char *) this->line.c_str();
        this->_nextBuffer.size = this->line.size();
       
      }	
      return this->_nextBuffer;		
   		
    }
    
private:
    
    ifstream _infile;
    std::string line;
    const unsigned char *_lineData;
    raw_buffer_t        _nextBuffer;
};

#endif	/* LINEITERATOR_H */

