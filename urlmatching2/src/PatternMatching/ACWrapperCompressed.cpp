/*
 * ACWrapperCompressed.cpp
 *
 *  Created on: 7 December 2014
 *      Author: Daniel Krauhgamer
 *
 *  Please see header file for comments
 *
 */


#include "../PatternMatching/ACWrapperCompressed.h"
#include <string.h>
#include <assert.h>
#include <map>
#include <sstream>
#include "MatchingFunctions.h"
#include "../UrlToolkit/UrlMatching.h"
#include "../UrlToolkit/UrlMatchingTypes.h"
#include "../common.h"

extern "C" {
#include "../Common/Types.h"
#include "../StateMachine/StateMachineDumper.h"

}

int special_handle_pattern(char* str,uint32_t idx, void* data) {
	urlMatchingType* url_module = (urlMatchingType*) data;
	assert(url_module!=NULL);

	DBG("at idx="<<idx<<", pattern="<<str);
	//update module with the pattern
	return 1;
}


ACWrapperCompressed::ACWrapperCompressed() :
		_is_loaded(false),
		_has_patterns(false),
		_patternsList(NULL),
		_patternsMap(NULL),
		_machine(NULL),
		_size(0),
		_statemachine_size(0)
{
	_symbolsTable.table=NULL;
	_symbolsTable.size=0;
	_symbolsTableLevel1=0;
	_symbolsTableLevel2=0;
	for (int i=0; i < MAX_CHAR; i++) {
		_char_to_symbol[i]=S_NULL;
	}

}

ACWrapperCompressed::~ACWrapperCompressed() {
	if (_machine != NULL) {
		destroyStateMachine(_machine);
		_machine = NULL;
	}
	if (_patternsMap!= 0) {
		delete _patternsMap;
		_patternsMap = 0;
	}

	if (_symbolsTable.table != NULL )
		delete[] _symbolsTable.table;

	if (_symbolsTableLevel1 != 0)
		delete _symbolsTableLevel1;
	if (_symbolsTableLevel2 != 0)
		delete _symbolsTableLevel2;

}


bool ACWrapperCompressed::LoadPatterns(std::string filepath) {
	assert(false); // not supported
	const char* tmp = filepath.c_str();
	_machine = createStateMachine(tmp,100,100,0);
	make_pattern_to_symbol_list();
	_is_loaded = true;
	return true;

}

bool ACWrapperCompressed::LoadPatterns(Symbol2pPatternVec* patternsList, uint32_t patternsList_size, bool optimizeAnchors) {
	assert(!isLoaded());
	//make a copy for Symbol2PatternType list
	_patternsList = patternsList;
	//convert Symbol2PatternType to StringListType
	PatternsIterator list (patternsList->size(), optimizeAnchors);
	uint32_t idx=0;				//store how many string we entered the list
	_patternsMap = new patternsMapType();

	for (symbolT i=1; i< patternsList_size; i++) {	// 0 is reserved pattern as NULL
		//go over all patterns, if pattern is single char - set it into _char_to_symbol array
		// if pattern is longer than 1 char - add it to patterns list so AC will load them
		if (strlen ( (*patternsList)[i]->_str ) == 1) {
			const uint32_t c = (uint32_t) ((*patternsList)[i]->_str)[0];
			_char_to_symbol[c] = i;
		} else {
			_has_patterns = true;
			list.insert((*patternsList)[i]);
			const char* s = (*patternsList)[i]->_str;
			_patternsMap->insert(std::make_pair(s,i));
			idx++;
		}
	}

	if (_has_patterns) {
		int mem = get_curr_memsize();
		_machine = createStateMachineFunc(getStringFromList,&list,1000,1000,0);
		_statemachine_size = (uint32_t) get_curr_memsize() - (uint32_t) mem;
		DBG("AC state machine real size = "<< (get_curr_memsize() - mem)/1024<<"KB");
		_machine->handlePatternFunc = handle_pattern;
		// Build the complimantry table symbol --> pattern
		make_pattern_to_symbol_list();
	} else {
		_machine = NULL;
		_statemachine_size = 0;
		_symbolsTableLevel1 = new SerialAllocator<symbolT*>(0);
		_symbolsTableLevel2 = new SerialAllocator<symbolT >(0);
	}

	delete _patternsMap;
	_patternsMap=0;
	_is_loaded = true;

	//Use it for debug size
//	std::cout<< "AC module sizes allocated size: "<<STDENDL;
//	std::cout<< DVAL(MAX_CHAR)<<STDENDL;
//	std::cout<< DVAL(sizeof(_machine))<<STDENDL;
//	std::cout<< DVAL(_symbolsTable.size * sizeof(symbolT*))<<STDENDL;
//	std::cout<< DVAL(_symbolsTableLevel1->capacity())<<STDENDL;
//	std::cout<< DVAL(_symbolsTableLevel2->capacity())<<STDENDL;

	return true;
}

