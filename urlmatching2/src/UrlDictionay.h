/*
 * UrlDictionay.h
 *
 *  Created on: 1 бреб 2014
 *      Author: Daniel
 */

#ifndef URLDICTIONAY_H_
#define URLDICTIONAY_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <map>
#include "Huffman.h"
//#include "PatternMatching/ACWrapperClassic.h"
#include "PatternMatching/ACWrapperCompressed.h"
#include "UrlDictionaryTypes.h"


#define MAX_URL_LENGTH 1000


enum UrlCompressorStatus {
	STATUS_OK = 1,
	STATUS_FAIL = -1,
	STATUS_ERR_NOT_LOADED = -2,
	STATUS_ERR_SMALL_BUF = -3,
	STATUS_ERR_LOST_DECODED_DATA = -4
};

class UrlCompressor {
public:
	UrlCompressor();
	virtual ~UrlCompressor();

	bool isLoaded() { return _is_loaded; }

	void load_strings_and_freqs(Strings2FreqMap* strings_to_freq);

	//load list of urls and build cached database
	bool initFromUrlsListFile(std::string& file_path, bool contains_basic_symbols);

	/** load pre-stored dictionary from file and build cached database
	 * DB file format:
	 * <number of db entries>
	 * <symbol#>,<frequency>,<pattern_string>\n
	 * ...
	 * Returns true if successfully loaded
	 * will assert if symbols are not ordered and continues, i.e 0..n
	 */
	bool initFromStoredDBFile(std::string& file_path);

	//API
//	void compressUrl(char* url);
//	void decompressUrl(char* code);

	void print_database(bool print_codes=false);
	void print_strings_and_codes();

	UrlCompressorStatus encode(std::string url, uint32_t* out_encoded_buf, uint32_t out_buf_size);
	UrlCompressorStatus decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size);


	struct patternsIterator {
		Symbol2pPatternArr arr;
		symbolT index;
	};

	Huffman _huffman;
	Strings2SymbolsMap _strings_to_symbols;	//maps std::strings to symbols

//temp private:
	void init_db(uint32_t size);
	void init_pattern_matching_algorithm();
	void calculate_symbols_score();

	void setLoaded() { _is_loaded = true; }
	void setUnloaded() { _is_loaded = false; }


	//TODO: get this into a struct
	Symbol2pPatternArr _symbol2pattern_db;	//array of patterns, where symbol is the index
	uint32_t _symbol2pattern_db_size;	//length of this array

	ACWrapperCompressed algo;
	bool _is_loaded;



};

class UrlBuilder {
public:
	UrlBuilder(Symbol2pPatternArr symbol2pattern_db);
	virtual ~UrlBuilder() {}

	virtual void reset() {_url.empty(); }
	virtual void append (symbolT symbol);
	virtual std::string get_url() {return _url; }
	virtual void print();

private:
	typedef std::deque<symbolT> SymbolDeque ;

	Symbol2pPatternArr _symbol2pattern_db;
	std::string _url;
	SymbolDeque _symbol_deque;
};


#endif /* URLDICTIONAY_H_ */
