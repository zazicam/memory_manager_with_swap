#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>
#include <atomic>
#include <queue>

#include "swap.hpp"
#include "logger.hpp"
#include "memory_block.hpp"

struct PoolStat {
	std::atomic<size_t> usedCounter = 0;
	std::atomic<size_t> lockedCounter = 0;
	std::atomic<size_t> swappedCounter = 0;
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
	const PoolStat& getStatistics() const;
};
