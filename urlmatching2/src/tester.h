/*
 * tester.h
 *
 *  Created on: 19 бреб 2014
 *      Author: Daniel
 */

#ifndef TESTER_H_
#define TESTER_H_

#define RUN_TEST(name) std::cout<<"TEST \""<<#name<<"\" STARTED now" << std::endl; \
	name(); \
	std::cout<<"TEST \""<<#name<<"\" ENDED" << std::endl;

class UrlCompressor;

void test_url_dictionary_load_from_stored_DB();
void test_url_matching() ;
void build_simple_dict(UrlCompressor* dict) ;
void test_aho_compressed();
void test_huffman();
void test_LLH();
void test_url_dictionary_load_from_url_txt_file();





#endif /* TESTER_H_ */
