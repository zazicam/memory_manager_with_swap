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
MemoryBlock::MemoryBlock(void *ptr, size_t id, size_t size, MemoryPool *pool)
    : ptr(ptr), id(id), size(size), pool(pool) {}

void MemoryBlock::lock() {
	std::cout << "MemoryBlock::lock() started" << std::endl;
	std::cout << "MemoryBlock::lock() ptr = " << ptr << std::endl;
	std::cout << "MemoryBlock::lock() id = " << id << std::endl;
	std::cout << "MemoryBlock::lock() size = " << size << std::endl;

	std::cout << "MemoryBlock::lock() finished" << std::endl;
}

void MemoryBlock::unlock() {
//    pool->unlockBlock(ptr);
}

void MemoryBlock::free() {
//    pool->privateFree(ptr, id);
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

    // create disk swap
    swap = new DiskSwap(memoryPtr, numBlocks, blockSize);
}

MemoryPool::~MemoryPool() {
    std::free(memoryPtr);
    delete swap;
}

MemoryBlock MemoryPool::getBlock() {
	std::cout << "MemoryPool::getBlock() started" << std::endl;
    void *ptr = privateAlloc();

	// TODO
	size_t blockId = 0;

    return MemoryBlock{ptr, blockId, blockSize, this};
}

void* MemoryPool::privateAlloc() {
	std::cout << "MemoryPool::privateAlloc() started" << std::endl;

    if (nextBlock) {
        char *block = nextBlock;
        nextBlock = *reinterpret_cast<char **>(nextBlock);
        return block;
    }

    // No free blocks -> try to use swap file

	std::cout << "MemoryPool::privateAlloc() finished" << std::endl;
    return nullptr; // TODO!!!
}

void MemoryPool::privateFree(void *ptr, size_t id) {
	std::cout << "MemoryPool::privateFree() started" << std::endl;
	UNUSED(ptr);
	UNUSED(id);
	std::cout << "MemoryPool::privateFree() finished" << std::endl;
}

size_t MemoryPool::blockIndexByAddress(void *ptr) {
    return (static_cast<char *>(ptr) - memoryPtr) / blockSize;
}

char* MemoryPool::blockAddressByIndex(size_t index) {
	return memoryPtr + index * blockSize;
}

void MemoryPool::lockBlock(void *ptr) {
	std::cout << "MemoryPool::lockBlock() started" << std::endl;
	UNUSED(ptr);	
	std::cout << "MemoryPool::lockBlock() finished" << std::endl;
}

void MemoryPool::unlockBlock(void *ptr) {
	std::cout << "MemoryPool::unlockBlock() started" << std::endl;
	UNUSED(ptr);	
	std::cout << "MemoryPool::unlockBlock() finished" << std::endl;
}
