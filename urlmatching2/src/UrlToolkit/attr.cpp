#include "UrlDictionay.h"
#include "../logger.h"

Pattern::Pattern(uint32_t symbol, uint32_t frequency, std::string str) : _str(str) {
	_symbol=symbol;
	_frequency=frequency;
	_coded.buf = NULL;
	_coded.length = 0;
}

Pattern::~Pattern()  {
	if (_coded.buf != NULL)
		delete[] _coded.buf;
}

void UrlCompressorStats::reset() {
	params_set = false;
	number_of_patterns = 0;
	number_of_symbols = 0;
	number_of_urls = 0;
	max_huffman_length = 0;
	total_input_bytes = 0;
	memory_allocated = 0;
}


void UrlCompressorStats::print(std::ostream& out) const {
	out<<  DVAL(number_of_symbols)<<STDENDL;
	out<<  DVAL(number_of_patterns)<<STDENDL;
	out<<  DVAL(number_of_urls)<<STDENDL;
	out<<	 DVAL(max_huffman_length)<< " bits"<<STDENDL;
	out<<	 DVAL(total_input_bytes)<< "B"<<STDENDL;
	out<<	 "estimated " << DVAL(memory_allocated) << "B"<<STDENDL;
	if (params_set) {
		out<< "params: "<< DVAL(params.kgrams_size)<< " " <<DVAL(params.r)<<STDENDL;
		out<< "params: "<< DVAL(params.n1)<< " " <<DVAL(params.n2)<<STDENDL;
	}
}


UrlBuilder::UrlBuilder(Symbol2pPatternVec symbol2pattern_db) :
		_symbol2pattern_db(symbol2pattern_db),
		_url("")
{
	_symbol_deque.empty();
}


void UrlBuilder::append (symbolT symbol) {
	_symbol_deque.push_back(symbol);
	_url.append(_symbol2pattern_db[symbol]->_str);
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
