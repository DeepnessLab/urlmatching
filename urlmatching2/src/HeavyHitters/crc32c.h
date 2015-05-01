// Copyright 2008,2009,2010 Massachusetts Institute of Technology.
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef LOGGING_CRC32C_H__
#define LOGGING_CRC32C_H__


#include <stdint.h>
//#include "../config.h"
//#include <config.h>
#include "crc32ctables.h"

/** Returns the initial value for a CRC32-C computation. */
static inline uint32_t crc32cInit() {
    return 0xFFFFFFFF;
}


/** Converts a partial CRC32-C computation to the final value. */
static inline uint32_t crc32cFinish(uint32_t crc) {
    return ~crc;
}

#ifdef HAVE_HW_CRC32

// Hardware-accelerated CRC-32C (using CRC32 instruction)
static inline uint32_t crc32cHardware32(uint32_t crc, const void* data, size_t length) {
    const char* p_buf = (const char*) data;
    //added by Michal
    //uint32_t crc32bit = 0;
      // alignment doesn't seem to help?
    for (size_t i = 0; i < length / sizeof(uint32_t); i++) {
        crc = __builtin_ia32_crc32si(crc, *(uint32_t*) p_buf);
        p_buf += sizeof(uint32_t);
    }
    
    // This ugly switch is slightly faster for short strings than the straightforward loop
    length &= sizeof(uint32_t) - 1;
    
    /* while (length > 0) { */
    /*    crc32bit = __builtin_ia32_crc32qi(crc32bit, *p_buf++); */
    /*    length--; */
    /*  } */
    
    switch (length) {
        case 3:
            crc = __builtin_ia32_crc32qi(crc, *p_buf++);
        case 2:
            crc = __builtin_ia32_crc32hi(crc, *(uint16_t*) p_buf);
            break;
        case 1:
            crc = __builtin_ia32_crc32qi(crc, *p_buf);
            break;
        case 0:
            break;
        default:
            break;
    }

    return crc;
}

//commented out - should add ifdef since this won't compile on 32bit..
// // Hardware-accelerated CRC-32C (using CRC32 instruction)
 /* static inline uint32_t crc32cHardware64(uint32_t crc, const void* data, size_t length) { */
 /*     const char* p_buf = (const char*) data; */
 /*     // alignment doesn't seem to help? */
 /*     uint64_t crc64bit = crc; */
 /*     for (size_t i = 0; i < length / sizeof(uint64_t); i++) { */
 /*         crc64bit = __builtin_ia32_crc32di(crc64bit, *(uint64_t*) p_buf); */
 /*         p_buf += sizeof(uint64_t); */
 /*     } */

 /*     // This ugly switch is slightly faster for short strings than the straightforward loop */
 /*     uint32_t crc32bit = (uint32_t) crc64bit; */
 /*     length &= sizeof(uint64_t) - 1; */
 /*     /\* */
 /*     while (length > 0) { */
 /*         crc32bit = __builtin_ia32_crc32qi(crc32bit, *p_buf++); */
 /*         length--; */
 /*     } */
 /*     *\/ */
 /*     switch (length) { */
 /*         case 7: */
 /*             crc32bit = __builtin_ia32_crc32qi(crc32bit, *p_buf++); */
 /*         case 6: */
 /*             crc32bit = __builtin_ia32_crc32hi(crc32bit, *(uint16_t*) p_buf); */
 /*             p_buf += 2; */
 /*         // case 5 is below: 4 + 1 */
 /*         case 4: */
 /*             crc32bit = __builtin_ia32_crc32si(crc32bit, *(uint32_t*) p_buf); */
 /*             break; */
 /*         case 3: */
 /*             crc32bit = __builtin_ia32_crc32qi(crc32bit, *p_buf++); */
 /*         case 2: */
 /*             crc32bit = __builtin_ia32_crc32hi(crc32bit, *(uint16_t*) p_buf); */
 /*             break; */
 /*         case 5: */
 /*             crc32bit = __builtin_ia32_crc32si(crc32bit, *(uint32_t*) p_buf); */
 /*             p_buf += 4; */
 /*         case 1: */
 /*             crc32bit = __builtin_ia32_crc32qi(crc32bit, *p_buf); */
 /*             break; */
 /*         case 0: */
 /*             break; */
 /*         default: */
 /*             break; */
 /*     } */

 /*     return crc32bit; */
 /* } */






/** Computes a complete CRC32C over data, using crc32c. */
static inline uint32_t crc32cComplete(const void* data, size_t length) {  
    #ifdef __LP64__
    return crc32cFinish(crc32cHardware64(0xFFFFFFFF, data, length));
    #else
    return crc32cFinish(crc32cHardware32(0xFFFFFFFF, data, length));
    #endif    
}

#else  // HAVE_HW_CRC32

static inline uint32_t crc32cSlicingBy8(uint32_t crc, const void* data, size_t length) {
    const char* p_buf = (const char*) data;

    // Handle leading misaligned bytes
    size_t initial_bytes = (sizeof(int32_t) - (intptr_t)p_buf) & (sizeof(int32_t) - 1);
    if (length < initial_bytes) initial_bytes = length;
    for (size_t li = 0; li < initial_bytes; li++) {
        crc = crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);
    }

    length -= initial_bytes;
    size_t running_length = length & ~(sizeof(uint64_t) - 1);
    size_t end_bytes = length - running_length; 

    for (size_t li = 0; li < running_length/8; li++) {
        crc ^= *(uint32_t*) p_buf;
        p_buf += 4;
        uint32_t term1 = crc_tableil8_o88[crc & 0x000000FF] ^
                crc_tableil8_o80[(crc >> 8) & 0x000000FF];
        uint32_t term2 = crc >> 16;
        crc = term1 ^
              crc_tableil8_o72[term2 & 0x000000FF] ^ 
              crc_tableil8_o64[(term2 >> 8) & 0x000000FF];
        term1 = crc_tableil8_o56[(*(uint32_t *)p_buf) & 0x000000FF] ^
                crc_tableil8_o48[((*(uint32_t *)p_buf) >> 8) & 0x000000FF];

        term2 = (*(uint32_t *)p_buf) >> 16;
        crc = crc ^ term1 ^
                crc_tableil8_o40[term2  & 0x000000FF] ^
                crc_tableil8_o32[(term2 >> 8) & 0x000000FF];
        p_buf += 4;
    }

    for (size_t li=0; li < end_bytes; li++) {
        crc = crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);
    }

    return crc;
}


static inline uint32_t crc32cComplete(const void* data, size_t length) {    
    return crc32cFinish(crc32cSlicingBy8(0xFFFFFFFF, data, length));
}

#endif  // HAVE_HW_CRC32


#endif
