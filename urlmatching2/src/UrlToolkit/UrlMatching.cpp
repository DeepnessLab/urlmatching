/*
 * UrlMatching.cpp
 *
 *  Created on: 18 December 2014
 *	    Author: Daniel Krauthgamer
 *
 *  See header file for comments.
 *
 */

#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <map>
#include <bitset>
#include <assert.h>
#include "UrlMatching.h"
#include "PrintUrlMatching.h"
#include "../HeavyHitters/dhh_lines.h"
#include "../common.h"
#include "../logger.h"

#define MAX_DB_LINE_SIZE 100
#define SEPERATOR ","
#define NULL_DEFAULT_FREQ 1

#define BITS_IN_BYTE 8
#define BITS_IN_UINT32_T (sizeof(uint32_t)*BITS_IN_BYTE)

typedef unsigned char uchar;

//globals
DoubleHeavyHittersParams_t default_hh_params {1000, 1000, 0.9, 8};

UrlMatchingModule::UrlMatchingModule():_huffman(), _charsAllocator(0), _strAllocator(0), _is_loaded(false), _nextSymbol(1)
{

}

UrlMatchingModule::~UrlMatchingModule()
{

	while (!_symbol2pattern_db.empty()) {
		Pattern* p = _symbol2pattern_db.back();
		_symbol2pattern_db.pop_back();
		delete p;
	}

	if (_charsAllocator != 0 )
		delete _charsAllocator;

	if(_strAllocator != 0)
		delete _strAllocator;

}

bool UrlMatchingModule::getUrlsListFromFile(const std::string& urls_file, std::deque<std::string>& url_list) {
	LineIteratorFile lit(urls_file.c_str(),'\n');
	if (!lit.canRun())
		return false;
	while (lit.has_next() ) {
		const raw_buffer_t &pckt = lit.next();
		std::string str((const char *) pckt.ptr,pckt.size);
		bool whiteSpacesOnly = std::all_of(str.begin(),str.end(),isspace);
		//isprint(str[0]) is here since if the last line is empty then str contains EOF char
		if ( (!isprint(str[0])) || whiteSpacesOnly || (str.length() == 0)||(str == "") ) {
			std::cout<<"Skipped line " << url_list.size() +1 <<": Empty"<<STDENDL;
		} else{
			bool allPrintable = std::all_of(str.begin(),str.end(),isprint); //this has issues with cygwin
			if (!allPrintable) {
				std::cout<<"Skipped line " << url_list.size() +1 <<": Has non-printable chars, make sure file is not Windows EOL " <<STDENDL;
				continue;
			}
			url_list.push_back(str);
		}
	}
	return true;
}

void
UrlMatchingModule::SplitUrlsList(const std::deque<std::string>& input, std::deque<std::string>& output , std::string delimiter)
{
	for (std::deque<std::string>::const_iterator it=input.begin(); it != input.end(); ++it) {
		size_t start = 0;
		size_t end = 0;
		while (start != std::string::npos && end != std::string::npos) {
			start = it->find_first_not_of(delimiter, end);
			if (start != std::string::npos) {
				end = it->find_first_of(delimiter, start);
				if (end != std::string::npos) {
					output.push_back(it->substr(start, end - start));
				}
				else {
					output.push_back(it->substr(start));
				}
			}
		}
	}
}