inline
symbolT* ACWrapperCompressed::create_symb_string (const char* c_string) {

	const char* c = c_string;
	uint32_t length = 2;	//symb_string = { symb, S_NULL } i.e length of 2
	while (*c != '\0') {
		if (*c == *_delimiter) {
			length++;
		}
		c++;
	}
//	symbolT* symb_string = new symbolT[length];
	symbolT* symb_string = _symbolsTableLevel2->alloc(length);
	assert(symb_string != NULL) ;
	symb_string[length-1] = S_NULL;


	char buffer[300];
	char* cpy =buffer;
	bool to_delete = false;
	if (strlen(c_string) >= 300) {
		cpy = new char[strlen(c_string)];
		to_delete = true;
	}
	strcpy(cpy,c_string);
	char* tk = strtok(cpy, _delimiter);
	uint32_t counter = 0;
	while (tk != NULL) {
		patternsMapType::iterator itr = _patternsMap->find(tk);
		if (itr == _patternsMap->end()) {
			ON_DEBUG_ONLY(std::cout<<"error not found \"" << DVAL(tk) << "\" from \"" << DVAL (c_string)<< "\" " << DVAL (counter)<<STDENDL);
			ASSERT(itr == _patternsMap->end());
		}
		symb_string[counter] = itr->second;

		ASSERT(strcmp ( (*_patternsList)[itr->second]->_str, tk) == 0 );

		counter++;
		tk = strtok(NULL, _delimiter);
	}
	if ((length-1) != counter) {	//lenght includes the S_NULL
		ON_DEBUG_ONLY(std::cout<<"error length "<<DVAL(length)<<" "<<DVAL(counter)<<STDENDL);
		ASSERT((length-1) != counter);
	}

	if (to_delete) {
		delete[] cpy;
	}
	return symb_string;
}

/* use this function to build a complimentary patterns table for symbols*/
void ACWrapperCompressed::make_pattern_to_symbol_list(bool verbose) {
	if(!_has_patterns) {
		return;
	}
	
	char ***patterns;
	patterns=_machine->patternTable->patterns;

	uint32_t size =_machine->patternTable->size;

	//patterns_as_symbols is an array of array of "symbolT string" i.e S_NULL terminated
	// patterns_as_symbols[i] is an array of dynamic size where the last pointer is NULL
	// 	i.e patterns_as_symbols[i] 		= {symbolT*,symbolT*,..,NULL}
	//		patterns_as_symbols [i][j] 	= {symbol1,symbol2,..,S_NULL}

	//since these allocations have poor memory alignment we will use SerialAllocator
	//which first allocates a big chunk and then we can get chunks from it
	//We need to go with two runs:
	// 1. to count allocations
	// 2. to actual allocated

	symbolT*** symbolsTable = new symbolT**[size];
	for (uint32_t i = 0; i < size; i++ ) {
		symbolsTable[i]=NULL;
	}

	uint32_t level1_size=0;	//for <sybmolT*> allocator
	uint32_t level2_size=0;	//for <sybmolT> allocator

	// count allocator sizes
	for (uint32_t i = 0; i < size; i++) {
		if (patterns[i] == NULL) {
			continue;
		}
		uint32_t num_of_j = 0;	//location of the terminating NULL
		//count patterns
		char* pp =patterns[i][num_of_j];
		while ( pp != NULL) {		// [0,1,2,3,NULL] j=4 is null, j returns with 5
			num_of_j++; //[11]
			pp= patterns[i][num_of_j];
		}
		level1_size+=num_of_j+1;
		if (num_of_j == 0) {
			continue;
		}

		//count symbols
		for (uint32_t j=0;j<num_of_j;j++) {
			const char* c = patterns[i][j];
			uint32_t length = 2;	//symb_string = { symb, S_NULL } i.e length of 2
			while (*c != '\0') {
				if (*c == *_delimiter) {
					length++;
				}
				c++;
			}
			level2_size+=length;
		}
	}

	_symbolsTableLevel1 = new SerialAllocator<symbolT*>(level1_size);
	_symbolsTableLevel2 = new SerialAllocator<symbolT >(level2_size);

	COND_DBG(verbose, "Print cached patterns table of size "<< size);
	for (uint32_t i = 0; i < size; i++) {
		if (patterns[i] == NULL) {
			symbolsTable[i]=NULL;
			COND_DBG(verbose, "patterns["<<i<<"]=NULL");
			continue;
		}
		uint32_t num_of_j = 0;	//location of the terminating NULL
		//count patterns
		char* pp =patterns[i][num_of_j];
		while ( pp != NULL) {		// [0,1,2,3,NULL] j=4 is null, j returns with 5
			num_of_j++; //[11]
			pp= patterns[i][num_of_j];
		}
//		symbolsTable[i] = new symbolT*[num_of_j+1];
		symbolsTable[i] = _symbolsTableLevel1->alloc(num_of_j+1);

		symbolsTable[i][num_of_j] = NULL;

		if (num_of_j == 0) {
			COND_DBG(verbose, "patterns["<<i<<"][0]=EMPTY"<<std::endl);
			continue;
		}

		//assign symbols
		for (uint32_t j=0;j<num_of_j;j++) {
			symbolT* symb_string = create_symb_string (patterns[i][j]);
			if (verbose) {
				std::stringstream out;
				out <<"patterns["<<i<<"]["<<j<<"  /"<<num_of_j<<"]=" << patterns[i][j] << "->";
				for(symbolT* sym = symb_string; *sym != S_NULL; sym++) {
					out << *sym<< ":"<<(*_patternsList)[*sym]->_str <<"; ";
				}
				COND_DBG(verbose, out);
			}
			symbolsTable[i][j] = symb_string;
		}
	}
	_symbolsTable.table = symbolsTable;
	_symbolsTable.size = size;
	COND_DBG(verbose, "FINISHED Print cached patterns table");
}

