#include <iostream>
#include <filesystem>
#include <cassert>
#include <cstddef>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>

using namespace std::chrono_literals;

#include "memory_pool.hpp"

#define UNUSED(var) ((void)var)    // for debug

// class BlockAddress
// --------------------------------------------------------
BlockAddress::BlockAddress()
	: inMemory(true), memoryAddress(nullptr) {}

BlockAddress::BlockAddress(void* memoryAddress)
	: inMemory(true), memoryAddress(memoryAddress) {}
	
void BlockAddress::moveToFile(size_t fileAddress) {
	inMemory = false;
	this->fileAddress = fileAddress;
}

void BlockAddress::moveToMemory() {
	inMemory = true;
}

bool BlockAddress::isInMemory() {
	return inMemory;
}

bool BlockAddress::isInSwap() {
    return !inMemory;
}

void* BlockAddress::getMemoryAddress() {
	return memoryAddress;
}

size_t BlockAddress::getFileAddress() {
	return fileAddress;
}


// class MemoryBlock
// --------------------------------------------------------
MemoryBlock::MemoryBlock(size_t id, void* ptr, size_t size, MemoryPool* pool)
: id(id), ptr(ptr), size(size), pool(pool) {}

void MemoryBlock::lock() {

//	std::cout << "lock()" << std::endl;

	pool->poolMutex.lock();
	BlockAddress blockAddr = pool->blockAddress.at(id);
	if(blockAddr.isInSwap()) {
		size_t filePos = blockAddr.getFileAddress();
		void* memAddr = blockAddr.getMemoryAddress();
		
		// wait until origin block is unlocked
		bool done = false;
		while(!done) {
			pool->blockStateMutex.lock();
			
			size_t blockIndex = pool->blockIndexByAddress(memAddr);
//			std::cout << "blockIndex: " << blockIndex << std::endl;
			MemoryBlockState blockState = pool->blockState.at(blockIndex);
			done = !blockState.locked;

			pool->blockStateMutex.unlock();

			if(!done)
				std::this_thread::sleep_for(10ms);	
		}

		pool->swap->swap(filePos, memAddr, size); 
	}
	pool->poolMutex.unlock();

	std::lock_guard<std::mutex> guard(pool->blockStateMutex);
	pool->lockBlock(ptr);
}

void MemoryBlock::unlock() {
	std::lock_guard<std::mutex> guard(pool->blockStateMutex);
	pool->unlockBlock(ptr);
}

void MemoryBlock::free() {
	std::lock_guard<std::mutex> guard(pool->poolMutex);
	pool->privateFree(ptr, id);
}

// class MemoryPool
// --------------------------------------------------------
size_t MemoryPool::blocksCounter;

MemoryPool::MemoryPool(size_t numBlocks, size_t blockSize) {
	assert(numBlocks > 0);
	assert(blockSize >= sizeof(char *)); // empty block contains a pointer to the next empty block

	this->numBlocks = numBlocks;
	this->blockSize = blockSize;
	this->totalSize = numBlocks * blockSize;

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
	
	// locked blocks info
	blockState.resize(numBlocks, {0, false});
	unlockedBlocksCounter = 0;

	// create swap
	swap = new Swap(1024, blockSize);
}

MemoryPool::~MemoryPool() { 
	std::free(memoryPtr); 
	std::free(swap); 
}

MemoryBlock MemoryPool::getBlock() {
//	std::cout << "getBlock()" << std::endl;
	std::lock_guard<std::mutex> guard(poolMutex);

	void* ptr = privateAlloc();
	size_t blockId = ++blocksCounter;
	
	blockStateMutex.lock();
	
	size_t blockIndex = blockIndexByAddress(ptr);
//	std::cout << "blockIndex: " << blockIndex << std::endl;

	blockState.at(blockIndex).id = blockId;  // WRONG INDEX HERE!!!
	
	++unlockedBlocksCounter;
	blockStateMutex.unlock();

	blockAddress[blockId] = BlockAddress(ptr);

//	std::cout << "getBlock() almost end" << std::endl;

	return MemoryBlock { blockId, ptr, blockSize, this };
}

void* MemoryPool::privateAlloc() {
	if (nextBlock) {
		char *block = nextBlock;
		nextBlock = *reinterpret_cast<char **>(nextBlock);
		return block;
	}

	// No free blocks -> try to use swap file
	blockStateMutex.lock();
//	std::cout << "unlocked blocks count: " << unlockedBlocksCounter << std::endl;

	if(unlockedBlocksCounter == 0) {
		std::cerr << "MemoryPool::privateAlloc() no free blocks and all allocated blocks are in use. "
				  << "No possible to use swap in that case! Just exit." << std::endl;
		exit(2);
	}

	auto unusedBlockIt = std::find_if(begin(blockState), end(blockState), [](const MemoryBlockState& block){
		return !block.locked;
	});
//	UNUSED(it);	
	
	size_t fileAddress = swap->alloc();
	size_t blockId = unusedBlockIt->id;

//	std::cout << "privateAlloc()" << std::endl;

	void* blockMemoryAddress = blockAddress.at(blockId).getMemoryAddress();
	swap->write(fileAddress, blockMemoryAddress, blockSize);
	blockAddress.at(blockId).moveToFile(fileAddress);
	
	blockStateMutex.unlock();
	
	return blockMemoryAddress;

//	return new char[blockSize]; // DEBUG ONLY!!!
}

void MemoryPool::privateFree(void *ptr, size_t id) {
	BlockAddress block = blockAddress.at(id);
	if(block.isInMemory()) {
		char *block = static_cast<char *>(ptr);
		*reinterpret_cast<char **>(block) = nextBlock;
		nextBlock = block;
	} else {
		// if it's in the swap file
		swap->free(block.getFileAddress());
	}
}

size_t MemoryPool::blockIndexByAddress(void* ptr) {
	return (static_cast<char*>(ptr) - memoryPtr) / blockSize;
}

void MemoryPool::lockBlock(void* ptr) {
	blockState.at(blockIndexByAddress(ptr)).locked = true;
	--unlockedBlocksCounter;
}

void MemoryPool::unlockBlock(void* ptr) {
	blockState.at(blockIndexByAddress(ptr)).locked = false;
	++unlockedBlocksCounter;
}
