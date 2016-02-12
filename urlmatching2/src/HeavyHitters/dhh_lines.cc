/* 
 * by Michal 
 */

#include <unordered_map>
#include <string>
#include <iostream>
#include "dhh_lines.h"


std::vector<unsigned char> ldelimiter;

void _init_ldelimiter() {
    ldelimiter.push_back(0xa1);
    ldelimiter.push_back(0xc3);
}

LDHH::LDHH(LineIterator& line_it, int n1, int n2, float r,
         size_t kgram_size) : _line_it(line_it/*pcap_filepath.c_str() , ENDL_DELIMITER*/) {
    
    this->_kgram_size = kgram_size;
    this->_r          = r;
    
    this->_hh1        = LCU_Init(1.0 / n1);
    this->_hh2        = LCU_Init(1.0 / n2);
    this->_hh3        = NULL;
    this->_use_pcaps = false;
    this->_pckt_count = 0;
    
    this->_white_list_permutation = NULL;
    
    _init_ldelimiter();
}

//TODO: differ between pcap_filepath or simple '\n' delimiter file
LDHH::LDHH(LineIterator& line_it, int n1, int n2, int n3, float r, size_t kgram_size,
         std::list<signature_t>* white_list) : _line_it(line_it) {
    
    this->_kgram_size = kgram_size;
    this->_r          = r;
    
    this->_hh1        = LCU_Init(1.0 / n1);
    this->_hh2        = LCU_Init(1.0 / n2);
    this->_hh3        = LCU_Init(1.0 / n3);
    this->_use_pcaps = true;
    
    this->_pckt_count = 0;
    
    this->_white_list_permutation = NULL;
    this->_white_list_to_hash(white_list);
    _init_ldelimiter();    
}

LDHH::~LDHH() {
    
    delete this->_white_list_permutation;
    
    LCU_Destroy(this->_hh3);
    LCU_Destroy(this->_hh2);
    LCU_Destroy(this->_hh1);
    
}

bool LDHH::run() {
    
	if (!_line_it.canRun() ){
		return false;
	}

    while (this->_line_it.has_next() ) {
        const raw_buffer_t &pckt = this->_line_it.next();
        
        this->_handle_pckt(pckt);
        
        this->_pckt_count++;
        
    }
    
    this->_produce_signatures(this->_hh2, this->_signatures);
    
    if (this->_hh3) {
        this->_produce_signatures(this->_hh3, this->_signatures_sets);
    }
    
    return true;
}

inline size_t LDHH::_handle_pckt(const raw_buffer_t &pckt) {
    
    size_t kgram_size = this->_kgram_size;
    
    if (pckt.size < kgram_size) {
        return 0;
    }
    
    float r = this->_r;
    
    raw_buffer_ordered_set_t signature_set;
    raw_buffer_hash_table_t  seen_kgram_series(1023);
            
    raw_buffer_t kgram_series = {NULL, 0};
    int kgram_series_hh_count = 0;
    
    int string_set_hh_count   = 0;
            
    // go over all k-grams in the packet...
    raw_buffer_t kgram         = {pckt.ptr, kgram_size};
    size_t       scan_end_indx = pckt.size - kgram_size + 1;
    
    for (register size_t i = 0; i < scan_end_indx; i++) {
        
        int  kgram_hh_count = 0;
        bool should_input_to_hh2 = false;
        bool should_begin_series_with_current_kgram = false;
        

        kgram_hh_count = LCU_Update(this->_hh1, kgram.ptr, kgram.size, this->_pckt_count);

        if (kgram_hh_count > 0) {
            
            if (kgram_series.ptr == NULL) {

                kgram_series.ptr      = kgram.ptr;
                kgram_series.size     = kgram.size;

                kgram_series_hh_count = kgram_hh_count;

            } else {

                // TODO: test - use only second condition
                if ( (kgram_hh_count >= kgram_series_hh_count) ||
                     (kgram_hh_count > kgram_series_hh_count * r ) ) {

                    ++kgram_series.size;
                    kgram_series_hh_count = kgram_hh_count;

                } else {

                    should_input_to_hh2                    = true;
                    should_begin_series_with_current_kgram = true;

                }

            }
            
        } else {

            should_input_to_hh2 = true;

        }            

        // handle k-gram series when it was finished to be collected...
        bool is_last_pckt_kgram = ( (i + 1) == scan_end_indx );

        if (should_input_to_hh2 || is_last_pckt_kgram) {
            // check if we at the end of a consecutive k-gram series
            if (kgram_series.ptr) {
                
                // check if k-gram series has already been seen in this packet...
                raw_buffer_hash_table_t::iterator it = seen_kgram_series.find(kgram_series);
                
                if (it == seen_kgram_series.end()) {                    
                    seen_kgram_series[kgram_series] = 0;
                                                       
                    bool is_sig_in_whitelist = this->_is_sig_in_white_list(kgram_series);

                    if ( !is_sig_in_whitelist ) {
                        
                        int string_hh_count = LCU_Update(this->_hh2, kgram_series.ptr, kgram_series.size, this->_pckt_count);
                       
                        // handle signature sets only if HH3 is enabled for this run
                        if ( this->_hh3 ) {
                            if (string_hh_count > string_set_hh_count * r ) {

                                signature_set.insert(kgram_series);
                                string_set_hh_count = string_hh_count;

                            }
                        }
                    }
                    
                }
                
                if (should_begin_series_with_current_kgram) {
                    kgram_series.ptr      = kgram.ptr;
                    kgram_series_hh_count = kgram_hh_count;
                } else {
                    kgram_series.ptr      = NULL;
                    kgram_series_hh_count = 0;
                }
            }
        }

        ++kgram.ptr;

    }
    
    if ( this->_hh3 && signature_set.size() > 0) {
        this->_input_to_hh3(signature_set);
    }
    
    return 0;
}

