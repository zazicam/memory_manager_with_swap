#pragma once

#include <cstddef>

#include "logger.hpp"

class MemoryPool;

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
class MemoryBlock {
    void* ptr_;
    size_t id_;
    size_t capacity_;
    size_t size_;
	bool locked_;
    MemoryPool *pool_;
	bool moved_;

	template<typename T> 
	class AutoLocker {
		T* ptr;
		mutable const MemoryBlock* block;
		bool wasLocked;
	public:
		AutoLocker(T* ptr, const MemoryBlock* block) 
			: ptr(ptr), block(block), wasLocked(block->isLocked())
		{
			if(!wasLocked) 
				const_cast<MemoryBlock*>(block)->lock(); 
		}
		~AutoLocker() { 
			if(!wasLocked)
				const_cast<MemoryBlock*>(block)->unlock(); 
		}
		operator T*() { return ptr; }
	};

  public:
    MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size,
                bool locked_, MemoryPool *pool);

    MemoryBlock(const MemoryBlock &) = delete;
    MemoryBlock &operator=(const MemoryBlock &) = delete;

    MemoryBlock(MemoryBlock &&);
    MemoryBlock &operator=(MemoryBlock &&);
	
	template<typename T=char>
	AutoLocker<T> data() {
		return AutoLocker{static_cast<T*>(ptr_), this};
	}

	template<typename T=const char>
	AutoLocker<T> data() const {
		return AutoLocker{static_cast<T*>(ptr_), this};
	}

    size_t size() const;
    size_t capacity() const;

    void lock();
    void unlock();
    void free();
	bool isLocked() const;

	void checkScopeError() const; // can exit(1)
    void debugPrint() const;
};

