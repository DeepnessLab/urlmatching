/*
 * PackedCode.h
 *
 *  Created on: 22 αιεμ 2015
 *      Author: Daniel
 */

#ifndef URLTOOLKIT_PACKEDCODE_H_
#define URLTOOLKIT_PACKEDCODE_H_

#include "UrlDictionaryTypes.h"


//Convert uint32_t buff from LittleEndian to BigEndian and vice versa
inline
void conv_LE_BE(uint32_t* buff, uint32_t len) {
	//	return;
	for (uint32_t i = 0; i < len; i++) {
		uint32_t num = buff[i];
		uint32_t swapped = ((num >> 24) & 0xff) | // move byte 3 to byte 0
				((num << 8) & 0xff0000) | // move byte 1 to byte 2
				((num >> 8) & 0xff00) | // move byte 2 to byte 1
				((num << 24) & 0xff000000); // byte 0 to byte 3
		buff[i] = swapped;
	}
}

inline
int is_big_endian(void) {
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	return bint.c[0] == 1;
}

inline void conv_to_big_endian(uint32_t& _4bytes) {
	if (!is_big_endian()) {
		conv_LE_BE(&_4bytes, 1);
	}
}

inline void conv_from_big_endian(uint32_t& _4bytes) {
	conv_to_big_endian(_4bytes);
}


namespace CodePack {

typedef uint16_t lenT ;

struct CodePackT {
	char* buf;

	inline lenT getBitLen () const {
		lenT* len = (lenT*) buf;
		return *len;
	}

	inline void setBitLen (lenT len) {
		lenT* plen = (lenT*) buf;
		*plen = len;
		return;
	}

	inline char* getBuff () const {
		char* ret = buf + sizeof (lenT);
		return ret;
	}

	inline uint32_t getByteSize() const {
		lenT bit_len = getBitLen();
		return conv_bits_to_bytes(bit_len);
	}

	inline uint32_t getU32Size() const {
		lenT bit_len = getBitLen();
		return conv_bits_to_uin32_size(bit_len);
	}

	//Packing uint32 in to char buffer:
	// the last uin32 might be truncated hence it is important that it will
	// be big endian
	// so when packing and unpacking if the system is little endian - we flip
	// the last position

	inline void Pack(lenT bit_len, uint32_t* in_u32buf, SerialAllocator<char>* allocator) {

		uint32_t bytesize = conv_bits_to_bytes(bit_len);
		buf = allocator->alloc(sizeof(lenT) + bytesize);
		setBitLen(bit_len);
		uint32_t u32size = getU32Size();

		//flip the last u32 (because only that might be truncated)
		uint32_t* t = &(in_u32buf[u32size-1]);
		conv_to_big_endian(*t);

		char* write_buf = getBuff();
		memcpy (write_buf, in_u32buf, bytesize);
		return ;
	}

	inline void UnPack(uint32_t* out_u32buf) {
		uint32_t bytesize = getByteSize();
		uint32_t u32size = getU32Size();

		out_u32buf[u32size-1] = 0;
		char* buf_ = getBuff();
		memcpy (out_u32buf, buf_, bytesize);

		//flip the last u32 (because only that might be truncated)
		conv_from_big_endian(out_u32buf[u32size-1]);
	}


	bool operator==(const CodePackT &other) const 	{
		lenT left_len = this->getBitLen();
		lenT right_len = other.getBitLen();

		if (left_len != right_len)
			return false;
		char* left = this->getBuff();
		char* right = other.getBuff();
		for (uint32_t i = 0; i <= left_len ; i++) {
			if (*left != *right)
				return false;
			left++;
			right++;
		}
		return true;
	}

};







}	//namespace CodePack



#endif /* URLTOOLKIT_PACKEDCODE_H_ */
