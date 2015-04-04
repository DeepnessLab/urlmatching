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




class UrlCompressor {
public:
	UrlCompressor();
	virtual ~UrlCompressor();

	void load_strings_and_freqs(Strings2FreqType* strings_to_freq);

	//load list of urls and build cached database
	bool initFromUrlsListFile(std::string& file_path, bool contains_basic_symbols);

	/* load pre-stored dictionary from file and build cached database
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

	struct patternsIterator {
		Symbol2PatternType arr;
		symbolT index;
	};

	Huffman _huffman;
	Strings2SymbolsType _strings_to_symbols;

//temp private:
	void init_db(uint32_t size);
	void init_pattern_matching_algorithm();
	void calculate_symbols_score();

	Symbol2PatternType _symbol2pattern_db;
	uint32_t _symbol2pattern_db_size;

	ACWrapperCompressed algo;



};


#endif /* URLDICTIONAY_H_ */
