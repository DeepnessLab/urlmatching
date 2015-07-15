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
#include "../PatternMatching/ACWrapperCompressed.h"
#include "UrlDictionaryTypes.h"


#define MAX_URL_LENGTH 1000
#define RESERVED_NUM_OF_PATTERNS 1000

#define URLC_STORED_DICT_VERSION 2

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
	//Importat:
	//Adding new members ? don't forget to update reset() & print()
	uint32_t number_of_symbols;
	uint32_t number_of_patterns;
	uint32_t number_of_urls;
	uint32_t max_huffman_length;
	uint32_t total_input_bytes;
	uint32_t total_patterns_length;
	uint32_t memory_allocated;	//how much memory the module allocated (except AC module)
	int 	 ac_memory_allocated;	//On linux only (otherwise 0)
	int 	 ac_statemachine_size;
	HeavyHittersParams_t params;
	bool params_set;

	uint32_t getACMachineEstSize() const { return (5* total_patterns_length); }

	void reset() ;

	void reset(const HeavyHittersParams_t& params_) {
		reset();
		params = params_;
		params_set = true;
	}

	void print(std::ostream& out) const ;

} UrlCompressorStats_t;

extern HeavyHittersParams_t default_hh_params;

class UrlCompressor {
public:
	UrlCompressor();
	virtual ~UrlCompressor();

	//Cache urls in input file into deque of std::string of urls
	static bool getUrlsListFromFile(const std::string& urls_file, std::deque<std::string>& url_list);
	static void SplitUrlsList(const std::deque<std::string>& input, std::deque<std::string>& output);

	//Load urlmatching dictionary from list of strings
	bool InitFromUrlsList(const std::deque<std::string> url_list,
			const HeavyHittersParams_t params,
			const  bool contains_basic_symbols);

	bool InitFromDictFile(std::string& file_path);
	bool InitFromDictFileStream(std::ifstream& file);
	bool StoreDictToFile(std::string& file_path);
	bool StoreDictToFileStream(std::ofstream& file );

	//This a much faster encoder
	//buf_size - input: out_encoded_buf max size, out - number of coded buffer
	UrlCompressorStatus encode_2(std::string url, uint32_t* out_encoded_buf, uint32_t& buf_size);
	//buf_size - input: out_encoded_buf max size, out - number of coded buffer
	UrlCompressorStatus encode(std::string url, uint32_t* out_encoded_buf, uint32_t& out_buf_size);

	//Decode in_encoded_buf
	//in_encoded_buf[0] contains the length of huffman coded bit
	//in_buf_size size of buf in bytes
	UrlCompressorStatus decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size);



	//Helpers
	inline bool isLoaded() const { return _is_loaded; }
	inline uint32_t SizeOfMemory() const {
		return (_statistics.memory_allocated + _statistics.getACMachineEstSize());
	}

	uint32_t getDictionarySize()  const;
	inline const UrlCompressorStats_t* get_stats() const {return  &_statistics; }
	bool sanity() ;

	//Debug API
	void print_database(std::ostream& ofs) const;
	void dump_ac_states(std::string filename) const;

	//load list of urls and build cached database
	//Deprecated!
	bool LoadUrlsFromFile(const std::string& file_path,
			const HeavyHittersParams_t params,
			const  bool contains_basic_symbols);

	Huffman _huffman;

private:
	void reset(uint32_t reserved_size = RESERVED_NUM_OF_PATTERNS);

	// calculates Huffman length and string lenght for every patterns
	 void calculate_symbols_huffman_score();
	/** once all patterns are loaded with their frequencies -
	  1. create huffman code
	  2. create pattern matching algorithm
	 */
	void prepare_database();
	void prepare_huffman_code(Pattern* pat, HuffCode& code);

	//rebuild _huffman in order to restore decoding capabilities (when loading from stored Dict file)
	void prepare_huffman();

	void setLoaded() { _is_loaded = true; }
	void setUnloaded() { _is_loaded = false; }
	bool unload_and_return_false();

	inline
	uint32_t getDBsize() const { return _symbol2pattern_db.size(); }

	/**
	 * Add pattern to dictionary
	 * @param str - pattern's string
	 * @param frequency - expected frequency
	 * @return the generated symbol of this pattern
	 */
	symbolT addPattern(const std::string& str, const uint32_t& frequency);

	inline
	void add_memory_counter(uint32_t bytes) { _statistics.memory_allocated+= bytes;}


	//Members:
	Symbol2pPatternVec _symbol2pattern_db;	//array of patterns, where symbol is the index

	ACWrapperCompressed algo;
	bool _is_loaded;
	symbolT _nextSymbol;
	UrlCompressorStats_t _statistics;

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
