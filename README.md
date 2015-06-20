# urlmatching
This is a C++ platform independent runtime tool that provides statistics to the ***"Scalable URL Matching with Small Memory Footprint"***
To lean how to use the code explore ```test.cpp```, espacially ```void test_main()```
This tool uses the ```easylogging++``` framework which is as well platform (WIN/LINUX) independent
https://github.com/danielonweb/easyloggingpp.git

# How to run 
```urlmatching32.exe -f \<file with urls from test_files.7z>```
- to explore all runtime options see urlmatching.exe -h

        Usage: urlcompressor -f <urls_path> [options]
                -f String[urls filepath]  - required
                -o String[ouput filepath], default: output.txt
                -a       [add header to output_filepath], default: None
                -k Int   [k-gram size], default: 8
                -1 Int   [heavy hitters count for HH1], default: 1000
                -2 Int   [heavy hitters count for HH2], default: 1000
                -r fload [consecutive k-gram ratio], default: 0.8
                -l       [Longest Prefix Match - split dictionary by /], default: false
                -p String[Print dictionary file path], default: None
                -b Int   [Take break time to measure program memory, Seconds], default: no
                -d       [Verify by Decode - longer], default: no
                -c String[logger config file], default: None
        -h prints this message

# test files
Some big .txt files with urls are provided in ```test_files/```

# How to build
##***Release & Debug***
    
    -std=c++0x
    
***DEBUG build***
    
    -DBUILD_DEBUG
    
## Loggin flags
In ```src/logger.h``` you can control how debug prints are handled, simply configure the following code. Repository default is all set to *NONE*
```
#ifdef BUILD_DEBUG
#define DEBUG_OUTPUT DBG_TO_NONE
#else
#define DEBUG_OUTPUT DBG_TO_NONE
#endif
```
