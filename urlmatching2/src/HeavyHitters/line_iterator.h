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
#include <deque>
#include "common.h"

using std::ifstream;

#define NULL_DELIMITER 0xff
#define ENDL_DELIMITER '\n'

class LineIteratorDeque;
class LineIteratorFile;


//Abstract class & Factory, use get methods
class LineIterator {
public:
	LineIterator() {}
	virtual ~LineIterator() {}
	virtual bool has_next() =0;
	virtual const raw_buffer_t& next() =0;
	virtual bool canRun() =0;

	static LineIterator* getLineIterator(const char *filePath) ;
	static LineIterator* getLineIterator(const char *filePath, const char delim) ;
	static LineIterator* getLineIterator(const std::deque<std::string> *deque) ;

};

class LineIteratorDeque : public LineIterator {
public:
	LineIteratorDeque(const std::deque<std::string>* deque) : _deque(deque){
		if (_deque != NULL) {
			_iter = _deque->begin();
		}
	}
	virtual ~LineIteratorDeque() {}

	virtual inline bool canRun() { return (_deque != NULL) ; }

	virtual inline bool has_next() { return (_iter != _deque->end() ); }

	virtual inline const raw_buffer_t& next() {
		this->_nextBuffer.ptr  =  (unsigned char *) _iter->c_str();
		this->_nextBuffer.size = _iter->size();
		_iter++;
		return this->_nextBuffer;
	}


private:
	const std::deque<std::string>* _deque;
	std::deque<std::string>::const_iterator _iter;
	raw_buffer_t  _nextBuffer;
};

class LineIteratorFile : public LineIterator {
    
public:
    LineIteratorFile(const char *filePath);
    
    LineIteratorFile(const char *filePath, const char delim);

    virtual ~LineIteratorFile();
    
    //Note: Only call has_next() after finished processing the buffer returned by a previous call to next().
    //      This is done to reduce memory allocations.
    virtual inline bool has_next() {
      return (_infile.eof() == false);
    }

    virtual inline const raw_buffer_t& next() {
      char delim = _delim; //0xff;	//TODO: which delimiter should be used ? NULL or '\n'
      if (std::getline((this->_infile), (this->line), delim) != 0){
        this->_nextBuffer.ptr  =  (unsigned char *) this->line.c_str();
        this->_nextBuffer.size = this->line.size();
       
      }	
      return this->_nextBuffer;		
   		
    }

    virtual inline bool canRun() { return _isFileOpenedSuccessfully; }
	inline char getDelimiter() const { return _delim; }
	inline void setDelimiter(char delim = NULL_DELIMITER) { _delim = delim; }

private:

	ifstream _infile;
	std::string line;
//	const unsigned char *_lineData;
	raw_buffer_t        _nextBuffer;
	char _delim = 0xff;
	bool _isFileOpenedSuccessfully;
};

#endif	/* LINEITERATOR_H */

