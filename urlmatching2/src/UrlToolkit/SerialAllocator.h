/*
 * SerialAllocator.h
 *
 *  Created on: 20 αιεμ 2015
 *      Author: Daniel
 */

#ifndef URLTOOLKIT_SERIALALLOCATOR_H_
#define URLTOOLKIT_SERIALALLOCATOR_H_

template <class T>
class SerialAllocator {
public:
	SerialAllocator (uint64_t reserved_size) {
		_idx = 0;
		if (reserved_size < 1) {
			_size = 0;
			_allocator = 0;
			return;
		}
		_size = reserved_size;
		_allocator = new T[reserved_size+1];
	}

	virtual ~SerialAllocator() {
		delete[] _allocator;
	}

	T* alloc(uint64_t size) {
		if (_idx >= _size)
			return 0;
		T* ret = &_allocator[_idx];
		_idx+=size;
		return ret;
	}

	inline bool 	canAlloc(size_t size) const { return (get_free_size() >= size); }
	inline uint64_t size() const 				{ return _idx; }
 	inline uint64_t capacity() const 			{ return _size; }
	inline uint64_t get_free_size() const 		{ return (_size - _idx) ; }

private:
	T* _allocator;
	uint64_t _idx;			//next available location
	uint64_t _size;		//last location in the allocator

};

#endif /* URLTOOLKIT_SERIALALLOCATOR_H_ */
