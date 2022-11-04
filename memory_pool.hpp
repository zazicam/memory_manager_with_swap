#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

#include "swap.hpp"
#include "logger.hpp"
#include "memory_block.hpp"

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
