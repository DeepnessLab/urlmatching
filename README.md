# ***urlmatching*** based compression algorithm
This project implements the URL compression algorithm descibed in the article ***"Scalable URL Matching with Small Memory Footprint" by Anat Bremler-Barr, David Hay, Daniel Krauthgamer and Shimrit Tzur David***. The algorithm was implemented by Daniel Krauthgamer.
* The project should be compiled under C++11 or later
* The project contains a tester code to test the performance of module.
* The recommended parameters are ```n=1000,r=0.5,k=3``` and ```n=1000,r=0.5,k=2``` when running in LPM using ```.``` delimiter
* To lean how to use the code explore the teseter's code in  ```tester.cpp```

#### Abstract:
URL matching lies at the core of many networking applications and Information Centric Networking architectures. For example, URL matching is extensively used by Layer 7 switches, ICN/NDN routers, load balancers, and security devices. Modern URL matching is done by maintaining a rich database that consists of tens of millions of URL which are classified to dozens of categories (or egress ports). In real-time, any input URL has to be searched in this database to find the corresponding category. In this paper, we introduce a generic framework for accurate URL matching (namely, no false positives or miscategorization) that aims to reduce the overall memory footprint, while still having low matching latency. We introduce a dictionary-based compression method that compresses the database by 60%, while having only a slight overhead in time. Our framework is very flexible and it allows hot-updates, cloud-based deployments, and can deal with strings that are not URLs. 

#### Acknowledgments:
* This framework uses the ```easylogging++```WIN/LINUX independent framework from https://github.com/easylogging/easyloggingpp
* This framework uses the compressed Aho-Corasik pattern matching algorithm in https://github.com/DeepnessLab/mca2. This code was generalized using a C-like visitor design pattern (the AH-Compressed executes a function on any matched pattern).
* This framework uses the ***Double Heavy Hitters algorithm*** from www.deepness-lab.org/pubs/ancs13_autosig.pdf

# API
The main module is located in `UrlMatchingModule.h` and contains api for:
* Initialize / Store / Load the compression dicionary:
  * `InitFromUrlsList` 
  * `StoreDictToFileStream` and `StoreDictToFile`
  * `InitFromDictFileStream` and `InitFromDictFile`
  * `OptimizedACMachineSize` - This is called from init methods when `optimize_size=true` It is needed since the original
Compressed Aho-Corasick code has a non-efficient memory footprint only after it loads itself from a file
* Encode / Decode
  * `encode`
  * `decode`
 
`FileCompressor.h` is module for archiving. Compress and store URL text file to disk, similar to ```zip```

# How to run the tester
```UrlMatchingTester -f <file with urls from>```
- to explore all runtime options see urlmatching -h

        Usage: UrlMatchingTester [CMD] [-f urls_path] <options> <compression params>
        
         1. testing CMD: test, ,testhash, article
            -f [String] urls filepath  - required
            -o [String] ouput filepath		, default: None 
            -a          add header to output_filepath	, default: None 
            -p [String] Print dictionary file path	, default: None
            -v          Verify by Decode - longer	, default: no
            -b [Int]    Take break time to measure program memory, Seconds, default: no
            -u [String] custom urls for lookup in testhash , default: use the original urls file
        
         2. Building dictionary and encoding input file using existing dictionary 
            CMD: build, encode
            All of the above flags and:
            -d [String] dicionary filepath	, default: "[urlsfile].dict" 
            -x 0 To skip Offline stage, this yelp wrong compression ratio, default: 1 
        
         3. compress file CMD: compress, extract -f [input file] -o [output file] <compression params> 
        
         Compression params: 
         ------------------ 
            -l [char]    Longest Prefix Match - split dictionary by /, default: false
            -k [Int]     k-gram size			, default: 3
            -r [Fload]   consecutive k-gram ratio	, default: 0.5
            -n [Int]     number of pattern anchors	, default: 100
         Debugging flags:
         ---------------  
            -s          dump Aho Corasik state machine	, default: No
            -c [String] logger config file		, default: None 
        
            -h prints this message


# Repeating the tests in the article
* Test files with urls are provided in ```test_files/```
* Some of the files are to big for github so they are in 7z container
* The URLs file that used in the article is ```test_files/full_domains_shuffled ```
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
