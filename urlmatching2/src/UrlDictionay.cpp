/*
 * UrlDictionay.cpp
 *
 *  Created on: 1 бреб 2014
 *      Author: Daniel
 */

#include "UrlDictionay.h"
#include "HeavyHitters/dhh_lines.h"
#include <fstream>
#include <iostream>
#include <exception>
#include <map>
#include <bitset>
#include <assert.h>
#include "common.h"
#include "logger.h"

#define MAX_DB_LINE_SIZE 100
#define SEPERATOR ","
#define NULL_DEFAULT_FREQ 1

#define BITS_IN_BYTE 8
#define BITS_IN_UINT32_T (sizeof(uint32_t)*BITS_IN_BYTE)

typedef unsigned char uchar;

//globals
HeavyHittersParams_t default_hh_params {3000, 3000, 0.1, 8};

UrlCompressor::UrlCompressor():_huffman(),_is_loaded(false), _nextSymbol(1){
//	_symbol2pattern_db_size=0;
}

UrlCompressor::~UrlCompressor() {
//	if (_symbol2pattern_db!=NULL)	//TODO: remove
//		delete _symbol2pattern_db;
//	_symbol2pattern_db=NULL;

	while (!_symbol2pattern_db.empty()) {
		Pattern* p = _symbol2pattern_db.back();
		_symbol2pattern_db.pop_back();
		delete p;
	}

	// TODO Auto-generated destructor stub
}

void UrlCompressor::load_strings_and_freqs(Strings2FreqMap* strings_to_freq)
{
	DBG("Entered load_strings_and_freqs");
	uint32_t size = strings_to_freq->size();
	uint32_t* myfreq = new uint32_t[size+1];
	_strings_to_symbols.clear();

	uint32_t ith_symbol = 0;
	DBG("Print strings_to_freq:");
	for (std::map<std::string,int>::iterator it=strings_to_freq->begin(); it!=strings_to_freq->end(); ++it) {
		DBG( it->first << " freq => " << it->second );
		_strings_to_symbols.insert( std::pair<std::string,uint32_t>(it->first,ith_symbol) );
		myfreq[ith_symbol]=  it->second;
		ith_symbol++;
	}
	DBG(" ");

	DBG("Print _strings_to_symbols:");
	for (std::map<std::string,uint32_t>::iterator it=_strings_to_symbols.begin(); it!=_strings_to_symbols.end(); ++it) {
		DBG(it->first << " symbol => " << it->second );
		_strings_to_symbols.insert( std::pair<std::string,uint32_t>(it->first,ith_symbol) );
	}

	_huffman.load(myfreq,size);

}

bool UrlCompressor::LoadUrlsFromFile(const std::string& file_path,
									const HeavyHittersParams_t params,
									const  bool contains_basic_symbols)
{
	init();

	/*
	 * - create all 'single char' patterns in db
	 * - go over input file and count  frequencies
	 * - Load file_path into LDHH (Heavy Hitters) with 2 pass constructor
	 * - run LDHH
	 * - go over all signatures and add to module
	 * 4. go over file_path again and count literals
	 * ...
	 */
	{	//in separate block so 'lit' will close the file
		uint32_t frequencies[CHAR_MAX+1];
		for (uint32_t i=0 ; i <= CHAR_MAX ; i++ )
			frequencies[i]=1;

		LineIterator lit(file_path.c_str(),'\n');
		while (lit.has_next() ) {
			const raw_buffer_t &pckt = lit.next();
			uchar* p = pckt.ptr;
			for (uint32_t i = 0 ; i < pckt.size; i++) {
				uchar c = *p;
				p++;
				frequencies[c] += 1;
			}
		}

		char chars[2];
		chars[1] = '\0';
		for (unsigned char c=1; c <= CHAR_MAX ; c++) {
			chars[0] = (char) (c);
			std::string patStr(chars);
			addPattern(patStr,frequencies[c]);
		}
	}

	LDHH ldhh(file_path, params.n1, params.n2, params.r, params.kgrams_size);
	if (ldhh.run() != true)
		return unload_and_return_false();

	std::list<signature_t>& common_strings = ldhh.get_signatures();
	size_t                  urls_count     = ldhh.get_pckt_count();
	DBG("** scanned " << urls_count << " urls " << std::endl );

	int patterns_counter = 0;
	for (std::list<signature_t>::iterator it = common_strings.begin(); it != common_strings.end(); ++it) {
		signature_t& sig = *it;
		std::string patStr;
		const char* str =(const char *) &sig.data[0];
		patStr.assign(str,  sig.data.size());
		uint32_t frequency = sig.calcHitsInSource();
		addPattern(patStr,frequency);
		patterns_counter++;
	}
	DBG("total of "<< patterns_counter <<" patterns were found");
	DBG("total of "<< _nextSymbol <<" symbols were inserted");

	prepare_database();

	DBG( "load_dict_from_file: loaded "<<_nextSymbol<<" patterns");

	return true;
}

/**
 * This method will load and create encode/decode DB from a saved DB file
 * @param file_path - DB file path
 * @return true if loaded successfully
 */
