/*
 * tester.h
 *
 *  Created on: 19 бреб 2014
 *      Author: Daniel
 */

#ifndef TESTER_H_
#define TESTER_H_

#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif


#define MAX_PATH 1000

//Helpers
inline
void getWorkingDirectory(std::string& out ) {
	char pBuf[1000];
	GetCurrentDir(pBuf, MAX_PATH);
	out.assign(pBuf);
}


#define GETTIMING double(end - begin) / (CLOCKS_PER_SEC)
#define START_TIMING 	do {begin = std::clock();} while(0)
#define STOP_TIMING 	do {end = std::clock();} while(0)
#define PREPARE_TIMING clock_t begin,end


#define RUN_TEST(name) PREPARE_TIMING; \
	std::cout<<"TEST \""<<#name<<"\" STARTED now" << std::endl; \
	START_TIMING ; \
	name(); \
	STOP_TIMING; \
	double test_time = GETTIMING; \
	std::cout<<"TEST \""<<#name<<"\" ENDED, took " << test_time / 60 << " min."<< std::endl;


class UrlCompressor;

void test_url_dictionary_load_from_stored_DB();
void test_url_matching() ;
void build_simple_dict(UrlCompressor* dict) ;
void test_aho_compressed();
void test_huffman();
void test_LLH();
void test_url_dictionary_load_from_url_txt_file();





#endif /* TESTER_H_ */
