
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <assert.h>
#include <ctime>
#include <unordered_map>
#include <cstdlib>

#include "tester.h"
#include "../UrlToolkit/Huffman.h"
#include "../UrlToolkit/UrlMatching.h"
#include "../UrlToolkit/FileCompressor.h"
#include "../HeavyHitters/dhh_lines.h"
#include "../logger.h"
#include "../common.h"


#define BUFFSIZE 500

#ifdef DVAL
#undef DVAL
#endif
#define DVAL(what) #what" = "<< (what)

