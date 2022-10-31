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

size_t MemoryBlock::size() {
    return size_;
}

size_t MemoryBlock::capacity() {
	return capacity_;
}

void MemoryBlock::lock() {

	size_t blockIndex = pool_->blockIndexByAddress(ptr_);
//	std::cout << "blockAddress: " << ptr_ << std::endl;
//	std::cout << "blockIndex: " << blockIndex << std::endl;

    pool_->lockBlock(ptr_);
	pool_->swapMutex.lock();
	pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);
//	pool_->diskSwap->debugPrint();
	pool_->swapMutex.unlock();
}

void MemoryBlock::unlock() {
    pool_->unlockBlock(ptr_);
}

void MemoryBlock::free() {
    pool_->freeBlock(ptr_, id_);
}

void MemoryBlock::debugPrint() {
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
}

MemoryPool::~MemoryPool() {
    delete diskSwap;
    std::free(memoryPtr);
}

MemoryBlock MemoryPool::getBlock(size_t size) {
	std::lock_guard<std::mutex> poolGuard(poolMutex);

//	LOG_BEGIN
//	logger << "getBlock() with size = " << size << std::endl;
//	//diskSwap->debugPrint();
//	logger << "OK" << std::endl;
//	LOG_END

	size_t blockId = 1;
    void *ptr = privateAlloc();

//	std::cout << "After privateAlloc()" << std::endl;
//	std::cout.flush();

	size_t blockIndex; 
	if(ptr) {
		blockIndex = blockIndexByAddress(ptr);
		LOG_BEGIN
		logger << "pool allocated block: " << ptr 
		<< ", blockIndex: " << blockIndex << std::endl;
		LOG_END

		swapMutex.lock();
		diskSwap->MarkBlockAllocated(blockIndex, blockId);
		swapMutex.unlock();

	} else {
		LOG_BEGIN
		logger << "No free blocks in the pool! AAAAA!!!!!" << std::endl;
		LOG_END
		// No free blocks in pool, try to use swap
		
		// Random block for tests!!!
		blockIndex = rand() % numBlocks;  
		ptr = blockAddressByIndex(blockIndex);

		std::cout << "block lock!" << std::endl;
		std::cout.flush();
		lockBlock(ptr);
		std::cout << "blockLocked" << std::endl;
		std::cout.flush();

		swapMutex.lock();
		blockId = diskSwap->Swap(blockIndex); // returns unique id for each new block
		diskSwap->MarkBlockAllocated(blockIndex, blockId);
		swapMutex.unlock();
		unlockBlock(ptr);
    }

//	LOG_BEGIN
//	logger << "getBlock() : blockIndex = " << blockIndex << std::endl;
//	std::cout << "getBlock() : blockAddress = " << ptr << std::endl;
//	//diskSwap->debugPrint();
//	LOG_END

    return MemoryBlock {ptr, blockId, blockSize, size, this};
}

void* MemoryPool::privateAlloc() {
	std::cout << "privateAlloc() called, nextBlock: " << (void*)nextBlock << std::endl;
	std::cout.flush();

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

	std::cout << "privateFree(): next = " << (void*)nextBlock << std::endl;
	std::cout.flush();
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
	cv.wait(ul, [=]() { return blockIsLocked.at(blockIndex) == false; } );
	blockIsLocked.at(blockIndexByAddress(ptr)) = true;
	ul.unlock();
}

void MemoryPool::unlockBlock(void *ptr) {
	std::unique_lock<std::mutex> ul(blockMutex);
	blockIsLocked.at(blockIndexByAddress(ptr)) = false;
	cv.notify_one();
	ul.unlock();
}

void MemoryPool::freeBlock(void* ptr, size_t id) {
	std::lock_guard<std::mutex> poolGuard(poolMutex);
	lockBlock(ptr);
	swapMutex.lock();

	size_t blockIndex = blockIndexByAddress(ptr);
	LOG_BEGIN
	logger << "freeBlock() index: " << blockIndex << ", id: " << id << std::endl;
	logger << "Before: " << std::endl;
	LOG_END
//	diskSwap->debugPrint();

	if(diskSwap->isBlockInSwap(blockIndex, id)) {
		// it's in swap, let's just mark it freed (in swapTable)
		diskSwap->MarkBlockFreed(blockIndex, id);
	} else {
		if(diskSwap->HasSwappedBlocks(blockIndex)) {
			diskSwap->ReturnLastSwappedBlockIntoRam(blockIndex);
		} else {
			logger << "private free: blockIndex: " << blockIndex
			<< ", id: " << id << std::endl;
			privateFree(ptr);	
			diskSwap->MarkBlockFreed(blockIndex, id);
		}
	}
	logger << "After: " << std::endl;
//	diskSwap->debugPrint();

	swapMutex.unlock();
	unlockBlock(ptr);
	poolMutex.unlock();
	
}
