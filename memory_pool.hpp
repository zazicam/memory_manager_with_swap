#pragma once

#include <cassert>
#include <cstddef>
#include <map>
#include <mutex>
#include <vector>

#include "swap.hpp"

class BlockAddress {
    bool inMemory;
    void *memoryAddress;
    size_t fileAddress;

  public:
    BlockAddress();
    BlockAddress(void *memoryAddress);

    void moveToFile(size_t fileAddress);
    void moveToMemory();
    bool isInMemory();
    bool isInSwap();
    void *getMemoryAddress();
    size_t getFileAddress();
};

struct MemoryBlockState {
    size_t id;
    bool locked;
};

class MemoryPool;

class MemoryBlock {
    size_t id;
    void *ptr;
    size_t size;
    MemoryPool *pool;

  public:
    MemoryBlock(size_t id, void *ptr, size_t size, MemoryPool *pool);

    template <typename T = char> T *getPtr() {
        lock();
        return static_cast<T *>(ptr);
    }

    void lock();
    void unlock();
    void free();
};

class MemoryPool {
    friend class MemoryBlock;

    size_t numBlocks;
    size_t blockSize;
    size_t totalSize;
    char *memoryPtr;
    char *nextBlock;
    std::mutex poolMutex;
    static size_t blocksCounter;

    std::vector<MemoryBlockState> blockState;
    size_t unlockedBlocksCounter;
    std::mutex blockStateMutex;

    std::map<size_t, BlockAddress> blockAddress;
    Swap *swap;

    void *privateAlloc();
    void privateFree(void *ptr, size_t id);

    size_t blockIndexByAddress(void *ptr);
    void lockBlock(void *ptr);
    void unlockBlock(void *ptr);

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;

  public:
    MemoryPool(size_t numBlocks, size_t blockSize);
    ~MemoryPool();

    MemoryBlock getBlock();
};