bool UrlMatchingModule::InitFromUrlsList(const std::deque<std::string>& orig_url_list,
		const std::deque<std::string>& list_for_patterns,
		const DoubleHeavyHittersParams_t params,
		const  bool contains_basic_symbols,
		bool optimize_ac_size)
{
	reset();
	_statistics.reset(params);
	_statistics.memory_footprint = get_curr_memsize();

	//Small n protection - if n2 is too small we add pattern to pick top n2
	int small_n_protection = (params.n2 < 1000) ? 1000 : 0;

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
		freqT frequencies[CHAR_MAX+1];
		for (uint32_t i=0 ; i <= CHAR_MAX ; i++ )
			frequencies[i]=1;

		LineIteratorDeque lit(&orig_url_list);
		while (lit.has_next() ) {
			const raw_buffer_t &pckt = lit.next();
			uchar* p = pckt.ptr;
			for (uint32_t i = 0 ; i < pckt.size; i++) {
				uchar c = *p;
				p++;
				frequencies[c] += 1;
				if (frequencies[c] == UINT32_MAX)
					std::cout<<"Error: char "<<c<< " reached UINT32_MAX"<<STDENDL;
			}
			_statistics.total_input_bytes+=pckt.size;
		}

		_charsAllocator = new SerialAllocator<char>(2*CHAR_MAX + 2);

		for (unsigned char c=1; c <= CHAR_MAX ; c++) {
			char* chars = _charsAllocator->alloc(2);
			chars[0] = (char) (c);
			chars[1] = '\0';
			addPattern(chars,frequencies[c]);
		}
	}

	int dhh_n1 = params.n1 * (1.2) + small_n_protection;
	int dhh_n2 = params.n2 * (1.2) + small_n_protection;

	LineIteratorDeque line_itr(&list_for_patterns);
	LDHH ldhh(line_itr, dhh_n1, dhh_n2, params.r, params.kgrams_size);
	if (ldhh.run() != true)
		return unload_and_return_false();

	std::list<signature_t>& common_strings = ldhh.get_signatures();
	size_t                  urls_count     = ldhh.get_pckt_count();
	DBG("** scanned " << urls_count << " urls, found " <<common_strings.size()<< " patterns"<< std::endl );
	if (params.n2 == 0) {
		common_strings.clear();
		DBG("Special mode - huffman only was detected. i.e n2=0");
	}

	//Pick top n2 results from common_strings list
	common_strings.sort(signature_t::reversed_frequency_compare_signature_t);
	if (common_strings.size() > (uint32_t) dhh_n2) {
		common_strings.resize(dhh_n2);
	}

	//Evaluate total char size for serial allocator
	uint64_t total_char_size = 0;
	int i = 0;
	for (std::list<signature_t>::iterator it = common_strings.begin(); it != common_strings.end(); ++it) {
		signature_t& sig = *it;
		total_char_size+= sig.data.size() + 1 /*for '\0'*/;
		i++;
	}

	_strAllocator = new SerialAllocator<char>(total_char_size + 1);

	_symbol2pattern_db.reserve(common_strings.size());
	int patterns_counter = 0;
	for (std::list<signature_t>::iterator it = common_strings.begin(); it != common_strings.end(); ++it) {
		signature_t& sig = *it;
		const char* str =(const char *) &sig.data[0];
		char* chars = _strAllocator->alloc(sig.data.size() + 1);
		strncpy (chars, str, sig.data.size());
		chars[sig.data.size()] = '\0';
		freqT frequency = sig.real_count;
		if (frequency > 0) {
			addPattern(chars,frequency);
			patterns_counter++;
		}
	}
	add_memory_counter(_symbol2pattern_db.capacity() * SIZEOFPOINTER);
	_statistics.number_of_patterns = patterns_counter;
	_statistics.number_of_urls = urls_count;
	_statistics.number_of_symbols=_nextSymbol;
	DBG("total of "<< patterns_counter <<" patterns were found");
	DBG("total of "<< _nextSymbol <<" symbols were inserted");

	_symbol2pattern_db.shrink_to_fit();

	evaluate_precise_patterns_frequencies(orig_url_list);

	for (Pattern* p: _symbol2pattern_db) {
		if (p->_str_len == 1) {
			p->_coded.length = (uint16_t) p->get_h();
		}
	}

	{
		//Step 1: build AC patterns matching algorithm
		//	Load patterns to pattern matching algorithm();

		ACWrapperCompressed tmp_ac;
		//Step 1: build AC patterns matching algorithm
		//	Load patterns to pattern matching algorithm();
		HeapDiffMeasure mem_measure;
		tmp_ac.LoadPatterns(&_symbol2pattern_db, getDBsize(), params.n2 ,true /*optimize anchors*/);

		//reduce number of symbols and patterns by patterns that have been optimized out by LoadPatterns
		uint32_t number_of_unused_patterns = 0;
		for (symbolT i=1; i < _symbol2pattern_db.size(); i++ ) {
			if (getPattern(i)->_frequency == 0 )
				number_of_unused_patterns++;
		}
		_statistics.number_of_patterns -= number_of_unused_patterns;
		_statistics.number_of_symbols -= number_of_unused_patterns;

		//	evaluate_precise_frequencies_ac(orig_url_list);
		evaluate_precise_patterns_frequencies(orig_url_list,&tmp_ac);
	}

	{
		Symbol2pPatternVec tmp;
		int i=0,j=0;
		for (Pattern* p : _symbol2pattern_db) {
			if (p->_frequency == 0) {
				delete p;
				j++;
			} else {
//				assert(p->_symbol > MAX_CHAR);
				tmp.push_back(p);
				i++;
			}
		}
		_symbol2pattern_db.clear();
		symbolT s = 0;
		for (Pattern* p : tmp) {
			_symbol2pattern_db.push_back(p);
			p->_symbol = s;
			s++;
		}
		std::cout<<DVAL(i)<<" "<<DVAL(j)<<std::endl;
		_nextSymbol = s;
		assert(s == getDBsize());
		_symbol2pattern_db.shrink_to_fit();
	}
	std::cout<<"DB size = "<<getDBsize()<<std::endl;

	_statistics.ac_memory_footprint = get_curr_memsize();
	HeapDiffMeasure mem_measure;
	algo.LoadPatterns(&_symbol2pattern_db, getDBsize());
	_statistics.ac_statemachine_footprint = algo.getStateMachineSize();
	_statistics.ac_memory_footprint = mem_measure.get_diff();
	_statistics.ac_memory_footprint = algo.getStateMachineSize();
	_statistics.ac_memory_allocated = algo.size();

	//Step 2: build huffman dictionary and update all patterns
	//prepare array to load huffman dictionary
	prepare_huffman();

	calculate_symbols_huffman_score();	//evaluate each symbol encoded length

	// ----------------------------
