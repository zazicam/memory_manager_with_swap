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
	mutex.lock();
	char* blockAddress = poolAddress + blockIndex * blockSize;
	std::memcpy(blockAddress, data, blockSize);
	mutex.unlock();
}

void RamSwapLevel::ReadBlock(void* data, size_t blockIndex) {
	mutex.lock();
	char* blockAddress = poolAddress + blockIndex * blockSize;
	std::memcpy(data, blockAddress, blockSize);
	mutex.unlock();
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
	mutex.lock();
	size_t pos = blockIndex*blockSize;
	file.seekp(pos);
	file.write(reinterpret_cast<char *>(data), blockSize);
	logger << "write to file: level = " << level  
	<< ", blockIndex: " << blockIndex << ", value: " << (size_t)((uchar*)data)[0] << std::endl;
	mutex.unlock();
}

void DiskSwapLevel::ReadBlock(void* data, size_t blockIndex) {
	assert(blockIndex < numBlocks);
	mutex.lock();
	size_t pos = blockIndex*blockSize;
	file.seekg(pos);
	file.read(reinterpret_cast<char *>(data), blockSize);
	logger << "read from file: level = " << level  
	<< ", blockIndex: " << blockIndex << ", value: " << (size_t)((uchar*)data)[0] << std::endl;
	mutex.unlock();
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
	logger << "loadBlockIntoRam" << std::endl;
	logger.flush();

	if(id == swapTable.at(RAM)->at(blockIndex)) {
		LOG_BEGIN
		logger << "Block is already in RAM blockIndex: " << blockIndex 
			   << ", blockId: " << id << std::endl;
		LOG_END
	} else {
		size_t swapLevel = FindSwapLevel(blockIndex, id);	
		Swap(blockIndex, swapLevel);
		LOG_BEGIN
		logger << "Block was loaded into RAM" << std::endl;
        logger << "blockIndex: " << blockIndex << ", swapLevel: " << swapLevel 
		       << "id: " << id << std::endl;
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
	if(numLevels == 1)
		return 0;

	size_t level = numLevels-1;
	while(swapTable.at(level)->at(blockIndex) == 0)
		--level;

	LOG_BEGIN
	logger << "FindLastLevel(): res = " << level << std::endl;
	LOG_END
	return level;
}
	
void DiskSwap::MarkBlockAllocated(size_t blockIndex, size_t id) {
	logger << "MarkBlockAllocated" << std::endl;
	logger.flush();
	swapTable.at(RAM)->at(blockIndex) = id;
}

void DiskSwap::MarkBlockFreed(size_t blockIndex, size_t id) {
	size_t goalLevel = FindSwapLevel(blockIndex, id);
	swapTable.at(goalLevel)->at(blockIndex) = 0; 
	logger << "MarkBlockFreed(): blockIndex: " << blockIndex  
	<< ", id: " << id << ", goal level: " << goalLevel<< std::endl;

//		debugPrint();
//		LOG_BEGIN
//		logger << "blockIndex: " << blockIndex << ", id: " << id << std::endl;
//		logger << "goal Level: " << goalLevel << std::endl;
//		LOG_END
	

}

void DiskSwap::Swap(size_t blockIndex, size_t swapLevel) {
	assert(blockIndex < numBlocks);
	
	LOG_BEGIN
	logger << "swap called for level " << swapLevel << ", blockIndex: " << blockIndex << std::endl;
	logger << "Before:" << std::endl;
//	debugPrint();
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
	LOG_BEGIN
	logger << "After:" << std::endl;
//	debugPrint();
	logger << std::endl;
	LOG_END
}

bool DiskSwap::isBlockInRam(size_t blockIndex, const size_t id) {
	return id == swapTable.at(RAM)->at(blockIndex); 
}

bool DiskSwap::isBlockInSwap(size_t blockIndex, size_t id) {
	return id != swapTable.at(RAM)->at(blockIndex); 
}

bool DiskSwap::HasSwappedBlocks(size_t blockIndex) {
	// it has if there is not 0 at the first swap level
	for(size_t level = 1; level < numLevels; ++level) {
		if(swapTable.at(level)->at(blockIndex) != 0) {
			return true;
		}
	}
	return false;
}

void DiskSwap::ReturnLastSwappedBlockIntoRam(size_t blockIndex) {
	logger << "ReturnLaswSwappedBlockIntoRam" << std::endl;
	logger.flush();
	size_t lastSwapLevel = FindLastLevel(blockIndex); 
	char* ramBlockAddress = poolAddress + blockIndex * blockSize; 
	swapTable.at(lastSwapLevel)->ReadBlock(ramBlockAddress, blockIndex);
	swapTable.at(RAM)->at(blockIndex) = swapTable.at(lastSwapLevel)->at(blockIndex);
	swapTable.at(lastSwapLevel)->at(blockIndex) = 0; // mark freed
}

size_t DiskSwap::Swap(size_t blockIndex) {
	logger << "DiskSwap:Swap()" << std::endl;
	logger.flush();
	// just check if pool block is empty (no need to swap in that case)
	if(swapTable.at(RAM) == 0) {
		logger << "DiskSwap::Swap(" << blockIndex << ")."
		          << "No need to swap: ram block with this index is empty!" << std::endl;
		return 1; // id for ram level
	}

	size_t swapLevel = FindEmptyLevel(blockIndex);
	if(swapLevel == 0) {
		// empty level not found, let's create it!
		swapTable.push_back( new DiskSwapLevel{numLevels, numBlocks, blockSize} );
		swapLevel = numLevels;
		++numLevels;
		logger << "+ 1 level" << std::endl;
		logger.flush();
	}
	LOG_BEGIN
	logger << "DiskSwap::Swap() swapLevel: " << swapLevel 
	<< ", blockIndex: " << blockIndex << std::endl;	
	LOG_END

	Swap(blockIndex, swapLevel);

	assert(swapId.at(blockIndex) < 255);
	size_t res = swapId.at(blockIndex);	
	swapId.at(blockIndex)++;
	return res; 
}

//char hash(char* data, size_t size) {
//	char res = 0;
//	for(size_t i = 0; i<size; ++i) {
//		res ^= data[i];
//	}
//	return res;
//}

void DiskSwap::debugPrint() {
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
	std::unique_ptr<char> tmpBlock{new char[blockSize]};

	std::vector<std::vector<size_t>> temp(numLevels, std::vector<size_t>(numBlocks, 0));	

	for(size_t level = 0; level < numLevels; ++level) {
		for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
			size_t value = static_cast<size_t>(swapTable.at(level)->at(blockIndex)); 
			if(value !=0) {
				swapTable.at(level)->ReadBlock(tmpBlock.get(), blockIndex);
				temp.at(level).at(blockIndex) = (size_t)((uchar*)tmpBlock.get())[0];
			}
		}
	}
	
	for(size_t level = 0; level < numLevels; ++level) {
		logger << "lv " << level << " | "; 
		for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
			size_t value = static_cast<size_t>(swapTable.at(level)->at(blockIndex)); 
			if(value !=0) {
				size_t hashValue = temp.at(level).at(blockIndex);
				temp.at(level).at(blockIndex) = hashValue;
				logger << std::setw(4) << hashValue << " ";
			} else {
				logger << std::setw(4) << "-" << " ";
			}

		}
		logger << std::endl;
	}
	logger << std::endl;
}

DiskSwap::~DiskSwap() {
	for(SwapLevel* swapLevel : swapTable) {
		delete swapLevel;
	}
}


