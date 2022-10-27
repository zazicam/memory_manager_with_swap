#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

#include "memory_pool.hpp"

#define UNUSED(var) ((void)var) // for debug

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
MemoryBlock::MemoryBlock(void *ptr, size_t id, size_t blockSize, size_t getSize, MemoryPool *pool)
    : ptr(ptr), id(id), blockSize(blockSize), getSize(getSize), pool(pool) {}

size_t MemoryBlock::size() {
    return getSize;
}

size_t MemoryBlock::capacity() {
	return blockSize;
}

void MemoryBlock::lock() {
    pool->lockBlock(ptr);
}

void MemoryBlock::unlock() {
    pool->unlockBlock(ptr);
}

void MemoryBlock::free() {
    pool->freeBlock(ptr, id);
}

// --------------------------------------------------------
// class MemoryPool
// --------------------------------------------------------
MemoryPool::MemoryPool(size_t numBlocks, size_t blockSize) 
	: numBlocks(numBlocks), 
	blockSize(blockSize),
	totalSize(numBlocks * blockSize)
{
    assert(numBlocks > 0);
    assert(blockSize >= sizeof(char*)); // empty block contains a pointer to the next empty block

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
    nextBlock = memoryPtr;

	// mutex for each block
	blockMutex = std::vector<std::mutex>(numBlocks);

    // create disk swap
    swap = new DiskSwap(memoryPtr, numBlocks, blockSize);
}

MemoryPool::~MemoryPool() {
    std::free(memoryPtr);
    delete swap;
}

MemoryBlock MemoryPool::getBlock(size_t size) {
	std::cout << "getBlock() with size = " << size << std::endl;
	size_t blockId = 0;
    void *ptr = privateAlloc();

	if(!ptr) {
		std::cout << "No free blocks in the pool! AAAAA!!!!!" << std::endl;
		// No free blocks in pool, try to use swap

		// Random block for tests!!!
		size_t blockIndex = rand() % numBlocks;  

		blockId = swap->Swap(blockIndex);
	}

    return MemoryBlock{ptr, blockId, blockSize, size, this};
}

void* MemoryPool::privateAlloc() {
    if (nextBlock) {
        char *block = nextBlock;
        nextBlock = *reinterpret_cast<char **>(nextBlock);
        return block;
    }

    // No free blocks
    return nullptr;
}

void MemoryPool::privateFree(void *ptr) {
	char* block = static_cast<char*>(ptr);
	*reinterpret_cast<char **>(block) = nextBlock;
	nextBlock = block;
}

size_t MemoryPool::blockIndexByAddress(void *ptr) {
    return (static_cast<char *>(ptr) - memoryPtr) / blockSize;
}

char* MemoryPool::blockAddressByIndex(size_t index) {
	return memoryPtr + index * blockSize;
}

void MemoryPool::lockBlock(void *ptr) {
	UNUSED(ptr);
//	blockMutex.at(blockIndexByAddress(ptr)).lock();
}

void MemoryPool::unlockBlock(void *ptr) {
	UNUSED(ptr);
//	blockMutex.at(blockIndexByAddress(ptr)).unlock();
}

void MemoryPool::freeBlock(void* ptr, size_t id) {
	if(id > 0) {
		// it's in swap, let's just mark it freed (in swapTable)
		size_t blockIndex = blockIndexByAddress(ptr);
		swap->MarkFreed(blockIndex, id);
	} else {
		// it's in ram
		// let's check if there are some swapped blocks for this blockIndex
		// - if yes then let's find the last one (max swap level) and move it to the ram level
		// - if there is not swap blocks then let's free this block for real (std::free)
	}
}