bool ACWrapperCompressed::MatchPatterns(std::string input_str, symbolT* result) {
	assert(isLoaded());

	if ( (input_str.length() == 0 )||
			(input_str == "") ){
		result[0]=S_NULL;
		return true;
	}

	if (! _has_patterns) {
		return MatchPatternsNoPatterns(input_str, result);
	}

	//prepare metadata
	urlMatchingType module;
	initModule(module);
	module.symbolsTable = &_symbolsTable;

	module.char_to_symbol = _char_to_symbol;
	module.list = _patternsList;

	const char* str = input_str.c_str();

	//TODO: insert the following into input string to init
	module.input_string = (char *) str;
	uint32_t first_char_of_string =  str[0];
	symbolT symbol_of_first_char = _char_to_symbol[first_char_of_string];
	module.matching_symbols_arr[module.matching_symbols_idx] = symbol_of_first_char;
	module.matching_symbols_idx++;

	MachineStats stats;
	_machine->handlePatternFunc = handle_pattern;
	_machine->handlePatternData = &module;
	//to support multithreaded - make a copy of _machine and set its handlePatternData member
	match(_machine,/*(char *)*/ str, strlen(str)
			, 1 /* verbose to make matching call handlePatternFunc when a pattern is found*/
			, &stats);
	finalize_result(module,result);

	return true;
}

bool ACWrapperCompressed::MatchPatternsNoPatterns(std::string input_str, symbolT* result) {
	const char* str = input_str.c_str();
	uint16_t i=0;
	for (i=0 ; i < input_str.length();i++) 	{
		uint32_t c = (uint32_t) str[i];
		result[i] = _char_to_symbol[c];
	}
	result[i]=S_NULL;
	return true;
}

void ACWrapperCompressed::printDB(std::ostream& os) {
	os<<"ACWrapperCompressed::printDB\n";
	return;
}

void ACWrapperCompressed::dump_states(std::string filename) const {
	if (!isLoaded()) {
		std::cout<< "Error: can't dump state, ACWapperCompressed is not loaded"<<std::endl;
		return;
	}
	dumpStateMachine(this->_machine, filename.c_str());
}

void ACWrapperCompressed::optimize_statemachine()
{
	if (!_has_patterns)
		return;

	std::string filename = "tmpACDUMPFILE.deleteme";
	int mem_before = get_curr_memsize();
	dumpStateMachine(this->_machine, filename.c_str());
	if (_machine != NULL) {
			destroyStateMachine(_machine);
			_machine = NULL;
	}
	DBG("Read dump..");
	this->_machine = createStateMachineFromDump(filename.c_str());
	patterntable_destroy(_machine->patternTable);
	_machine->patternTable = 0 ;
	int mem_after = get_curr_memsize();
	_statemachine_size = (uint32_t) mem_after - (uint32_t) mem_before;
	DBG("AC state machine Optimization reduced: "<< Byte2KB(mem_before - mem_after)<<"KB");
	DBG("AC state machine NEW real size = "<< Byte2KB(_statemachine_size)<<"KB");
}
