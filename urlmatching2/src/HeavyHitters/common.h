/* 
 * File:   common.h
 * Author: golanp
 *
 * Created on November 23, 2013, 6:54 PM
 * Edited by Michal
 */

#ifndef COMMON_H
#define	COMMON_H

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <string>
#include <algorithm>

#include "crc32c.h"

//michal added:
//#include <memory>
#include <cstring>
// Ethernet header
struct eth_header {
    unsigned char  dest[6];
    unsigned char  source[6];
    unsigned short type;
};


//IP header (v4)
struct ip_hdr {
    unsigned char  ip_header_len:4;     // 4-bit header length (in 32-bit words) normally=5 (Means 20 Bytes may be 24 also)
    unsigned char  ip_version :4;       // 4-bit IPv4 version
    unsigned char  ip_tos;              // IP type of service
    unsigned short ip_total_length;     // Total length
    unsigned short ip_id;               // Unique identifier
    unsigned char  ip_frag_offset :5;   // Fragment offset field
    unsigned char  ip_more_fragment :1;
    unsigned char  ip_dont_fragment :1;
    unsigned char  ip_reserved_zero :1;
    unsigned char  ip_frag_offset1;     // fragment offset
    unsigned char  ip_ttl;              // Time to live
    unsigned char  ip_protocol;         // Protocol(TCP,UDP etc)
    unsigned short ip_checksum;         // IP checksum
    unsigned int   ip_srcaddr;          // Source address
    unsigned int   ip_destaddr;         // Source address
};


// TCP header
struct tcp_header {
    unsigned short source_port;         // source port
    unsigned short dest_port;           // destination port
    unsigned int   sequence;            // sequence number - 32 bits
    unsigned int   acknowledge;         // acknowledgement number - 32 bits
    unsigned char  ns :1;               // Nonce Sum Flag Added in RFC 3540.
    unsigned char  reserved_part1:3;
    unsigned char  data_offset:4;       /* The number of 32-bit words in the TCP header. 
                                         * Indicates where the data begins.
                                         * The length of the TCP header is always a multiple
                                         * of 32 bits.*/

    unsigned char  fin :1;              // Finish Flag
    unsigned char  syn :1;              // Synchronize Flag
    unsigned char  rst :1;              // Reset Flag
    unsigned char  psh :1;              // Push Flag
    unsigned char  ack :1;              // acknowledgement Flag
    unsigned char  urg :1;              // Urgent Flag
    unsigned char  ecn :1;              // ECN-Echo Flag
    unsigned char  cwr :1;              // Congestion Window Reduced Flag
    unsigned short window;              // window
    unsigned short checksum;            // checksum
    unsigned short urgent_pointer;      // urgent pointer
};

//UDP header
struct udp_header {
  unsigned short source_port;         // source port
  unsigned short dest_port;           // destination port
  unsigned short len;           // UDP header + data length
  unsigned short chksm;       //checksum
  unsigned int message; //the payload

};




typedef struct raw_buffer {
    unsigned char *ptr;
    size_t        size;
    
    inline bool operator == (const struct raw_buffer& rhs) const {
        if (size != rhs.size) {
            return false;
        }
        
        return std::memcmp(ptr, rhs.ptr, size) == 0;
    }
    
} raw_buffer_t;


class raw_buffer_t_less {
public:
    inline bool operator () (const raw_buffer_t& lhs, const raw_buffer_t& rhs) const {        
        int res = std::memcmp(lhs.ptr, rhs.ptr, std::min(lhs.size, rhs.size));
        
        if (res == 0 && lhs.size != rhs.size) {
            return lhs.size < rhs.size ? true : false;
        }
        
        return res < 0 ? true : false;
    }
};


struct hash_raw_buffer {
    size_t operator()(const raw_buffer_t& buf) const {        
        return crc32cComplete((const void *)buf.ptr, buf.size);
    }
};

typedef std::unordered_map<raw_buffer_t, char, hash_raw_buffer> raw_buffer_hash_table_t;
typedef std::unordered_set<raw_buffer_t, hash_raw_buffer>       raw_buffer_hash_set_t;
typedef std::set<raw_buffer_t, raw_buffer_t_less>               raw_buffer_ordered_set_t;

typedef std::vector<unsigned char> buffer_t;
//typedef std::deque<unsigned char> buffer_t;

#endif	/* COMMON_H */

