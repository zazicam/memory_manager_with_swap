#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include <functional>

#include "swap.hpp"

namespace fs = std::filesystem;

//-------------------------------------------------------------------
// class SwapLevel
//-------------------------------------------------------------------
SwapLevel::SwapLevel(size_t level, size_t numBlocks, size_t blockSize)
	: level(level), 
	numBlocks(numBlocks),
	blockSize(blockSize),
	totalSize(numBlocks*blockSize),
	blockId(std::vector<uchar>(numBlocks, 0)) 
{
}

uchar& SwapLevel::at(size_t blockIndex) {
	assert(blockIndex < numBlocks);
	return blockId.at(blockIndex);
}

const uchar& SwapLevel::at(size_t blockIndex) const {
	assert(blockIndex < numBlocks);
	return blockId.at(blockIndex);
}

SwapLevel::~SwapLevel() {}

//-------------------------------------------------------------------
// class RamSwapLevel
//-------------------------------------------------------------------
RamSwapLevel::RamSwapLevel(size_t level, size_t numBlocks, size_t blockSize, void* poolAddress)
: SwapLevel(level, numBlocks, blockSize),
poolAddress(static_cast<char*>(poolAddress)) 
{
} 

void RamSwapLevel::WriteBlock(void* data, size_t blockIndex) {
	char* blockAddress = poolAddress + blockIndex * blockSize;
	std::memcpy(blockAddress, data, blockSize);
}

void RamSwapLevel::ReadBlock(void* data, size_t blockIndex) {
	char* blockAddress = poolAddress + blockIndex * blockSize;
	std::memcpy(data, blockAddress, blockSize);
}

RamSwapLevel::~RamSwapLevel() {}

//-------------------------------------------------------------------
// class DiskLevel
//-------------------------------------------------------------------
DiskSwapLevel::DiskSwapLevel(size_t level, size_t numBlocks, size_t blockSize)
	: SwapLevel(level, numBlocks, blockSize) 
{
	std::string filename = "swap";
	filepath = "./" + filename + "_" 
		+ std::to_string(numBlocks) + "x" 
		+ std::to_string(blockSize) + "_"
		+ "L" + std::to_string(level) + ".bin";

	// just to create a new file
	std::ofstream tmp(filepath, std::ios::binary);
	tmp.close();

	try {
		std::filesystem::resize_file(filepath, totalSize);
	} catch (const fs::filesystem_error &) {
		std::cerr << "Error: Swap() can't resize file to " << totalSize
			<< " bytes" << std::endl;
	}

	file = std::move(
		std::fstream(filepath, std::ios::binary | std::ios::in | std::ios::out)
	);

	if (!file) {
		std::cerr << "Error: Swap() can't create file " << filepath
			<< " for writing" << std::endl;
		exit(1);
	}
}

void DiskSwapLevel::WriteBlock(void* data, size_t blockIndex) {
	assert(blockIndex < numBlocks);
	size_t pos = blockIndex*blockSize;
	file.seekp(pos);
	file.write(reinterpret_cast<char *>(data), blockSize);
}

void DiskSwapLevel::ReadBlock(void* data, size_t blockIndex) {
	assert(blockIndex < numBlocks);
	size_t pos = blockIndex*blockSize;
	file.seekg(pos);
	file.read(reinterpret_cast<char *>(data), blockSize);
}

DiskSwapLevel::~DiskSwapLevel() {
	file.close();	
	fs::remove(filepath);
}

//-------------------------------------------------------------------
// class DiskSwap
//-------------------------------------------------------------------
DiskSwap::DiskSwap(void* poolAddress, size_t numBlocks, size_t blockSize) 
	: numBlocks(numBlocks),
	blockSize(blockSize),
	numLevels(2),
	poolAddress(static_cast<char*>(poolAddress)),
	swapId(std::vector<uchar>(numBlocks, 2)), // 0 - empty, 1 - ram level, 2..255 - swap levels
	swapTable({
		new RamSwapLevel(0, numBlocks, blockSize, poolAddress),
		new DiskSwapLevel(1, numBlocks, blockSize)
	}) {}
	
void DiskSwap::UpdateRamBlockId(size_t blockId, size_t id) {
	swapTable[RAM]->at(blockId) = id;
}

void DiskSwap::Swap(size_t blockIndex, size_t swapLevel) {
	assert(blockIndex < numBlocks);
	std::cout << "swap called for level " << swapLevel << std::endl;
	std::unique_ptr<char> tmpBlock{new char[blockSize]};
	swapTable[swapLevel]->ReadBlock(tmpBlock.get(), blockIndex);
	char* blockAddress = poolAddress + blockIndex * blockSize;
	swapTable[swapLevel]->WriteBlock(blockAddress, blockIndex);
	swapTable[RAM]->WriteBlock(tmpBlock.get(), blockIndex);
	
	std::swap(swapTable[RAM]->at(blockIndex), swapTable[swapLevel]->at(blockIndex));
}

size_t DiskSwap::FindEmptyLevel(size_t blockIndex) {
	assert(blockIndex < numBlocks);
	size_t swapLevel = 0;
	for(size_t level=1;level<numLevels;++level) {
		// id == 0 means this level is empty:
		if(swapTable[level]->at(blockIndex) == 0) {
			swapLevel = level;
			break;
		}
	}
	return swapLevel;
}

size_t DiskSwap::Swap(size_t blockIndex) {
	// just check if pool block is empty (no need to swap in that case)
	if(swapTable[RAM] == 0) {
		std::cerr << "Swap::MoveToSwap(" << blockIndex << ")."
		          << "No need to swap: ram block with this index is empty!" << std::endl;
		return 1; // id for ram level
	}

	size_t swapLevel = FindEmptyLevel(blockIndex);
	if(swapLevel == 0) {
		// empty level not found, let's create it!
		swapTable.push_back( new DiskSwapLevel{numLevels, numBlocks, blockSize} );
		swapLevel = numLevels;
		++numLevels;
	}

	Swap(blockIndex, swapLevel);
	return swapId.at(blockIndex)++;
}

void DiskSwap::Print() {
	std::cout << "       ";
	for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
		std::cout << blockIndex << " ";
	}
	std::cout << "(blocks)" << std::endl;
	std::cout << "      ";
	for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
		std::cout << "==";
	}
	std::cout << std::endl;

	for(size_t level = 0; level < numLevels; ++level) {
		std::cout << "lv " << level << " | "; 
		for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
			size_t value = static_cast<size_t>(swapTable[level]->at(blockIndex)); 
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

DiskSwap::~DiskSwap() {
	for(SwapLevel* swapLevel : swapTable) {
		delete swapLevel;
	}
}


