#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include <functional>

// debug only
#include <iomanip>

#include "swap.hpp"
#include "logger.hpp"

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
	swapId(std::vector<uchar>(numBlocks, 2)), // 0 - empty, 1..255 - block ids 
	swapTable({
		new RamSwapLevel(0, numBlocks, blockSize, poolAddress),
		new DiskSwapLevel(1, numBlocks, blockSize)
	}) {}

void DiskSwap::LoadBlockIntoRam(size_t blockIndex, size_t id) {
	if(id == swapTable.at(RAM)->at(blockIndex)) {
		LOG_BEGIN
		logger << "Block is already in RAM" << std::endl;
		LOG_END
	} else {
		size_t swapLevel = FindSwapLevel(blockIndex, id);	
		Swap(blockIndex, swapLevel);
		LOG_BEGIN
		logger << "Block was loaded into RAM" << std::endl;
		LOG_END
	}
}

size_t DiskSwap::FindSwapLevel(size_t blockIndex, size_t id) {
	assert(blockIndex < numBlocks);
	size_t swapLevel = 0;
	for(size_t level=1;level<numLevels;++level) {
		// id == 0 means this level is empty:
		if(id == swapTable.at(level)->at(blockIndex)) {
			swapLevel = level;
			break;
		}
	}
	return swapLevel;
}

size_t DiskSwap::FindEmptyLevel(size_t blockIndex) {
	assert(blockIndex < numBlocks);
	size_t swapLevel = 0;
	for(size_t level=1;level<numLevels;++level) {
		// id == 0 means this level is empty:
		if(swapTable.at(level)->at(blockIndex) == 0) {
			swapLevel = level;
			break;
		}
	}
	return swapLevel;
}

size_t DiskSwap::FindLastLevel(size_t blockIndex) {
	assert(blockIndex < numBlocks);
	if(swapTable.at(0)->at(blockIndex) == 0)
		return 0;

	size_t lastLevel = numLevels-1;
	for(size_t level=1;level<numLevels;++level) {
		if(swapTable.at(level)->at(blockIndex) == 0) {
			lastLevel = level - 1;
			break;
		}
	}
	return lastLevel;
}
	
void DiskSwap::MarkBlockAllocated(size_t blockIndex, size_t id) {
	swapTable.at(RAM)->at(blockIndex) = id;
}

void DiskSwap::MarkBlockFreed(size_t blockIndex, size_t id) {
	// we are to avoid holes (zeroes) inside the swap table
	// so if it's the last level we can just put there 0
	// else we'd better put there value from the last level
	size_t goalLevel = FindSwapLevel(blockIndex, id);
	size_t lastLevel = FindLastLevel(blockIndex);

	if(lastLevel < 1) {
		debugPrint();
		LOG_BEGIN
		logger << "blockIndex: " << blockIndex << ", id: " << id << std::endl;
		logger << "lastLevel: " << lastLevel << std::endl;
		LOG_END
	}
	
	assert(lastLevel >= 1);

	if(goalLevel == lastLevel)
		swapTable.at(goalLevel)->at(blockIndex) = 0; 
	else
		swapTable.at(goalLevel)->at(blockIndex) = swapTable.at(lastLevel)->at(blockIndex);
}

void DiskSwap::Swap(size_t blockIndex, size_t swapLevel) {
	assert(blockIndex < numBlocks);
	LOG_BEGIN
	logger << "swap called for level " << swapLevel << std::endl;
	LOG_END
	std::unique_ptr<char> tmpBlock{new char[blockSize]};
//	std::cout << "read tmp " << std::endl;
	swapTable.at(swapLevel)->ReadBlock(tmpBlock.get(), blockIndex);
	char* blockAddress = poolAddress + blockIndex * blockSize;
//	std::cout << "write to file " << std::endl;
	swapTable.at(swapLevel)->WriteBlock(blockAddress, blockIndex);
//	std::cout << "write to ram " << std::endl;
	swapTable.at(RAM)->WriteBlock(tmpBlock.get(), blockIndex);
//	std::cout << "swap in swapTable " << std::endl;
	std::swap(swapTable.at(RAM)->at(blockIndex), swapTable.at(swapLevel)->at(blockIndex));
//	std::cout << "swap finished " << std::endl;

}

bool DiskSwap::isBlockInRam(size_t blockIndex, const size_t id) {
	return id == swapTable.at(RAM)->at(blockIndex); 
}

bool DiskSwap::isBlockInSwap(size_t blockIndex, size_t id) {
	return id != swapTable.at(RAM)->at(blockIndex); 
}

bool DiskSwap::HasSwappedBlocks(size_t blockIndex) {
	// it has if there is not 0 at the first swap level
	return swapTable.at(1)->at(blockIndex) != 0;
}

void DiskSwap::ReturnLastSwappedBlockIntoRam(size_t blockIndex) {
	size_t lastSwapLevel = FindEmptyLevel(blockIndex) - 1;
	char* ramBlockAddress = poolAddress + blockIndex * blockSize;
	swapTable.at(lastSwapLevel)->ReadBlock(ramBlockAddress, blockIndex);
	MarkBlockFreed(blockIndex, lastSwapLevel);
}

size_t DiskSwap::Swap(size_t blockIndex) {
	// just check if pool block is empty (no need to swap in that case)
	if(swapTable.at(RAM) == 0) {
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
	LOG_BEGIN
	logger << "DiskSwap::Swap() swapLevel: " << swapLevel << std::endl;	
	LOG_END

	Swap(blockIndex, swapLevel);

	assert(swapId.at(blockIndex) < 255);

	return swapId.at(blockIndex)++;
}

uchar hash(uchar* data, size_t size) {
	uchar res = 0;
	for(size_t i = 0; i<size; ++i) {
		res ^= data[i];
	}
	return res;
}

void DiskSwap::debugPrint() {
	LOG_BEGIN
	logger << "       ";
	for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
		logger << blockIndex << " ";
	}
	logger << "(blocks)" << std::endl;
	logger << "      ";
	for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
		logger << "==";
	}
	logger << std::endl;

	for(size_t level = 0; level < numLevels; ++level) {
		logger << "lv " << level << " | "; 
		for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
			size_t value = static_cast<size_t>(swapTable.at(level)->at(blockIndex)); 
			logger << value << " ";
		}
		logger << std::endl;
	}
	logger << std::endl;

	logger << "data hashes: " << std::endl;
	std::unique_ptr<uchar> tmpBlock{new uchar[blockSize]};

	for(size_t level = 0; level < numLevels; ++level) {
		logger << "lv " << level << " | "; 
		for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
			size_t value = static_cast<size_t>(swapTable.at(level)->at(blockIndex)); 
			if(value !=0) {
				swapTable.at(level)->ReadBlock(tmpBlock.get(), blockIndex);
				size_t hashValue = static_cast<size_t>(hash(tmpBlock.get(), blockSize)); 
				logger << std::setw(4) << hashValue << " ";
			} else {
				logger << std::setw(4) << "-" << " ";
			}

		}
		logger << std::endl;
	}
	logger << std::endl;

	LOG_END
}

DiskSwap::~DiskSwap() {
	for(SwapLevel* swapLevel : swapTable) {
		delete swapLevel;
	}
}


