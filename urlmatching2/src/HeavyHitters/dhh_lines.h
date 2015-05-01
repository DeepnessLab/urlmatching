/* 
 * File:   SignatureExtractor.h
 */

#ifndef LSIGNATUREEXTRACTOR_H
#define	LSIGNATUREEXTRACTOR_H

#include <string>
#include <list>
#include "line_iterator.h"
#include "lcu_buffer.h"
#include "signature.h"
#include "common.h"


class LDHH {
    
public:
    
    LDHH(const std::string& pcap_filepath, int n1, int n2, float r, size_t kgram_size);
    
    LDHH(const std::string& pcap_filepath, int n1, int n2, int n3, float r, 
        size_t kgram_size, std::list<signature_t>* white_list = NULL);
    
    virtual ~LDHH();
    
public:
    
    void                    run();
    size_t                  get_pckt_count();
    std::list<signature_t>& get_signatures();
    std::list<signature_t>& get_signatures_sets();
    
private:
    
    inline size_t _handle_pckt(const raw_buffer_t &pckt);    
    inline bool   _is_sig_in_white_list(raw_buffer_t& sig_candidate);
    void          _produce_signatures(LCU_type* hh, std::list<signature_t>& signatures);
    void          _white_list_to_hash(std::list<signature_t>* white_list);
    void          _input_to_hh3(const raw_buffer_ordered_set_t& signature_set);
    
private:
    LineIterator		   _line_it;
    size_t                 _pckt_count;
    
    size_t                 _kgram_size;
    float                  _r;
    LCU_type*              _hh1;
    LCU_type*              _hh2;
    LCU_type*              _hh3;
    
    std::list<signature_t> _signatures;
    std::list<signature_t> _signatures_sets;
    
    raw_buffer_hash_set_t* _white_list_permutation;
	bool				   _use_pcaps;
};


extern std::vector<unsigned char> delimiter;

#endif	/* SIGNATUREEXTRACTOR_H */

