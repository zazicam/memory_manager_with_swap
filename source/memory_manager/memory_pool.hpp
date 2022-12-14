#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <vector>

#include "../utils/logger.hpp"
#include "memory_block.hpp"
#include "swap.hpp"

struct PoolStat {
    std::atomic<size_t> usedCounter = 0;
    std::atomic<size_t> lockedCounter = 0;
    std::atomic<size_t> swappedCounter = 0;
    std::atomic<size_t> swapLevels = 0;
};

// --------------------------------------------------------
// class MemoryPool
// --------------------------------------------------------
class MemoryPool {
    friend class MemoryBlock;
    friend class DiskSwap;
    size_t numBlocks;
    size_t blockSize;
    size_t totalSize;
    char *memoryPtr;
    char *nextBlock;
    mutable std::mutex poolMutex;
    std::mutex swapMutex;

    std::mutex blockMutex;
    std::condition_variable conditionVariable;
    std::vector<bool> blockIsLocked;

    std::queue<size_t> swapQueue;

    DiskSwap *diskSwap;
    PoolStat stat;

    void *privateAlloc();
    void privateFree(void *ptr);

    size_t blockIndexByAddress(void *ptr);
    char *blockAddressByIndex(size_t index);

  public:
    MemoryPool(size_t numBlocks, size_t blockSize);
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;
    ~MemoryPool();

    void lockBlock(void *ptr);
    void unlockBlock(void *ptr);

    MemoryBlock getBlock(size_t size);
    void freeBlock(void *ptr, SwapIdType id);

    size_t getNumBlocks() const;
    const PoolStat &getStatistics() const;
};
