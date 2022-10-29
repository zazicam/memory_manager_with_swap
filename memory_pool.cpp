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

void MemoryBlock::lock() {
    pool_->lockBlock(ptr_);

	size_t blockIndex = pool_->blockIndexByAddress(ptr_);
//	std::cout << "blockAddress: " << ptr_ << std::endl;
//	std::cout << "blockIndex: " << blockIndex << std::endl;

	pool_->swapMutex.lock();
	pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);
	pool_->diskSwap->Print();
	pool_->swapMutex.unlock();
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
    delete diskSwap;
    std::free(memoryPtr);
}

MemoryBlock MemoryPool::getBlock(size_t size) {
	std::lock_guard<std::mutex> poolGuard(poolMutex);
	swapMutex.lock();
	std::cout << "getBlock() with size = " << size << std::endl;

	size_t blockId = 1;
    void *ptr = privateAlloc();
	size_t blockIndex; 
	if(ptr) {
		blockIndex = blockIndexByAddress(ptr);
	} else {
		std::cout << "No free blocks in the pool! AAAAA!!!!!" << std::endl;
		// No free blocks in pool, try to use swap
		
		// Random block for tests!!!
		blockIndex = rand() % numBlocks;  
		ptr = blockAddressByIndex(blockIndex);
		
		blockId = diskSwap->Swap(blockIndex); // returns unique id for each new block
	}
	diskSwap->MarkBlockAllocated(blockIndex, blockId);
	swapMutex.unlock();

	std::cout << "getBlock() : blockIndex = " << blockIndex << std::endl;
//	std::cout << "getBlock() : blockAddress = " << ptr << std::endl;
//	std::cout << "getBlock() : finishing" << std::endl;
	diskSwap->Print();

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
//	UNUSED(ptr);
	blockMutex.at(blockIndexByAddress(ptr)).lock();
}

void MemoryPool::unlockBlock(void *ptr) {
//	UNUSED(ptr);
	blockMutex.at(blockIndexByAddress(ptr)).unlock();
}

void MemoryPool::freeBlock(void* ptr, size_t id) {
	UNUSED(ptr);
	UNUSED(id);
//	std::lock_guard<std::mutex> guard(poolMutex);
//
//	size_t blockIndex = blockIndexByAddress(ptr);
//	std::cout << "freeBlock() index: " << blockIndex << ", id: " << id << std::endl;
//	swapMutex.lock();
//	if(diskSwap->isBlockInSwap(blockIndex, id)) {
//		// it's in swap, let's just mark it freed (in swapTable)
//		diskSwap->MarkBlockFreed(blockIndex, id);
//	} else {
//		if(diskSwap->HasSwappedBlocks(blockIndex)) {
//			diskSwap->ReturnLastSwappedBlockIntoRam(blockIndex);
//		} else {
//			privateFree(ptr);	
//		}
//	}
//	swapMutex.unlock();
}
