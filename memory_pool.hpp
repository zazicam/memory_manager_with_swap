#pragma once

#include <cassert>
#include <cstddef>
#include <mutex>
#include <vector>

#include "swap.hpp"

class MemoryPool;

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
class MemoryBlock {
    void* ptr;
    size_t id;
    size_t size;
    MemoryPool *pool;

  public:
    MemoryBlock(void *ptr, size_t id, size_t size, MemoryPool *pool);

    template <typename T = char> T *getPtr() {
        lock();
        return static_cast<T *>(ptr);
    }

    void lock();
    void unlock();
    void free();
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
    std::vector<std::mutex> blockMutex;
    DiskSwap *swap;

    void *privateAlloc();
    void privateFree(void *ptr, size_t id);

    size_t blockIndexByAddress(void *ptr);
	char* blockAddressByIndex(size_t index);

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

  public:
    MemoryPool(size_t numBlocks, size_t blockSize);
    ~MemoryPool();

	void lockBlock(void *ptr); 
	void unlockBlock(void *ptr); 

    MemoryBlock getBlock();
};
