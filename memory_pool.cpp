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
#include "logger.hpp"

#define UNUSED(var) ((void)var) // for debug

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
MemoryBlock::MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size, MemoryPool *pool)
    : ptr_(ptr), id_(id), capacity_(capacity), size_(size), pool_(pool) {}

size_t MemoryBlock::size() const {
    return size_;
}

size_t MemoryBlock::capacity() const {
	return capacity_;
}

void MemoryBlock::lock() {

	size_t blockIndex = pool_->blockIndexByAddress(ptr_);
    pool_->lockBlock(ptr_);
	pool_->swapMutex.lock();
	pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);
	pool_->swapMutex.unlock();
}

void MemoryBlock::unlock() {
    pool_->unlockBlock(ptr_);
}

void MemoryBlock::free() {
    pool_->freeBlock(ptr_, id_);
}

void MemoryBlock::debugPrint() const {
	LOG_BEGIN
	logger << "Block Info: " << std::endl;
	logger << "ptr: " << ptr_ << ", blockIndex: " << pool_->blockIndexByAddress(ptr_) << std::endl;
	logger << "id: " << id_ << ", size: " << size_ << std::endl;
	logger << std::endl;
	LOG_END
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
	*reinterpret_cast<char**>(memoryPtr+(numBlocks-1)*blockSize) = nullptr;

    nextBlock = memoryPtr;

	// locker for each block
	blockIsLocked.resize(numBlocks);

    // create disk swap
    diskSwap = new DiskSwap(memoryPtr, numBlocks, blockSize);
	
	std::cout << "Created memory pool " << numBlocks << " blocks x " << blockSize << " bytes" << std::endl;
}

MemoryPool::~MemoryPool() {
    delete diskSwap;
    std::free(memoryPtr);
}

MemoryBlock MemoryPool::getBlock(size_t size) {
	std::lock_guard<std::mutex> poolGuard(poolMutex);

	size_t blockId = 1;
    void *ptr = privateAlloc();

	size_t blockIndex; 
	if(ptr) {
		blockIndex = blockIndexByAddress(ptr);
		swapMutex.lock();
		diskSwap->MarkBlockAllocated(blockIndex, blockId);
		swapMutex.unlock();

	} else {
		// No free blocks in pool, try to use swap
		
		// Random block for tests!!!
		blockIndex = rand() % numBlocks;  
		ptr = blockAddressByIndex(blockIndex);

		lockBlock(ptr);
		swapMutex.lock();
		blockId = diskSwap->Swap(blockIndex); // returns unique id for each new block
		diskSwap->MarkBlockAllocated(blockIndex, blockId);
		swapMutex.unlock();
		unlockBlock(ptr);
    }

    return MemoryBlock {ptr, blockId, blockSize, size, this};
}

void* MemoryPool::privateAlloc() {
    if (nextBlock) {
        char* block = nextBlock;
        nextBlock = *reinterpret_cast<char **>(nextBlock);
        return block;
    }

    // No free blocks
    return nullptr;
}

void MemoryPool::privateFree(void *ptr) {
	assert(ptr != nullptr);
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
	size_t blockIndex = blockIndexByAddress(ptr);
	std::unique_lock<std::mutex> ul(blockMutex);
	conditionVariable.wait(ul, [=]() { 
		return blockIsLocked.at(blockIndex) == false; 
	});
	blockIsLocked.at(blockIndexByAddress(ptr)) = true;
	ul.unlock();
}

void MemoryPool::unlockBlock(void *ptr) {
	std::unique_lock<std::mutex> ul(blockMutex);
	if(blockIsLocked.at(blockIndexByAddress(ptr)) == true) {
		blockIsLocked.at(blockIndexByAddress(ptr)) = false;
		conditionVariable.notify_one();
	}
	ul.unlock();
}

void MemoryPool::freeBlock(void* ptr, size_t id) {
	std::lock_guard<std::mutex> poolGuard(poolMutex);
	lockBlock(ptr);
	swapMutex.lock();

	size_t blockIndex = blockIndexByAddress(ptr);

	if(diskSwap->isBlockInSwap(blockIndex, id)) {
		// it's in swap, let's just mark it freed (in swapTable)
		diskSwap->MarkBlockFreed(blockIndex, id);
	} else {
		if(diskSwap->HasSwappedBlocks(blockIndex)) {
			diskSwap->ReturnLastSwappedBlockIntoRam(blockIndex);
		} else {
			privateFree(ptr);	
			diskSwap->MarkBlockFreed(blockIndex, id);
		}
	}

	swapMutex.unlock();
	unlockBlock(ptr);
}
