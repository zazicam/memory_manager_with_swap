#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

#include "logger.hpp"
#include "memory_pool.hpp"

// --------------------------------------------------------
// class MemoryPool
// --------------------------------------------------------
MemoryPool::MemoryPool(size_t numBlocks, size_t blockSize)
    : numBlocks(numBlocks), blockSize(blockSize),
      totalSize(numBlocks * blockSize) {
    assert(numBlocks > 0);
    assert(
        blockSize >=
        sizeof(
            char *)); // empty block contains a pointer to the next empty block

    memoryPtr = static_cast<char *>(malloc(totalSize));
    if (!memoryPtr) {
        std::cerr << "MemoryPool() can't allocate " << totalSize
                  << " bytes of memory!" << std::endl;
        exit(1);
    }

    // create internal list of blocks
    for (size_t i = 0; i < numBlocks - 1; ++i) {
        char *p = memoryPtr + i * blockSize;
        *reinterpret_cast<char **>(p) = static_cast<char *>(p + blockSize);
    }
    *reinterpret_cast<char **>(memoryPtr + (numBlocks - 1) * blockSize) =
        nullptr;

    nextBlock = memoryPtr;

    // locker for each block
    blockIsLocked.resize(numBlocks, 0);

    // create disk swap
    diskSwap = new DiskSwap(memoryPtr, numBlocks, blockSize);

    std::cout << "Created memory pool " << numBlocks << " blocks x "
              << blockSize << " bytes" << std::endl;
}

MemoryPool::~MemoryPool() {
    delete diskSwap;
    std::free(memoryPtr);
}

MemoryBlock MemoryPool::getBlock(size_t size) {
    std::lock_guard<std::mutex> poolGuard(poolMutex);

    SwapIdType blockId = 1;
    void *ptr = privateAlloc();

    size_t blockIndex = 0;
    if (ptr) {
        blockIndex = blockIndexByAddress(ptr);
        swapMutex.lock();
        diskSwap->MarkBlockAllocated(blockIndex, blockId);
        swapMutex.unlock();
		stat.usedCounter++;
    } else {
        // No free blocks in pool, try to use swap

        // Random block for test :)
        // blockIndex = rand() % numBlocks;

		// The oldest allocated block for task
		if(!swapQueue.empty()) {
			blockIndex = swapQueue.front();
			swapQueue.pop();
		}

        ptr = blockAddressByIndex(blockIndex);

        lockBlock(ptr);
        swapMutex.lock();

	 	// returns unique id for each new block
        blockId = diskSwap->Swap(blockIndex);

        diskSwap->MarkBlockAllocated(blockIndex, blockId);
        swapMutex.unlock();
        unlockBlock(ptr);
		stat.swappedCounter++;
    }
	swapQueue.push(blockIndex);
    return MemoryBlock{ptr, blockId, blockSize, size, false, this};
}

void *MemoryPool::privateAlloc() {
    if (nextBlock) {
        char *block = nextBlock;
        nextBlock = *reinterpret_cast<char **>(nextBlock);
        return block;
    }

    // No free blocks
    return nullptr;
}

void MemoryPool::privateFree(void *ptr) {
    assert(ptr != nullptr);
    char *block = static_cast<char *>(ptr);
    *reinterpret_cast<char **>(block) = nextBlock;
    nextBlock = block;
}

size_t MemoryPool::blockIndexByAddress(void *ptr) {
    return (static_cast<char *>(ptr) - memoryPtr) / blockSize;
}

char *MemoryPool::blockAddressByIndex(size_t index) {
    return memoryPtr + index * blockSize;
}

void MemoryPool::lockBlock(void *ptr) {
    size_t blockIndex = blockIndexByAddress(ptr);
    std::unique_lock<std::mutex> ul(blockMutex);
    conditionVariable.wait(
        ul, [=]() { return blockIsLocked.at(blockIndex) == 0; });
    blockIsLocked.at(blockIndexByAddress(ptr)) = 1;
    ul.unlock();
	stat.lockedCounter++;
}

void MemoryPool::unlockBlock(void *ptr) {
    std::unique_lock<std::mutex> ul(blockMutex);
	blockIsLocked.at(blockIndexByAddress(ptr)) = 0;
	conditionVariable.notify_one();
    ul.unlock();
	stat.lockedCounter--;
}

void MemoryPool::freeBlock(void *ptr, SwapIdType id) {
    std::lock_guard<std::mutex> poolGuard(poolMutex);
    lockBlock(ptr);
    swapMutex.lock();
    size_t blockIndex = blockIndexByAddress(ptr);
    if (diskSwap->isBlockInSwap(blockIndex, id)) {
        // it's in swap, let's just mark it freed (in swapTable)
        diskSwap->MarkBlockFreed(blockIndex, id);
		stat.swappedCounter--;
    } else {
		// it's it ram
        if (diskSwap->HasSwappedBlocks(blockIndex)) {
            diskSwap->ReturnLastSwappedBlockIntoRam(blockIndex);
			stat.swappedCounter--;
        } else {
            privateFree(ptr);
            diskSwap->MarkBlockFreed(blockIndex, id);
			stat.usedCounter--;
        }
    }
    swapMutex.unlock();
    unlockBlock(ptr);
}

const PoolStat& MemoryPool::getStatistics() const {
	return stat;
}

size_t MemoryPool::getNumBlocks() const {
    std::lock_guard<std::mutex> poolGuard(poolMutex);
	return numBlocks;
}
