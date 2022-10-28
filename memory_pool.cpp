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
MemoryBlock::MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size, MemoryPool *pool)
    : ptr_(ptr), id_(id), capacity_(capacity), size_(size), pool_(pool) {}

size_t MemoryBlock::size() {
    return size_;
}

size_t MemoryBlock::capacity() {
	return capacity_;
}

void MemoryBlock::loadIntoRam() {
	size_t blockIndex = pool_->blockIndexByAddress(ptr_);
	pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);	
}

void MemoryBlock::lock() {
	size_t blockIndex = pool_->blockIndexByAddress(ptr_);
	std::cout << "blockAddress: " << ptr_ << std::endl;
	std::cout << "blockIndex: " << blockIndex << std::endl;

	pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);

//    pool_->lockBlock(ptr_);
}

void MemoryBlock::unlock() {
    pool_->unlockBlock(ptr_);
}

void MemoryBlock::free() {
    pool_->freeBlock(ptr_, id_);
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
    diskSwap = new DiskSwap(memoryPtr, numBlocks, blockSize);
}

MemoryPool::~MemoryPool() {
    std::free(memoryPtr);
    delete diskSwap;
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
		ptr = blockAddressByIndex(blockIndex);
		
		std::cout << "getBlock() : blockIndex = " << blockIndex << std::endl;
		std::cout << "getBlock() : blockAddress = " << ptr << std::endl;

		blockId = diskSwap->Swap(blockIndex);
	}

	std::cout << "getBlock() : finishing" << std::endl;
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
		diskSwap->MarkBlockFreed(blockIndex, id);
	} else {
		// it's in ram
		// let's check if there are some swapped blocks for this blockIndex
		// - if yes then let's find the last one (max swap level) and move it to the ram level
		// - if there is not swap blocks then let's free this block for real (std::free)
	}
}