//	_statistics.number_of_symbols = _symbol2pattern_db.size();
//	add_memory_counter(algo.size());

	_huffman.free_encoding_memory();

	if (optimize_ac_size) {
		OptimizedACMachineSize();
	}
	DBG( "load_dict_from_file: loaded "<<_nextSymbol<<" patterns");

	_statistics.memory_footprint = get_curr_memsize() - _statistics.memory_footprint;
	return true;
}

//recheck all frequencies
void UrlMatchingModule::evaluate_precise_frequencies_simple(const std::deque<std::string>& urls) {
	//reset all frequencies with length > 1
	symbolT first = MAX_CHAR;
	for (symbolT i=first ;  i < getDBsize(); i ++ ) {
		Pattern* p = _symbol2pattern_db[i];
		if (p->_str_len == 1) {
			continue;
		}
		p->_frequency = 0;
	}
	TimerUtil timer;
	//reevaluate frequencies with length > 1
	for (std::deque<std::string>::const_iterator it = urls.begin() ; it!=urls.end(); ++it) {
		for (symbolT i=first ;  i < getDBsize(); i ++ ) {
			Pattern* p = _symbol2pattern_db[i];
			if (p->_str_len == 1) {
				continue;
			}
			freqT f = 0;
			size_t pos=0;
			const char* pat = p->_str;
			pos = it->find(pat, pos);
			while (pos != std::string::npos) {
				f++;
				pos = it->find(pat, pos+1);
			}
			p->_frequency += f;
		}
	}
	std::cout<<"evaluate_precise_frequencies_simple, took " << timer.get_milseconds() << " ms" << std::endl;
}


//recheck all frequencies
void UrlMatchingModule::evaluate_precise_frequencies_ac(const std::deque<std::string>& urls) {
	//reset all frequencies with length > 1
	symbolT first = MAX_CHAR;
	for (symbolT i=first ;  i < getDBsize(); i ++ ) {
		Pattern* p = _symbol2pattern_db[i];
		if (p->_str_len == 1) {
			continue;
		}
		p->_frequency = 0;
	}
	TimerUtil timer;
	ACWrapperCompressed ac;
	ac.LoadPatterns(&_symbol2pattern_db, getDBsize(),false /*optimize anchors*/);
	//reevaluate frequencies with length > 1
	for (std::deque<std::string>::const_iterator it = urls.begin() ; it!=urls.end(); ++it) {
		ac.MatchPatterns_update_frequencies(*it);
	}
	std::cout<<"evaluate_precise_frequencies_ac, took " << timer.get_milseconds() << " ms" << std::endl;
}


