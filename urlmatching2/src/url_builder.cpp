/*
 * url_builder.cpp
 *
 *  Created on: 12 במאי 2015
 *      Author: Daniel
 */

#include "UrlDictionay.h"
#include "logger.h"

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

void UrlBuilder::print() {
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
