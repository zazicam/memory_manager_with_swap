#pragma once

#include <cassert>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "swap.hpp"

class MemoryPool;

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
class MemoryBlock {
    void* ptr_;
    size_t id_;
    size_t capacity_;
	size_t size_; 
    MemoryPool *pool_;

//    MemoryBlock(const MemoryBlock&) = delete;
//    MemoryBlock &operator=(const MemoryBlock &) = delete;

  public:
    MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size, MemoryPool *pool);

    template <typename T = char> T *getPtr() {
//		std::cout << "getPtr()" << std::endl;
        return static_cast<T *>(ptr_);
    }

    size_t size();
	size_t capacity(); 

    void lock();
    void unlock();
    void free();

	void debugPrint();
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
	std::condition_variable cv;
	std::vector<bool> blockIsLocked;

    DiskSwap *diskSwap;

    void *privateAlloc();
	void privateFree(void *ptr); 

    size_t blockIndexByAddress(void *ptr);
	char* blockAddressByIndex(size_t index);

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

  public:
    MemoryPool(size_t numBlocks, size_t blockSize);
    ~MemoryPool();

	void lockBlock(void *ptr); 
	void unlockBlock(void *ptr); 

    MemoryBlock getBlock(size_t size);
	void freeBlock(void* ptr, size_t id); 
};