//recheck all frequencies
void UrlMatchingModule::evaluate_precise_patterns_frequencies(const std::deque<std::string>& urls, ACWrapperCompressed* custom_ac) {
	TimerUtil timer;
	ACWrapperCompressed ac;

	std::deque<freqT> new_frequencies(getDBsize());
	for (uint32_t i = 0 ; i < getDBsize(); i++) {
		new_frequencies[i]=1;
	}

	if (custom_ac == 0) {
		ac.LoadPatterns(&_symbol2pattern_db, getDBsize(),false /*optimize anchors*/);
		custom_ac = &ac;
	}

	//reevaluate frequencies with length > 1
	for (std::deque<std::string>::const_iterator it = urls.begin() ; it!=urls.end(); ++it) {
		//find patterns cover over url
		symbolT result[MAX_URL_LENGTH];

		custom_ac->MatchPatterns(*it,result);
		symbolT* symbol = result;
		while (*symbol != S_NULL) {
			freqT f = new_frequencies[*symbol];
			new_frequencies[*symbol] = f+1;
			symbol++;
		}
	}

 	for (symbolT i=1 ;  i < getDBsize(); i ++ ) {
		Pattern* p = _symbol2pattern_db[i];

		if ((p->_frequency) == 0)
			continue;
		p->_frequency = new_frequencies[i];
//		std::cout<<"symbol="<<i<<" freq before="<<p->_frequency
//				<<", after="<<new_frequencies[i]
//											  << " \""<<p->_str
//											  << "\" huf len="<< p->getHuffmanLength()<<std::endl;
	}
//	std::cout<<"evaluate_precise_patterns_frequencies, took " << timer.get_milseconds() << " ms" << std::endl;
}


//DEPRECATED
bool UrlMatchingModule::LoadUrlsFromFile(const std::string& file_path,
									const DoubleHeavyHittersParams_t params,
									const  bool contains_basic_symbols)
{
	reset();
	_statistics.reset(params);

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

		LineIteratorFile lit(file_path.c_str(),'\n');
		while (lit.has_next() ) {
			const raw_buffer_t &pckt = lit.next();
			uchar* p = pckt.ptr;
			for (uint32_t i = 0 ; i < pckt.size; i++) {
				uchar c = *p;
				p++;
				frequencies[c] += 1;
			}
			_statistics.total_input_bytes+=pckt.size;
		}

		char chars[2];
		chars[1] = '\0';
		for (unsigned char c=1; c <= CHAR_MAX ; c++) {
			chars[0] = (char) (c);
			std::string patStr(chars);
//			addPattern(patStr,frequencies[c]);
		}
	}

	LineIteratorFile lit(file_path.c_str(),'\n');
	LDHH ldhh(lit, params.n1, params.n2, params.r, params.kgrams_size);
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
//		uint32_t frequency = sig.calcHitsInSource();
//		addPattern(patStr,frequency);
		patterns_counter++;
	}
	_statistics.number_of_patterns = patterns_counter;
	_statistics.number_of_urls = urls_count;
	DBG("total of "<< patterns_counter <<" patterns were found");
	DBG("total of "<< _nextSymbol <<" symbols were inserted");

	prepare_modules();

	_huffman.free_encoding_memory();
	DBG( "load_dict_from_file: loaded "<<_nextSymbol<<" patterns");

	return true;
}

