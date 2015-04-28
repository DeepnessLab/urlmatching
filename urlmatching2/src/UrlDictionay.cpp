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
#include <bitset>
#include <assert.h>
#include "macros.h"


#define MAX_DB_LINE_SIZE 100
#define SEPERATOR ","

#define BITS_IN_BYTE 8
#define BITS_IN_UINT32_T (sizeof(uint32_t)*BITS_IN_BYTE)


UrlCompressor::UrlCompressor():_huffman(),_symbol2pattern_db(NULL),_is_loaded(false){
	_symbol2pattern_db_size=0;
}

UrlCompressor::~UrlCompressor() {
	if (_symbol2pattern_db!=NULL)
		delete _symbol2pattern_db;
	_symbol2pattern_db=NULL;
	// TODO Auto-generated destructor stub
}

void UrlCompressor::load_strings_and_freqs(Strings2FreqMap* strings_to_freq)
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

bool UrlCompressor::initFromUrlsListFile(std::string& file_path, bool contains_basic_symbols)
{
	//TODO: implement this
	return true;
}

/**
 * This method will load and create encode/decode DB from a saved DB file
 * @param file_path - DB file path
 * @return true if loaded successfully
 */
bool UrlCompressor::initFromStoredDBFile(std::string& file_path)
{
	std::string line;
	std::ifstream file(file_path.c_str());
	uint32_t symbol_counter=1;
	if (!file.is_open()) {
		return false;
	}
	char chars[MAX_DB_LINE_SIZE];
	if (!getline(file,line)) {
		return false;
	}
	strcpy(chars,line.c_str());
	init_db(atoi(chars)); //get number of lines to load

	//read all lines by the format <symbol#>,<frequency>,<pattern_string>\n
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
			patternStr.assign(p);
		} else
			return false;
		assert(symbol==symbol_counter);
		Pattern* patt = new Pattern(symbol,frequency,patternStr);
		_symbol2pattern_db[symbol]=patt;
		_strings_to_symbols[patternStr]=symbol;
		symbol_counter++;
	}
	//symbol_counter is last symbol number +1
	file.close();

	//prepare array to load huffman dictionary
	int* freqArr = new int[symbol_counter];
	for (uint32_t i=0; i<symbol_counter;i++)  {  //skip symbol 0
			Pattern* pat =_symbol2pattern_db[i];
			assert(pat->_symbol == i);
			freqArr[i]=pat->_frequency;
	}
	_huffman.load(freqArr,symbol_counter);
	delete freqArr;

	calculate_symbols_score();	//evaluate each symbol encoded length
//	init_pattern_matching_algorithm();
	algo.load_patterns(_symbol2pattern_db,_symbol2pattern_db_size);

	std::cout << "load_dict_from_file: loaded "<<symbol_counter<<" patterns"<<std::endl;
	return true;
}

//out_buf_size[0] is the length of coded bits (number of coded + 1)
UrlCompressorStatus UrlCompressor::encode(std::string url, uint32_t* out_encoded_buf, uint32_t out_buf_size) {
	assert (out_buf_size > 2);

	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}

	//find patterns cover over url
	symbolT result[MAX_URL_LENGTH];
	algo.find_patterns(url,result);


	uint32_t reset_mask = 1 << (BITS_IN_UINT32_T -1 );
	uint32_t mask = reset_mask;
	uint32_t& bit_counter = out_encoded_buf[0];
	bit_counter=0;
	uint32_t i = 1;	//idx in out_encoded_buf
	out_encoded_buf[i] = 0;

	//Huffman encode the symbols
	symbolT* symbol = result;
	while (*symbol != S_NULL) {
		HuffCode coded_symbol = _huffman.encode(*symbol);
		for (HuffCode::const_iterator it = coded_symbol.begin(); it != coded_symbol.end(); ++it) {
			if (*it == true) {
				out_encoded_buf[i] = out_encoded_buf[i] | mask;
			}
			bit_counter++;
			mask = mask / 2;	//move right
			if (mask == 0) {
				i++;
				if (i >= out_buf_size) {
					return STATUS_ERR_SMALL_BUF;
				}
				mask = reset_mask;
				out_encoded_buf[i] = 0;
			}
		}
		symbol++;
	}
	return STATUS_OK;
}

UrlCompressorStatus UrlCompressor::decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size) {
	assert (in_buf_size > 2);
	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}

	UrlBuilder urlbuilder(_symbol2pattern_db);
	uint32_t num_of_left_bits_to_read = in_encoded_buf[0];

	uint32_t most_left_bit = 1 << (BITS_IN_UINT32_T -1 );
	HuffCode huff_code;
	uint32_t i;
	for (i=1 /*[0] was number of coded bits*/; i < in_buf_size; i++) {
		uint32_t buf = in_encoded_buf[i];
		std::bitset<32> x(buf);
		std::cout<<"buf="<<x<<STDENDL;

		for (uint16_t j=0; j < BITS_IN_UINT32_T ; j++) {
			uint32_t bit = buf & most_left_bit;
			if (bit == 0) { 	// 0
				huff_code.push_back(false);
				std::cout<<0;
			} else {			// 1
				huff_code.push_back(true);
				std::cout<<1;
			}
			num_of_left_bits_to_read--;
			buf = buf << 1;	//shift left buf
			_huffman.printHuffCode(&huff_code);
			symbolT symbol = _huffman.decode(huff_code);
			if (symbol != S_NULL) {
				std::cout<<";";
				urlbuilder.append(symbol);
				huff_code.clear();
				if (num_of_left_bits_to_read == 0) {
					i = in_buf_size; 	//break outer for loop
					break; 				//break this loop
				}
			}

			if (num_of_left_bits_to_read == 0) {
				return STATUS_ERR_LOST_DECODED_DATA;
			}

		}
	}
	std::cout<<STDENDL;
	urlbuilder.print();
	url = urlbuilder.get_url();

	return STATUS_OK;
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
		std::cout<<", pattern=" << ptrn->_str <<std::endl;
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
	if (!isLoaded()) {
		DELETE_AND_NULL(_symbol2pattern_db);
	}
	_symbol2pattern_db_size=size+1;
	_symbol2pattern_db = new Pattern*[size];
	//symbol=0 is used to represent "strings" of symbols
	_symbol2pattern_db[0]=new Pattern(0,0,"NULL");
	for (symbolT i=1; i<_symbol2pattern_db_size;i++)
		_symbol2pattern_db[i]=NULL;
	setLoaded();
}

void UrlCompressor::init_pattern_matching_algorithm() {
	algo.load_patterns(_symbol2pattern_db,_symbol2pattern_db_size);
}


UrlBuilder::UrlBuilder(Symbol2pPatternArr symbol2pattern_db) :
		_symbol2pattern_db(symbol2pattern_db),
		_url("")
{
	_symbol_deque.empty();
}


void UrlBuilder::append (symbolT symbol) {
	_symbol_deque.push_back(symbol);
	_url.append(_symbol2pattern_db[symbol]->_str);
}

void UrlBuilder::print() {
	std::cout << "UrlBuilder::print " << DVAL(_url)<<STDENDL;
	std::cout << "string: " ;
	for (SymbolDeque::iterator it = _symbol_deque.begin(); it != _symbol_deque.end(); ++it) {
		std::cout << _symbol2pattern_db[*it]->_str << ";";
	}
	std::cout << STDENDL;
	std::cout << "symbols: " ;
	for (SymbolDeque::iterator it = _symbol_deque.begin(); it != _symbol_deque.end(); ++it) {
		std::cout << *it << ";";
	}
	std::cout << STDENDL;
}
