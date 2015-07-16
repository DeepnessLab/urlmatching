#include "UrlDictionay.h"
#include "../logger.h"
#include "../common.h"
#include <string.h>

Pattern::Pattern(uint32_t symbol, uint32_t frequency, std::string str) : _str(str) {
	_symbol=symbol;
	_frequency=frequency;
	_coded.buf[0] = 0;
	_coded.length = 0;
}

Pattern::~Pattern()  {
//	if (_coded.buf != NULL)
//		delete[] _coded.buf;
}

void UrlCompressorStats::reset() {
	params_set = false;
	number_of_patterns = 0;
	number_of_symbols = 0;
	number_of_urls = 0;
	max_huffman_length = 0;
	max_pattern_length = 0;
	total_input_bytes = 0;
	total_patterns_length = 0;
	memory_allocated = 0;
	ac_memory_allocated = 0;
	ac_statemachine_size = 0;
}


void UrlCompressorStats::print(std::ostream& out) const {
	out<< "Number of symbols  = " << (number_of_symbols)<<STDENDL;
	out<< "Number of patterns = " << (number_of_patterns)<<STDENDL;
	out<< "Number_of urls     = " << (number_of_urls)<<STDENDL;
	out<< "Max huffman length = " << (max_huffman_length)<< " bits"<<STDENDL;
	out<< "Max pattern length = " << (max_pattern_length)<< " chars"<<STDENDL;
	out<< "Total input bytes      = "<<(total_input_bytes)<< " Bytes = " << Byte2KB(total_input_bytes)<<"KB"<<STDENDL;
	out<< "Total patterns length  = "<<(total_patterns_length)<< " Bytes = "<< Byte2KB(total_patterns_length)<<"KB"<<STDENDL;
	out<< "Module memory allocated (without AC module)= " <<(memory_allocated)<< " Bytes = "<< Byte2KB(memory_allocated)<<"KB"<<STDENDL;
	out<< "AC statemachine mem size estimation ~ "<<getACMachineEstSize()<< " Bytes = "<< Byte2KB(getACMachineEstSize())<<"KB"<<STDENDL;
	out <<"AC module mem footprint (linux only)       ~ "<<ac_memory_allocated<< " Bytes = "<< Byte2KB(ac_memory_allocated) <<"KB"<< STDENDL;
	out <<"AC statemachine mem footprint (linux only) ~ "<<ac_statemachine_size<< " Bytes = "<< Byte2KB(ac_statemachine_size) <<"KB"<< STDENDL;
	if (params_set) {
		out<< "params: kgram size = "<< (params.kgrams_size)<< " r = " <<DVAL(params.r)<<STDENDL;
		out<< "params: n1 = "<< (params.n1)<< " n2 = " <<(params.n2)<<STDENDL;
	}
	out<<"Size of pointer = "<<SIZEOFPOINTER<<" Bytes"<<STDENDL;
	out<<"Size of Pattern = "<<sizeof(Pattern)<<" Bytes"<<STDENDL;
	out<<"Size of std::string = "<<sizeof(std::string)<<" Bytes"<<STDENDL;
	out<<"Size of CodedHuffman = "<<sizeof(CodedHuffman)<<" Bytes"<<STDENDL;
}


UrlBuilder::UrlBuilder(Symbol2pPatternVec symbol2pattern_db) :
		_symbol2pattern_db(symbol2pattern_db) ,
		buf_size(UrlBuilder_CHARBUFFSIZE) ,
		_url(_buf) ,
		is_url_dynamic(false)
{
	reset();
	_symbol_deque.clear();
}

void UrlBuilder::reset() {
	if (is_url_dynamic)
		delete[] _url;
	is_url_dynamic = false;
	_buf[0]='\0';
	_url = _buf;
	buf_size = UrlBuilder_CHARBUFFSIZE;
}

void UrlBuilder::append (symbolT symbol) {
	_symbol_deque.push_back(symbol);

	if ( ( strlen(_url) + _symbol2pattern_db[symbol]->_str.length()) > buf_size) {
		buf_size+=UrlBuilder_CHARBUFFSIZE;
		char* new_buf = new char[buf_size];
		std::cout<<"UrlBuilder::append: dynamic allocation"<<STDENDL;
		strcpy(new_buf,_url);
		if (is_url_dynamic) {
			delete[] _url;
		}
		is_url_dynamic = true;
		_url = new_buf;
	}
	strcat(_url,_symbol2pattern_db[symbol]->_str.c_str());
}

void UrlBuilder::debug_print() {
	DBG( "UrlBuilder::print -" << DVAL(_url) << ", " <<DVAL(_symbol_deque.size()));
	std::stringstream s0;
	s0 << "string: " ;
	for (SymbolDeque::iterator it = _symbol_deque.begin(); it != _symbol_deque.end(); ++it) {
		s0 << _symbol2pattern_db[*it]->_str << ";";
	}
	std::string out = s0.str();
	DBG(out);
	std::stringstream s1;
	s1 << "symbols: " ;
	for (SymbolDeque::iterator it = _symbol_deque.begin(); it != _symbol_deque.end(); ++it) {
		s1 << *it << ";";
	}
	out = s1.str();
	DBG(out);
}
