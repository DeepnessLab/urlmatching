/* 
 * File:   Signature.h
 * Author: golanp
 * Edited by Michal
 * Created on November 30, 2013, 2:27 PM
 */

#ifndef SIGNATURE_H
#define	SIGNATURE_H

#include <iostream>
#include <iomanip>
#include "lcu_buffer.h"
#include <limits>


typedef struct signature {
    
    signature() {
    }
    
    signature(buffer_t& sig_data, int hh_count, int real_count, 
              int real_count_all_series, int src_count=0, double cover_rate=0.0,
			  size_t firstPacket = 0)
    :  data(sig_data) {
        this->hh_count              = hh_count;
        this->real_count            = real_count;
        this->real_count_all_series = real_count_all_series;
        this->src_count             = src_count;
        this->cover_rate            = cover_rate;
        this->firstPacket = firstPacket;
    }
    
    inline bool operator < (const struct signature &other) const {
        return this->real_count_all_series < other.real_count_all_series;
    }
    
    void print() const {
                
        std::cout << "##";
        
        std::ios::fmtflags f( std::cout.flags() );
        for (size_t i = 0; i < this->data.size(); i++) {
            unsigned char c = this->data[i];
            
            if (c > 31 && c < 127) {
                std::cout << c;
            } else {
                std::cout << "\\x" << std::setw(2) << std::setfill('0') << std::hex << int(c);
            }
        }
        std::cout.flags( f );
                
        std::cout << std::setprecision (6) 
                  << "##,hh_count," << this->hh_count 
                  << ",real_count," << this->real_count 
                  << ",real_count_all_series," << this->real_count_all_series 
                  << ",src_count," << this->src_count 
                  << ",cover_rate," << this->cover_rate
                  << std::endl; 
    }
    
    buffer_t data;
    int      hh_count;
    int      real_count;
    int      real_count_all_series;
    int      src_count;
    double   cover_rate;
  size_t firstPacket;

} signature_t;


#endif	/* SIGNATURE_H */