bool UrlCompressor::LoadStoredDBFromFiled(std::string& file_path)
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
	init(atoi(chars)); //get number of lines to load

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
	uint32_t* freqArr = new uint32_t[symbol_counter];
	for (uint32_t i=0; i<symbol_counter;i++)  {  //skip symbol 0
			Pattern* pat =_symbol2pattern_db[i];
			assert(pat->_symbol == i);
			freqArr[i]=pat->_frequency;
	}
	_huffman.load(freqArr,symbol_counter);
	delete freqArr;

	calculate_symbols_score();	//evaluate each symbol encoded length
//	init_pattern_matching_algorithm();
	algo.load_patterns(&_symbol2pattern_db,getDBsize());

	DBG("load_dict_from_file: loaded "<<symbol_counter<<" patterns");
	return true;
}

//out_buf_size[0] is the length of coded bits (number of coded + 1)
UrlCompressorStatus UrlCompressor::encode(std::string url, uint32_t* out_encoded_buf, uint32_t& buf_size) {
	assert (buf_size > 2);

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
				if (i >= buf_size) {
					return STATUS_ERR_SMALL_BUF;
				}
				mask = reset_mask;
				out_encoded_buf[i] = 0;
			}
		}
		symbol++;
	}
	buf_size = i;
	return STATUS_OK;
}

UrlCompressorStatus UrlCompressor::decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size) {
	assert (in_buf_size > 2);
	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}

	DBG("decode:");
	UrlBuilder urlbuilder(_symbol2pattern_db);
	uint32_t num_of_left_bits_to_read = in_encoded_buf[0];

	uint32_t most_left_bit = 1 << (BITS_IN_UINT32_T -1 );
	HuffCode huff_code;
	uint32_t i;
	for (i=1 /*[0] was number of coded bits*/; i < in_buf_size; i++) {
		uint32_t buf = in_encoded_buf[i];
		std::bitset<32> x(buf);
		DBG(" buf="<<x);

		for (uint16_t j=0; j < BITS_IN_UINT32_T ; j++) {
			uint32_t bit = buf & most_left_bit;
			if (bit == 0) { 	// 0
				huff_code.push_back(false);
			} else {			// 1
				huff_code.push_back(true);
			}
			num_of_left_bits_to_read--;
			buf = buf << 1;	//shift left buf
//			DBG(s);
			symbolT symbol = _huffman.decode(huff_code);
			if (symbol != S_NULL) {
// these are only for debugging:
//				std::string code = _huffman.HuffCode_str(&huff_code);;
//				std::cout<<"code="<<code<<"; "<<_symbol2pattern_db[symbol]->_str<<STDENDL;
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
	DBG("finished decode");
//	urlbuilder.print();
	url = urlbuilder.get_url();

	return STATUS_OK;
}


void UrlCompressor::print_database(bool print_codes) {
	std::cout<<"UrlCompressor::print_database"<<std::endl;
	std::cout<<"UrlCompressor db contains "<< getDBsize() << " patterns:"<<std::endl;
	for (uint32_t i=0; i< getDBsize() ;i++) {
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
	for (symbolT i=0; i < getDBsize() ;i++) {
//	for (Symbol2PatternType::iterator iter=_symbol2pattern_db.begin(); iter!=_symbol2pattern_db.end();++iter) {
		HuffCode code=_huffman.encode( _symbol2pattern_db[i]->_symbol );
		_symbol2pattern_db[i]->_huffman_length=code.size();
	}
}

void UrlCompressor::init(uint32_t reserved_size) {
	if (isLoaded()) {
		unload_and_return_false();
	}
	_symbol2pattern_db.reserve(reserved_size);
	_symbol2pattern_db.push_back( new Pattern(0,NULL_DEFAULT_FREQ,"NULL") );
//	_symbol2pattern_db_size=1;
	_nextSymbol = 1;
	assert (_nextSymbol == getDBsize() );
	setLoaded();
}

symbolT UrlCompressor::addPattern(const std::string& str, const uint32_t& frequency) {
	Pattern* pat = new Pattern(_nextSymbol, frequency, str);
//	_symbol2pattern_db[_nextSymbol]=pat;
	_symbol2pattern_db.push_back( pat );
	_strings_to_symbols[str]=_nextSymbol;
	symbolT ret = _nextSymbol;
	_nextSymbol++;
	assert (_nextSymbol == _symbol2pattern_db.size());
	assert ((ret + 1) == _nextSymbol );
	return ret;
}

bool UrlCompressor::unload_and_return_false() {
	for (Symbol2pPatternVec::iterator it = _symbol2pattern_db.begin(); it!= _symbol2pattern_db.end(); ++it){
		delete *it;
	}
	_symbol2pattern_db.clear();
	setUnloaded();
	return false;
}

void UrlCompressor::prepare_database() {

	DBG("prepare_database:" << DVAL(_nextSymbol));
	//update number of symbols were loaded
//	_symbol2pattern_db_size = _nextSymbol;


	//prepare array to load huffman dictionary
	uint32_t* freqArr = new uint32_t[_nextSymbol];
	for (uint32_t i=0; i<_nextSymbol;i++)  {  //skip symbol 0
			Pattern* pat =_symbol2pattern_db[i];
			assert(pat->_symbol == i);
			freqArr[i]=pat->_frequency;
	}
	_huffman.load(freqArr,_nextSymbol);
	delete freqArr;

	calculate_symbols_score();	//evaluate each symbol encoded length
//	init_pattern_matching_algorithm();
	algo.load_patterns(&_symbol2pattern_db, getDBsize());

}


