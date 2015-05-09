/*
 * main.cpp
 *
 *  Created on: 1 2014
 *      Author: Daniel
 */

#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "Huffman.h"
#include "PatternMatching/ACWrapperClassic.h"
#include "tester.h"
#include "UrlDictionay.h"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

//void my_print(char* str) {
//	printf("%s\n", str);
//}



int main()
{
////	test_huffman();
//	std::cout<<"starting now"<<std::endl;
//	test_aho_compressed();

//	RUN_TEST(test_aho_compressed);
//	RUN_TEST(test_url_matching);
//	RUN_TEST(test_url_dictionary);
//	RUN_TEST(test_LLH);
	RUN_TEST(test_url_dictionary_load_from_url_txt_file);
	std::cout<<"test ended"<<std::endl;



	return 0;
}
