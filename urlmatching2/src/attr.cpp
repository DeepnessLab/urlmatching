#include "UrlDictionay.h"
#include "logger.h"

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
