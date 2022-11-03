#pragma once

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

#include "swap.hpp"

#include <thread>
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

    MemoryBlock(const MemoryBlock &) = delete;
    MemoryBlock &operator=(const MemoryBlock &) = delete;
	
	template<typename T> 
	class AutoLocker {
		T* ptr;
		MemoryBlock* block;
		bool wasLocked;
	public:
		AutoLocker(T* ptr, MemoryBlock* block) 
			: ptr(ptr), block(block), wasLocked(block->isLocked())
		{
			if(!wasLocked) 
				block->lock(); 
		}
		~AutoLocker() { 
			if(!wasLocked)
				block->unlock(); 
		}
		operator T*() { return ptr; }
	};

  public:
    MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size,
                bool locked_, MemoryPool *pool);

    MemoryBlock(MemoryBlock &&);
    MemoryBlock &operator=(MemoryBlock &&);
	
	template<typename T=char>
	AutoLocker<T> data() {
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

// --------------------------------------------------------
// class MemoryPool
// --------------------------------------------------------
class MemoryPool {
    friend class MemoryBlock;

    size_t numBlocks;
    size_t blockSize;
    size_t totalSize;
    char *memoryPtr;
    char *nextBlock;
    std::mutex poolMutex;
    std::mutex swapMutex;

    std::mutex blockMutex;
    std::condition_variable conditionVariable;
    std::vector<bool> blockIsLocked;

    DiskSwap *diskSwap;

    void *privateAlloc();
    void privateFree(void *ptr);

    size_t blockIndexByAddress(void *ptr);
    char *blockAddressByIndex(size_t index);

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

  public:
    MemoryPool(size_t numBlocks, size_t blockSize);
    ~MemoryPool();

    void lockBlock(void *ptr);
    void unlockBlock(void *ptr);

    MemoryBlock getBlock(size_t size);
    void freeBlock(void *ptr, size_t id);
};
