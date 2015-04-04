/*
 * UrlDictionay.cpp
 *
 *  Created on: 1 бреб 2014
 *      Author: Daniel
 */

#include "UrlDictionay.h"
#include <fstream>
#include <iostream>
#include <exception>
#include <map>
#include <assert.h>
#include "macros.h"


#define MAX_DB_LINE_SIZE 100
#define SEPERATOR ","


UrlCompressor::UrlCompressor():_huffman(),_symbol2pattern_db(NULL){
	_symbol2pattern_db_size=0;
}

UrlCompressor::~UrlCompressor() {
	if (_symbol2pattern_db!=NULL)
		delete _symbol2pattern_db;
	_symbol2pattern_db=NULL;
	// TODO Auto-generated destructor stub
}

void UrlCompressor::load_strings_and_freqs(Strings2FreqType* strings_to_freq)
{
	std::cout<<"Entered load_strings_and_freqs"<<std::endl;
	uint32_t size = strings_to_freq->size();
	int* myfreq = new int[size+1];
	_strings_to_symbols.clear();

	uint32_t ith_symbol = 0;
	std::cout<<"Print strings_to_freq:"<<std::endl;
	for (std::map<std::string,int>::iterator it=strings_to_freq->begin(); it!=strings_to_freq->end(); ++it) {
		std::cout << it->first << " freq => " << it->second << '\n';
		_strings_to_symbols.insert( std::pair<std::string,uint32_t>(it->first,ith_symbol) );
		myfreq[ith_symbol]=  it->second;
		ith_symbol++;
	}
	std::cout << std::endl;

	std::cout<<"Print _strings_to_symbols:"<<std::endl;
	for (std::map<std::string,uint32_t>::iterator it=_strings_to_symbols.begin(); it!=_strings_to_symbols.end(); ++it) {
		std::cout << it->first << " symbol => " << it->second << '\n';
		_strings_to_symbols.insert( std::pair<std::string,uint32_t>(it->first,ith_symbol) );
	}

	_huffman.load(myfreq,size);

}

bool UrlCompressor::initFromUrlsListFile(std::string& file_path, bool contains_basic_symbols) {
	//TODO: implement this
	return true;
}
bool UrlCompressor::initFromStoredDBFile(std::string& file_path) {
	std::string line;
	std::ifstream file(file_path.c_str());
	uint32_t symbol_counter=1;
	if (file.is_open()) {
		char chars[MAX_DB_LINE_SIZE];
		if (!getline(file,line)) {
			return false;
		}
		strcpy(chars,line.c_str());
		init_db(atoi(chars));


		while (getline(file,line)) {
			uint32_t symbol=0;
			std::string patternStr;
			uint32_t frequency=0;
			if (line.length()> MAX_DB_LINE_SIZE)
				return false;
			strcpy(chars,line.c_str());
			char *p = strtok(chars, SEPERATOR);
			if (p) {
				symbol= atoi(p);
				assert(symbol!=0); //0 is saved for S_NULL
				p = strtok(NULL, SEPERATOR);
			} else
				return false;
			if (p) {
				frequency = atoi(p);
				p = strtok(NULL, SEPERATOR);
			} else
				return false;
			if (p) {
				patternStr=p;
			} else
				return false;
			assert(symbol==symbol_counter);
			Pattern* patt = new Pattern(symbol,frequency,patternStr);
			_symbol2pattern_db[symbol]=patt;
			_strings_to_symbols[patternStr]=symbol;
			symbol_counter++;
		}
		symbol_counter--;
		file.close();

		//prepare array to load huffman dictionary
		int* freqArr = new int[symbol_counter];
		for (uint32_t i=1; i<=symbol_counter;i++)  {  //skip symbol 0
				Pattern* pat =_symbol2pattern_db[i];
				assert(pat->_symbol == i);
				freqArr[i-1]=pat->_frequency;
		}
		_huffman.load(freqArr,symbol_counter);
		delete freqArr;

		calculate_symbols_score();
		init_pattern_matching_algorithm();

		std::cout << "load_dict_from_file: loaded "<<symbol_counter<<" patterns"<<std::endl;
		return true;
	}

	return false;
}

void UrlCompressor::print_database(bool print_codes) {
	std::cout<<"UrlCompressor::print_database"<<std::endl;
	std::cout<<"UrlCompressor db contains "<<_symbol2pattern_db_size<< " patterns:"<<std::endl;
	for (uint32_t i=0; i<_symbol2pattern_db_size;i++) {
//	for (Symbol2PatternType::iterator it=_symbol2pattern_db.begin(); it!=_symbol2pattern_db.end(); ++it) {
		Pattern* ptrn = _symbol2pattern_db[i];
		std::cout << "symbol=" << ptrn->_symbol << ",freq=" << ptrn->_frequency
				<<",str_size="<<ptrn->getStringLength()
				<<",huff_len="<<ptrn->getHuffmanLength();
		if (print_codes) {
			HuffCode code = _huffman.encode(ptrn->_symbol);
			std::cout<<",code=";
			std::copy(code.begin(), code.end(), std::ostream_iterator<bool>(std::cout));
		}
		std::cout<<",pattern=" << ptrn->_str <<std::endl;
	}
}

void UrlCompressor::print_strings_and_codes() {
	std::cout<<"Entered print_strings_and_codes:"<<std::endl;
	std::cout<<"Print strings and their huffman codes:"<<std::endl;
	for (std::map<std::string,uint32_t>::iterator it=_strings_to_symbols.begin(); it!=_strings_to_symbols.end(); ++it) {
		std::cout << it->first << " => symbol:" << it->second << "\tcode: ";
		HuffCode code = _huffman.encode(it->second);
		std::copy(code.begin(), code.end(), std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}

}

Pattern::Pattern(uint32_t symbol, uint32_t frequency, std::string str) : _str(str) {
	_symbol=symbol;
	_frequency=frequency;
	_huffman_length=UINT32_MAX;

}

void UrlCompressor::calculate_symbols_score() {
	for (symbolT i=0; i<_symbol2pattern_db_size;i++) {
//	for (Symbol2PatternType::iterator iter=_symbol2pattern_db.begin(); iter!=_symbol2pattern_db.end();++iter) {
		HuffCode code=_huffman.encode( _symbol2pattern_db[i]->_symbol );
		_symbol2pattern_db[i]->_huffman_length=code.size();
	}
}

void UrlCompressor::init_db(uint32_t size) {
	DELETE_AND_NULL(_symbol2pattern_db);
	_symbol2pattern_db_size=size+1;
	_symbol2pattern_db = new Pattern*[size];
	//symbol=0 is used to represent "strings" of symbols
	_symbol2pattern_db[0]=new Pattern(0,0,"NULL");
	for (symbolT i=1; i<_symbol2pattern_db_size;i++)
		_symbol2pattern_db[i]=NULL;
}

void UrlCompressor::init_pattern_matching_algorithm() {
	algo.load_patterns(_symbol2pattern_db,_symbol2pattern_db_size);
}