void LDHH::_white_list_to_hash(std::list<signature_t>* white_list) {    
    if (NULL == white_list || this->_white_list_permutation)
        return;
        
    this->_white_list_permutation = new raw_buffer_hash_set_t;
    
    raw_buffer_hash_set_t& white_list_permutation_ref = *this->_white_list_permutation;
    
    std::list<signature_t>& white_list_ref = *white_list;
    std::list<signature_t>::iterator it;
     
    // for all white list signatures, extract all sub signatures...
    for (it = white_list_ref.begin(); it != white_list_ref.end(); it++) {
        signature_t &sig = *it;
        
        size_t sub_sig_size_limit = sig.data.size() - this->_kgram_size + 1;        
        size_t sub_sig_size       = this->_kgram_size;
                                      
        for (register size_t i = 0; i < sub_sig_size_limit; i++, sub_sig_size++) {
                        
            // extract all sub_sig_size-gram permutations...
            raw_buffer_t permutation = {&sig.data[0], sub_sig_size};                                    
            size_t scan_end_indx     = sig.data.size() - sub_sig_size + 1;

            for (register size_t j = 0; j < scan_end_indx; j++) {
                white_list_permutation_ref.insert(permutation);
                
                ++permutation.ptr;
            }       
            
        }               
    }
}

inline bool LDHH::_is_sig_in_white_list(raw_buffer_t& sig_candidate) {
    if (NULL == this->_white_list_permutation) {
        return false;
    }

    raw_buffer_hash_set_t::iterator it = this->_white_list_permutation->find(sig_candidate);                

    return it == this->_white_list_permutation->end() ? false : true;
}

void LDHH::_produce_signatures(LCU_type* hh, std::list<signature_t>& signatures) {
    
    LCU_Calc_Total_Count(hh);
    
    std::vector<LCUITEM *> item_list = LCU_List_Items(hh);    
    std::vector<LCUITEM *>::iterator items_it;
    
    for (items_it = item_list.begin(); items_it != item_list.end(); items_it++) {
        LCUITEM *item = *items_it;
                
        signatures.push_back(signature_t(item->item, item->parentg->count, item->real_count, item->real_count_all_series));
    }
    
    signatures.sort();
    
}


std::list<signature_t>& LDHH::get_signatures() {
    return this->_signatures;
}

size_t LDHH::get_pckt_count() {
    return this->_pckt_count;
}

std::list<signature_t>& LDHH::get_signatures_sets() {
    return this->_signatures_sets;
}


void LDHH::_input_to_hh3(const raw_buffer_ordered_set_t& signature_set) {
    
    buffer_t set_series;
    raw_buffer_ordered_set_t::iterator set_it;
    
    
    for (set_it = signature_set.begin(); set_it != signature_set.end(); set_it++) {
        
        const raw_buffer_t& signature = *set_it;
        
        std::vector<unsigned char> tmp_signature(signature.ptr, signature.ptr + signature.size);
        
        set_series.reserve(set_series.size()+ldelimiter.size()+tmp_signature.size());
        
        set_series.insert(set_series.end(), ldelimiter.begin(), ldelimiter.end());        
        set_series.insert(set_series.end(), tmp_signature.begin(), tmp_signature.end());  
        
    }
    set_series.insert(set_series.end(), ldelimiter.begin(), ldelimiter.end());
    
    LCU_Update(this->_hh3, &set_series[0], set_series.size(), this->_pckt_count);
    
}
