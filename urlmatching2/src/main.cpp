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


void my_print(char* str) {
	printf("%s\n", str);
}


#define RUN_TEST(name) std::cout<<"starting now test "<<#name<<std::endl; \
	name();

int main()
{
////	test_huffman();
//	std::cout<<"starting now"<<std::endl;
//	test_aho_compressed();

//	RUN_TEST(test_aho_compressed);
	RUN_TEST(test_url_matching);
	std::cout<<"test ended"<<std::endl;



	return 0;
}
