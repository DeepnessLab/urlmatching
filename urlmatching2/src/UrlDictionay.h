/*
 * UrlDictionay.h
 *
 *  Created on: 1 ���� 2014
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
#define RESERVED_NUM_OF_PATTERNS 1000

enum UrlCompressorStatus {
	STATUS_OK = 1,
	STATUS_FAIL = -1,
	STATUS_ERR_NOT_LOADED = -2,
	STATUS_ERR_SMALL_BUF = -3,
	STATUS_ERR_LOST_DECODED_DATA = -4
};

typedef struct HeavyHittersParams {
	int n1;
	int n2;
	float r;
	size_t kgrams_size;
} HeavyHittersParams_t;

typedef struct UrlCompressorStats {
	uint32_t number_of_symbols;
	uint32_t number_of_patterns;
	uint32_t number_of_urls;
	uint32_t max_huffman_length;
	uint32_t total_input_bytes;
	uint32_t memory_allocated;
	HeavyHittersParams_t params;
	bool params_set;

	void reset() {
		params_set = false;
		number_of_patterns = 0;
		number_of_symbols = 0;
		number_of_urls = 0;
		max_huffman_length = 0;
		total_input_bytes = 0;
		memory_allocated = 0;
	}

	void reset(const HeavyHittersParams_t& params_) {
		reset();
		params = params_;
		params_set = true;
	}

	void print() const ;

} HeavyHittersStats_t;

extern HeavyHittersParams_t default_hh_params;

class UrlCompressor {
public:
	UrlCompressor();
	virtual ~UrlCompressor();

	bool isLoaded() { return _is_loaded; }

	//buf_size - input: out_encoded_buf max size, out - number of coded buffer
	UrlCompressorStatus encode(std::string url, uint32_t* out_encoded_buf, uint32_t& out_buf_size);
	//This a much faster encoder
	//buf_size - input: out_encoded_buf max size, out - number of coded buffer
	UrlCompressorStatus encode_2(std::string url, uint32_t* out_encoded_buf, uint32_t& buf_size);

	//Decode in_encoded_buf
	//in_encoded_buf[0] contains the length of huffman coded bit
	//in_buf_size size of buf in bytes
	UrlCompressorStatus decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size);

	//load list of urls and build cached database
	bool LoadUrlsFromFile(const std::string& file_path,
			const HeavyHittersParams_t params,
			const  bool contains_basic_symbols);

	/** load pre-stored dictionary from file and build cached database
	 * DB file format:
	 * <number of db entries>
	 * <symbol#>,<frequency>,<pattern_string>\n
	 * ...
	 * Returns true if successfully loaded
	 * will assert if symbols are not ordered and continues, i.e 0..n
	 */
	bool LoadStoredDBFromFile(std::string& file_path);

	uint32_t SizeOfMemory() { return _statistics.memory_allocated; }

	//Debug API
	void print_database(bool print_codes=false);
	void print_strings_and_codes();

	//---- Deprecated API -----
	void load_strings_and_freqs(Strings2FreqMap* strings_to_freq);
	//---- Deprecated API -----

	const HeavyHittersStats_t* 	get_stats() {return  &_statistics; }


	struct patternsIterator {
		Symbol2pPatternVec arr;
		symbolT index;
	};

	Huffman _huffman;
	Strings2SymbolsMap _strings_to_symbols;	//maps std::strings to symbols

private:
	void init(uint32_t reserved_size = RESERVED_NUM_OF_PATTERNS);

	// calculates Huffman length and string lenght for every patterns
	 void calculate_symbols_huffman_score();
	/** once all patterns are loaded with their frequencies -
	  1. create huffman code
	  2. create pattern matching algorithm
	 */
	void prepare_database();
	void prepare_huffman_code(Pattern* pat, HuffCode& code);

	void setLoaded() { _is_loaded = true; }
	void setUnloaded() { _is_loaded = false; }
	bool unload_and_return_false();

	inline
	uint32_t getDBsize() { return _symbol2pattern_db.size(); }

	/**
	 * Add pattern to dictionary
	 * @param str - pattern's string
	 * @param frequency - expected frequency
	 * @return the generated symbol of this pattern
	 */
	symbolT addPattern(const std::string& str, const uint32_t& frequency);

	inline
	void add_memory_counter(uint32_t bytes) { _statistics.memory_allocated+= bytes;}

	//TODO: get this into a struct
	Symbol2pPatternVec _symbol2pattern_db;	//array of patterns, where symbol is the index
//	uint32_t _symbol2pattern_db_size;	//length of this array

	ACWrapperCompressed algo;
	bool _is_loaded;
	symbolT _nextSymbol;
	HeavyHittersStats_t _statistics;

};

class UrlBuilder {
public:
	UrlBuilder(Symbol2pPatternVec symbol2pattern_db);
	virtual ~UrlBuilder() {}

	virtual void reset() {_url.empty(); }
	virtual void append (symbolT symbol);
	virtual std::string get_url() {return _url; }
	virtual void debug_print();

private:
	typedef std::deque<symbolT> SymbolDeque ;

	Symbol2pPatternVec _symbol2pattern_db;
	std::string _url;
	SymbolDeque _symbol_deque;
};


#endif /* URLDICTIONAY_H_ */
