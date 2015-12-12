# urlmatching based compression algorithm
This project implements the URL compression algorithm descibed in ***"Scalable URL Matching with Small Memory Footprint"*** article. The project should be compiled under C++11
* The project contains a tester code to test the performance of module.
* The recommanded parameters are ```n=3000,r=0.5,k=8```
* To lean how to use the code explore the teseter's code in  ```tester.cpp```
* This tool uses the ```easylogging++```WIN/LINUX independent framework from https://github.com/easylogging/easyloggingpp

# API
The main module is located in `UrlMatchingModule.h` and contains api for:
* `InitFromUrlsList` 
* `StoreDictToFileStream` and `StoreDictToFile`
* `InitFromDictFileStream` and `InitFromDictFile`
* `encode`
* `decode`
* `OptimizedACMachineSize` - This is called from init methods when `optimize_size=true` It is needed since the original Compressed Aho-Corasick code has a non-efficient memory footprint only after it loads itself from a file

`FileCompressor.h` is module to compress and store URL text file to disk

# How to run 
```UrlMatchingTester -f \<file with urls from test_files.7z>```
- to explore all runtime options see urlmatching.exe -h

        Usage: UrlMatchingTester [CMD] [-f urls_path] <options> <compression params>
        
         1. testing CMD: test, ,testhash, article
            -f [String] urls filepath  - required
            -o [String] ouput filepath		, default: None 
            -a          add header to output_filepath	, default: None 
            -p [String] Print dictionary file path	, default: None
            -v          Verify by Decode - longer	, default: no
            -b [Int]    Take break time to measure program memory, Seconds, default: no
        
         2. Building dictionary and encoding input file using existing dictionary 
            CMD: build, encode
            All of the above flags and:
            -d [String] dicionary filepath	, default: "[urlsfile].dict" 
            -x 0 To skip Offline stage, this yelp wrong compression ratio, default: 1 
        
         3. compress file CMD: compress, extract -f [input file] -o [output file] <compression params> 
        
         Compression params: 
         ------------------ 
            -l [char]    Longest Prefix Match - split dictionary by /, default: false
            -k [Int]     k-gram size			, default: 8
            -r [Fload]   consecutive k-gram ratio	, default: 0.8
            -n [Int]     number of pattern anchors	, default: 1000
         Debugging flags:
         ---------------  
            -s          dump Aho Corasik state machine	, default: No
            -c [String] logger config file		, default: None 
        
            -h prints this message


# Repeating the tests in the article
* Test files with urls are provided in ```test_files/```
* Some of the files are to big for githug so they are in 7z container
* The URLs file that used in the article is ```test_files/full_domains ```
* The scripts used to produce the graphs and data in the article are in ```scripts/```, they might need to be updated with the executable name in order to use them.

# How to build

***Release***

    -std=c++0x
    
***Debug***

    -DBUILD_DEBUG
    -std=c++0x
    
  
## Loggin flags
```src/logger.h``` controls how debug prints are handled, simply configure the following code. Repository default is all set to *NONE*
```
#ifdef BUILD_DEBUG
#define DEBUG_OUTPUT DBG_TO_NONE
#else
#define DEBUG_OUTPUT DBG_TO_NONE
#endif
```
