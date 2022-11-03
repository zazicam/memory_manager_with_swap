#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

#include "logger.hpp"
#include "memory_pool.hpp"

#define UNUSED(var) ((void)var) // for debug

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
MemoryBlock::MemoryBlock(void *ptr, size_t id, size_t capacity, size_t size,
                         bool locked, MemoryPool *pool)
    : ptr_(ptr), id_(id), capacity_(capacity), size_(size)
	, locked_(locked), pool_(pool), moved_(false) {}

MemoryBlock::MemoryBlock(MemoryBlock&& other) 
    : ptr_(other.ptr_), id_(other.id_), capacity_(other.capacity_), size_(other.size_)
	, locked_(other.locked_), pool_(other.pool_), moved_(false) 
	{
		other.ptr_ = nullptr;
		other.moved_ = true;
	}

MemoryBlock& MemoryBlock::operator=(MemoryBlock&& other) {
	if(this == &other)
		return *this;

	other.ptr_ = nullptr;
	other.moved_ = true;
	return *this;
}

size_t MemoryBlock::size() const { return size_; }

size_t MemoryBlock::capacity() const { return capacity_; }

void MemoryBlock::checkScopeError() const {
	if(moved_) {
		LOG_BEGIN
		logger << "ERROR: you can't use MemoryBlock in old scope after it was moved to a new scope!" << std::endl;
		LOG_END
		exit(1);
	}
}

void MemoryBlock::lock() {
	checkScopeError();	
	if(locked_ == false) {
		size_t blockIndex = pool_->blockIndexByAddress(ptr_);
		pool_->lockBlock(ptr_);
		pool_->swapMutex.lock();
		pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);
		pool_->swapMutex.unlock();
		locked_ = true;
	} else {
		LOG_BEGIN
		logger << "Info: lock() called for a locked block" << std::endl; 
		LOG_END
	}
}

void MemoryBlock::unlock() { 
	checkScopeError();	
	if(locked_ == true) {
		pool_->unlockBlock(ptr_); 
		locked_ = false;
	} else {
		LOG_BEGIN
		logger << "Info: unlock() called for an unlocked block" << std::endl; 
		LOG_END
	}
}

bool MemoryBlock::isLocked() const { 
	checkScopeError();	
	return locked_; 
}

void MemoryBlock::free() { 
	checkScopeError();	
	pool_->freeBlock(ptr_, id_); 
}

void MemoryBlock::debugPrint() const {
	checkScopeError();	
    LOG_BEGIN
    logger << "Block Info: " << std::endl;
    logger << "ptr: " << ptr_
           << ", blockIndex: " << pool_->blockIndexByAddress(ptr_) << std::endl;
    logger << "id: " << id_ << ", size: " << size_ << std::endl;
    logger << std::endl;
    LOG_END
}
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

    size_t blockId = 1;
    void *ptr = privateAlloc();

    size_t blockIndex;
    if (ptr) {
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

	 	// returns unique id for each new block
        blockId = diskSwap->Swap(blockIndex);

        diskSwap->MarkBlockAllocated(blockIndex, blockId);
        swapMutex.unlock();
        unlockBlock(ptr);
    }

    return {ptr, blockId, blockSize, size, false, this};
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
}

void MemoryPool::unlockBlock(void *ptr) {
    std::unique_lock<std::mutex> ul(blockMutex);
	blockIsLocked.at(blockIndexByAddress(ptr)) = 0;
	conditionVariable.notify_one();
    ul.unlock();
}

void MemoryPool::freeBlock(void *ptr, size_t id) {
    std::lock_guard<std::mutex> poolGuard(poolMutex);
    lockBlock(ptr);
    swapMutex.lock();

    size_t blockIndex = blockIndexByAddress(ptr);

    if (diskSwap->isBlockInSwap(blockIndex, id)) {
        // it's in swap, let's just mark it freed (in swapTable)
        diskSwap->MarkBlockFreed(blockIndex, id);
    } else {
        if (diskSwap->HasSwappedBlocks(blockIndex)) {
            diskSwap->ReturnLastSwappedBlockIntoRam(blockIndex);
        } else {
            privateFree(ptr);
            diskSwap->MarkBlockFreed(blockIndex, id);
        }
    }

    swapMutex.unlock();
    unlockBlock(ptr);
}