bool UrlMatchingModule::StoreDictToFile(std::string& file_path)
{
	std::ofstream file (file_path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	bool ret = false;
	ret = StoreDictToFileStream(file);
	file.close();

	return ret;

}


bool UrlMatchingModule::StoreDictToFileStream(std::ofstream& file )
{
	/* File Format:
	 *	Header|_statistics|
	 *	FlatPattern|cstring|NULL|huffman code buff|..|..|
	 *	Header
	 */

	char* mem_block;
	FileHeader header;
	header.num_of_patterns = getDBsize();
	header.version = URLC_STORED_DICT_VERSION;
	header.chars_allocator_size = 0;
	header.chars_allocator_size = _symbol2pattern_db[0]->getStringLength();	//in case it was not allocated by an allocator
	header.chars_allocator_size += (_charsAllocator != 0) 	? _charsAllocator->size() : 0;
	header.chars_allocator_size += (_strAllocator != 0) 	? _strAllocator->size() : 0;
	mem_block = (char *) &header;
	file.write(mem_block,sizeof(header));

	mem_block = (char *) &_statistics;
	file.write(mem_block,sizeof(_statistics));

	for (uint32_t i=0; i< getDBsize() ;i++) {
		//	for (Symbol2PatternType::iterator it=_symbol2pattern_db.begin(); it!=_symbol2pattern_db.end(); ++it) {
		Pattern* ptrn = _symbol2pattern_db[i];
		//write FlatPattern
		FlatPattern fp;
		fp.symbol 		= ptrn->_symbol;
		fp.frequency 	= ptrn->_frequency;
		fp.str_length	= ptrn->getStringLength();
		fp.huffman_length= ptrn->getHuffmanLength(); //length in bits
		mem_block = (char *) &fp;
		file.write(mem_block,sizeof(FlatPattern));
		//write cstring+NULL
		file.write(ptrn->_str,ptrn->getStringLength()+1 /* for last NULL */);
		//write huffman code buffer

		uint16_t huff_buf_size = conv_bits_to_uin32_size(fp.huffman_length );
		huff_buf_size *= sizeof(uint32_t);
		mem_block = (char *) ptrn->_coded.buf;
		file.write(mem_block,huff_buf_size  );

	}
	mem_block = (char *) &header;
	file.write(mem_block,sizeof(header));

	std::cout<<"Info: Stored Dictionary file contains "<<sizeof(header)+sizeof(_statistics)+sizeof(header)
				<<" Bytes of header and statistics which are not needed for compression"<<std::endl;

	return true;
}

bool UrlMatchingModule::InitFromDictFile(std::string& file_path, bool optimize_ac_size)
{
	std::ifstream file (file_path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	bool ret = false;
	ret = InitFromDictFileStream(file,optimize_ac_size);
	file.close();

	return ret;
}

bool UrlMatchingModule::InitFromDictFileStream(std::ifstream& file, bool optimize_ac_size)
{
	/* File Format:
	 *	Header|_statistics|
	 *	FlatPattern|cstring|NULL|huffman code buff|..|..|
	 *	Header
	 */

	reset();

	char* mem_block;
	FileHeader header;
	mem_block = (char *) &header;
	file.read(mem_block,sizeof(header));
	if (header.version != URLC_STORED_DICT_VERSION) {
		file.close();
		std::cout<<"Wrong version of DB, expected "<<URLC_STORED_DICT_VERSION<<" found "<<header.version<<STDENDL;
		return false;
	}

	_charsAllocator = new SerialAllocator<char>(header.chars_allocator_size+10); //spare for Pattern 0

	mem_block = (char *) &_statistics;
	file.read(mem_block,sizeof(_statistics));

	//remove symbol 0 - NULL
	delete _symbol2pattern_db.back();
	_symbol2pattern_db.pop_back();
	ASSERT (_symbol2pattern_db.size() == 0);
	_nextSymbol = 0;

	_statistics.total_patterns_length=0;	//addPattern(..) will update that again
	_statistics.memory_footprint = get_curr_memsize();
	_statistics.memory_allocated = 0;

	_statistics.number_of_symbols = 0;
	_statistics.number_of_patterns = 0;

	char strbuf[1000];
	uint32_t patterns_counter = 0;
	for (uint32_t i=0; i< header.num_of_patterns ;i++) {
		//read FlatPattern
		FlatPattern fp;
		mem_block = (char *) &fp;
		file.read(mem_block,sizeof(FlatPattern));
		//create pattern with str and freq.
		ASSERT(patterns_counter == fp.symbol );
		file.read(strbuf,fp.str_length + 1);
		ASSERT(strlen(strbuf) == fp.str_length );
		char* str = _charsAllocator->alloc(fp.str_length + 1);
		strcpy(str , strbuf);
		symbolT s = addPattern(str,fp.frequency);
		ASSERT(s == patterns_counter);
		if (fp.str_length > 1)
			_statistics.number_of_patterns++;
		_statistics.number_of_symbols++;

		//update huffman code
		CodedHuffman& coded = _symbol2pattern_db[s]->_coded;
		coded.length = fp.huffman_length ;
		uint16_t huff_buf_size = conv_bits_to_uin32_size(coded.length );
		mem_block = (char *) coded.buf;
		huff_buf_size *= sizeof(uint32_t);
		file.read(mem_block,huff_buf_size );

		patterns_counter++;
	}

	//verify correct header
	FileHeader header_tmp;
	mem_block = (char *) &header_tmp;
	file.read(mem_block,sizeof(header_tmp));
	if (header_tmp.version != URLC_STORED_DICT_VERSION) {
		std::cout<<"Wrong version of DB, expected "<<URLC_STORED_DICT_VERSION<<" found "<<header_tmp.version<<STDENDL;
		return false;
	}


	//	Load patterns to pattern matching algorithm();
	_statistics.ac_memory_footprint = get_curr_memsize();
	algo.LoadPatterns(&_symbol2pattern_db, getDBsize(), false /*no anchor optimizing */);
	_statistics.ac_memory_footprint = get_curr_memsize() - _statistics.ac_memory_footprint -  algo.getStateMachineSize();
	_statistics.ac_statemachine_footprint = algo.getStateMachineSize();
	_statistics.ac_memory_allocated = algo.size();

	if (optimize_ac_size) {
		OptimizedACMachineSize();
		_symbol2pattern_db.shrink_to_fit();
	}
	add_memory_counter(_symbol2pattern_db.capacity() * SIZEOFPOINTER);
	// ----------------------------

	_statistics.memory_footprint = get_curr_memsize() - _statistics.memory_footprint;
	_is_loaded = true;
	return true;
}


/**
 * use the inner _symbol2pattern_db load _huffman
 * _huffman is needed when building the module to generate the huffman codes
 * Or when loading from a stored Dict file to enable decoding
 */
void UrlMatchingModule::prepare_huffman() {
	freqT* freqArr = new freqT[_nextSymbol];
	for (symbolT i=0; i<_nextSymbol;i++)  {  //skip symbol 0
			Pattern* pat =_symbol2pattern_db[i];
			ASSERT(pat->_symbol == i);
			freqArr[i]=pat->_frequency;
	}

	_huffman.load(freqArr,_nextSymbol);
	delete[] freqArr;
}

//out_buf_size[0] is the length of coded bits (number of coded + 1)
UrlCompressorStatus UrlMatchingModule::encode_slow(std::string url, uint32_t* out_encoded_buf, uint32_t& buf_size) {
	ASSERT (buf_size > 2);
	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}
	//find patterns cover over url
	symbolT result[MAX_URL_LENGTH];
	algo.MatchPatterns(url,result);

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
	buf_size = i + 1; //convert from last used space to size
	return STATUS_OK;
}

void print(std::ostream& o, uint32_t buf) {
	std::bitset<32> x(buf);
	o <<x<<STDENDL;
}

//out_buf_size[0] is the length of coded bits (number of coded + 1)
UrlCompressorStatus UrlMatchingModule::encode(std::string url, uint32_t* out_encoded_buf, uint32_t& buf_size) {
	ASSERT (buf_size > 2);
	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}
	//find patterns cover over url
	symbolT result[MAX_URL_LENGTH];
	algo.MatchPatterns(url,result);


	//Huffman encode the symbols
	uint32_t& bit_counter = out_encoded_buf[0];
	bit_counter=0;
	uint32_t i_out = 1;	//idx in out_encoded_buf
	out_encoded_buf[i_out] = 0;

	uint32_t lsb = 0;	//next bit code {0..31}
	const uint32_t bitsinbuf = 8 * sizeof(uint32_t);
	symbolT* symbol = result;
	while (*symbol != S_NULL) {
		Pattern* pat = _symbol2pattern_db[*symbol];
//		std::cout<<"pat= "<<pat->_str<<" " << DVAL(lsb) <<STDENDL;
		uint32_t i = 0;
		uint16_t length = pat->_coded.length;
		bit_counter+=length;
		uint32_t next_lsb = (length + lsb) % bitsinbuf;
		//each iteration we code one buf - i.e buf[i]
		while (length > 0 ) {
			uint32_t buf = pat->_coded.buf[i];
//			std::cout << "-- " << DVAL(length) << " "<< DVAL(i)<<STDENDL;
//			std::cout << "buf "<<i<<" l-"<<length<<" ="; print(buf);
			//lsb = 2
			buf >>= lsb;		//code the left most bit  01234567 >> lsb 2
//			std::cout << "buf  >>=   "; print(buf);
//			std::cout << "buf out["<<i_out<<"]=";print(out_encoded_buf[i_out]);
			out_encoded_buf[i_out] |=  buf;					//xx012345
//			std::cout << "buf out["<<i_out<<"]=";print(out_encoded_buf[i_out]);
			uint32_t lsb_tag = bitsinbuf - lsb;			// 6
			length = (length > lsb_tag) ? (length - lsb_tag) : 0; // 8 - 2 = 6

			// how many bits were code ?
			if (length > 0 ) {	//we need to code another byte
				buf = pat->_coded.buf[i];
//				std::cout << "- in   =   "; print(buf);
				buf <<= lsb_tag;							// 6  --> 67000000
//				std::cout << "buf  <<=   "; print(buf);
				i_out++;
				out_encoded_buf[i_out] =  0;						//67xxxxxx
				out_encoded_buf[i_out] |=  buf;						//67xxxxxx
//				std::cout << "buf out["<<i_out<<"]=";print(out_encoded_buf[i_out]);
				length = (length > lsb) ? (length - lsb) : 0; // length - 2
			}
			i++;
		}
		lsb = next_lsb;
		if (lsb==0) {
			i_out++;
			out_encoded_buf[i_out] = 0;
		}
		symbol++;
	}

	buf_size = i_out + 1; //convert from last used space to size
	return STATUS_OK;
}

UrlCompressorStatus UrlMatchingModule::decode(std::string& url, uint32_t* in_encoded_buf, uint32_t in_buf_size) {
	ASSERT (in_buf_size > 2);
	if (isLoaded() == false) {
		return STATUS_ERR_NOT_LOADED;
	}

	if (!_huffman.isLoaded()) {
		prepare_huffman();
	}

	DBG("decode:");
	UrlBuilder urlbuilder(_symbol2pattern_db);
	uint32_t num_of_left_bits_to_read = in_encoded_buf[0];

	uint32_t most_left_bit = 1 << (BITS_IN_UINT32_T -1 );
	HuffCode huff_code;
	uint32_t i;
	for (i=1 /*[0] was number of coded bits*/; i < in_buf_size; i++) {
		uint32_t buf = in_encoded_buf[i];
#ifdef BUILD_DEBUG
		std::bitset<32> x(buf);
		DBG(" buf="<<x);
#endif
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
			symbolT symbol ;
			bool found = _huffman.decode(huff_code,symbol);
			if (found) {
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
	ON_DEBUG_ONLY( urlbuilder.debug_print() );
	url = urlbuilder.get_url();

	return STATUS_OK;
}

uint32_t UrlMatchingModule::getDictionarySize()  const{
	uint32_t size = 0;
	for (uint32_t i=0; i< getDBsize() ;i++) {
		Pattern* ptrn = _symbol2pattern_db[i];
		size += sizeof(symbolT) ; //ptrn->_symbol
		size += sizeof(freqT) ; //ptrn->_frequency
		size += conv_bits_to_uin32_size(ptrn->getHuffmanLength()) ;
		size += sizeof(uint16_t) ;// to store huffmanLength
		size += ptrn->getStringLength();
		size += sizeof(uint16_t) ; // to store stringLength
	}
	return size;
}

void UrlMatchingModule::print_database(std::ostream& ofs) const
{
	ofs <<"UrlCompressor db contains "<< getDBsize() << " patterns:"<<std::endl;
	ofs << "Symbol,Frequency, Code length (bits), Code, strlen (bytes), string, Huffman length est. (bits)"<<std::endl;
	for (uint32_t i=0; i< getDBsize() ;i++) {
	//	for (Symbol2PatternType::iterator it=_symbol2pattern_db.begin(); it!=_symbol2pattern_db.end(); ++it) {
		Pattern* ptrn = _symbol2pattern_db[i];
		ofs 	<< ptrn->_symbol
				<<"," << ptrn->_frequency
				<<","<<ptrn->getHuffmanLength()
				<<",0x";
		uint16_t size_ = conv_bits_to_uin32_size(ptrn->getHuffmanLength());
		for (uint16_t t=0 ; t < size_;  t++) {
			ofs << std::hex <<std::setfill('0') << std::setw(sizeof(uint32_t)) << ptrn->_coded.buf[t] << std::dec ;
		}
		ofs <<","<<ptrn->getStringLength();
		ofs <<","<<ptrn->_str;
		ofs <<","<<ptrn->get_h();
		ofs << std::endl;
	}
}


void UrlMatchingModule::calculate_symbols_huffman_score() {
	for (symbolT i=0; i < getDBsize() ;i++) {
//	for (Symbol2PatternType::iterator iter=_symbol2pattern_db.begin(); iter!=_symbol2pattern_db.end();++iter) {
		HuffCode code=_huffman.encode( _symbol2pattern_db[i]->_symbol );
		prepare_huffman_code(_symbol2pattern_db[i],code);
		_statistics.max_huffman_length =
				(_symbol2pattern_db[i]->getHuffmanLength() > _statistics.max_huffman_length)?
				_symbol2pattern_db[i]->getHuffmanLength() : _statistics.max_huffman_length;
	}
}

void UrlMatchingModule::prepare_huffman_code(Pattern* pat, HuffCode& code) {
	pat->_coded.length = code.size();
	uint32_t buf_size = conv_bits_to_bytes(pat->_coded.length) ;
	if (buf_size > MAX_CODED_HUFFMAN_SIZE*sizeof(uint32_t)) {
		std::cout<<"Critical error: Found huffman to long="<<pat->_coded.length<<"bit or "<<buf_size<<" Bytes, can't continue since max buffer size is "<<sizeof(uint32_t)*MAX_CODED_HUFFMAN_SIZE<<STDENDL;
		exit(1);
	}
//	pat->_coded.buf = new uint32_t[ buf_size ];	//this line is obsolete
	uint32_t* buf = pat->_coded.buf;

	uint32_t mask_reset = 1 << (BITS_IN_UINT32_T -1 );	//MSb - 1 i.e 100000000..
	uint32_t mask = mask_reset;
	uint32_t i = 0;		//idx in buf
	buf[i] = 0;			//buf[0] = 0;
	for (HuffCode::const_iterator it = code.begin(); it != code.end(); ++it) {
		if (*it == true) {
			buf[i] = buf[i] | mask;
		}
		mask = mask / 2;	//move right
		if (mask == 0) {
			i++;
			ASSERT( i < buf_size );
			mask = mask_reset;
			buf[i] = 0;
		}
	}
}

extern uint32_t Pattern::total_frequency;
extern char 	Pattern::C_state;
void UrlMatchingModule::reset(uint32_t reserved_size) {
	if (isLoaded()) {
		unload_and_return_false();
	}
	Pattern::total_frequency = 0;
	Pattern::C_state = 4;
	_symbol2pattern_db.reserve(reserved_size);
	_symbol2pattern_db.push_back( new Pattern(0,NULL_DEFAULT_FREQ,"NULL") );
	_nextSymbol = 1;
	_statistics.reset();
	ASSERT (_nextSymbol == getDBsize() );
	setLoaded();
}

symbolT UrlMatchingModule::addPattern(const char* str, const freqT& frequency) {
	Pattern* pat = new Pattern(_nextSymbol, frequency, str);
//	_symbol2pattern_db[_nextSymbol]=pat;
	_symbol2pattern_db.push_back( pat );
//	_strings_to_symbols[str]=_nextSymbol;
	symbolT ret = _nextSymbol;
	_nextSymbol++;
	uint16_t len = pat->getStringLength();
	if (frequency > 0 ) {
		_statistics.total_patterns_length+= len*sizeof(char);
		_statistics.max_pattern_length = (_statistics.max_pattern_length < len ) ? len : _statistics.max_pattern_length;
	}
	ASSERT (_nextSymbol == _symbol2pattern_db.size());
	ASSERT ((ret + 1) == _nextSymbol );
	add_memory_counter(pat->size());
	return ret;
}

bool UrlMatchingModule::unload_and_return_false() {
	for (Symbol2pPatternVec::iterator it = _symbol2pattern_db.begin(); it!= _symbol2pattern_db.end(); ++it){
		delete *it;
	}
	_symbol2pattern_db.clear();
	setUnloaded();
	return false;
}

void UrlMatchingModule::prepare_modules() {

	DBG("prepare_database:" << DVAL(_nextSymbol));


}

bool UrlMatchingModule::sanity()
{
	if (!isLoaded()) {
		return false;
	}
	//Sanity testing - encode/decode a single string
	std::string my_string = "http://www.besound.com/pushead/home.html";
	uint32_t buff_size = 100;
	uint32_t codedbuff[100] ;
	encode(my_string,codedbuff,buff_size);

	std::string decoded_str;
	int ret = decode(decoded_str,codedbuff,buff_size);
	if (ret != STATUS_OK)
		return false;
	if (my_string != decoded_str) {
		std::cout<<"ERROR DECODING: STRINGS NOT MATCH"<<STDENDL;
		std::cout<<DVAL(my_string)<<" != "<<DVAL(decoded_str)<<STDENDL;
		std::cout<<" had length "<<DVAL(codedbuff[0])<<STDENDL;
		return false;
	}
	return true;
}

void UrlMatchingModule::count_pattern_matching_hits(std::string url, uint64_t& hits_counter) {
	algo.MatchPatterns_for_Throughput(url, hits_counter);
}

void UrlMatchingModule::dump_ac_states(std::string filename) const
{
	algo.dump_states(filename);
}
