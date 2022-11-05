#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

#include "logger.hpp"
#include "swap.hpp"

namespace fs = std::filesystem;

//-------------------------------------------------------------------
// class SwapLevel
//-------------------------------------------------------------------
SwapLevel::SwapLevel(size_t level, size_t numBlocks, size_t blockSize)
    : level(level), numBlocks(numBlocks), blockSize(blockSize),
      totalSize(numBlocks * blockSize),
      blockId(std::vector<SwapIdType>(numBlocks, 0)) {}

SwapIdType &SwapLevel::at(size_t blockIndex) {
    assert(blockIndex < numBlocks);
    return blockId.at(blockIndex);
}

const SwapIdType &SwapLevel::at(size_t blockIndex) const {
    assert(blockIndex < numBlocks);
    return blockId.at(blockIndex);
}

SwapLevel::~SwapLevel() {}

//-------------------------------------------------------------------
// class RamSwapLevel
//-------------------------------------------------------------------
RamSwapLevel::RamSwapLevel(size_t level, size_t numBlocks, size_t blockSize,
                           void *poolAddress)
    : SwapLevel(level, numBlocks, blockSize),
      poolAddress(static_cast<char *>(poolAddress)) {}

void RamSwapLevel::WriteBlock(void *data, size_t blockIndex) {
    std::lock_guard<std::mutex> guard(mutex);
    char *blockAddress = poolAddress + blockIndex * blockSize;
    std::memcpy(blockAddress, data, blockSize);
}

void RamSwapLevel::ReadBlock(void *data, size_t blockIndex) {
    std::lock_guard<std::mutex> guard(mutex);
    char *blockAddress = poolAddress + blockIndex * blockSize;
    std::memcpy(data, blockAddress, blockSize);
}

RamSwapLevel::~RamSwapLevel() {}

//-------------------------------------------------------------------
// class DiskLevel
//-------------------------------------------------------------------
DiskSwapLevel::DiskSwapLevel(size_t level, size_t numBlocks, size_t blockSize)
    : SwapLevel(level, numBlocks, blockSize) 
{
	fs::path swapDir = SWAP_DIR_PATH;
	if(!fs::exists(swapDir) && !fs::create_directory(swapDir)) {
		std::cerr << "Swap folder " << swapDir << " does not exists"
			" and can't create it!" << std::endl;
		exit(1);
	}

    std::string filename = std::string("swap") + "_" + std::to_string(numBlocks) + "x" +
               std::to_string(blockSize) + "_" + "L" + std::to_string(level) +
               ".bin";

	filepath = swapDir / filename;

    // just to create a new file
    std::ofstream tmp(filepath, std::ios::binary);
    tmp.close();

    try {
        std::filesystem::resize_file(filepath, totalSize);
    } catch (const fs::filesystem_error &) {
        std::cerr << "Error: Swap() can't resize file to " << totalSize
                  << " bytes!\n"
                  << "Wrong rights or limit for amount of file descriptors"
                  << std::endl;
        exit(1);
    }

    file = std::move(std::fstream(filepath, std::ios::binary | std::ios::in |
                                                std::ios::out));

    if (!file) {
        std::cerr << "Error: Swap() can't create file " << filepath
                  << " for writing!\n"
                  << "Wrong rights or limit for amount of file descriptors"
                  << std::endl;
        exit(1);
    }
}

void DiskSwapLevel::WriteBlock(void *data, size_t blockIndex) {
    assert(blockIndex < numBlocks);
    std::lock_guard<std::mutex> guard(mutex);
    size_t pos = blockIndex * blockSize;
    file.seekp(pos);
    file.write(reinterpret_cast<char *>(data), blockSize);
}

void DiskSwapLevel::ReadBlock(void *data, size_t blockIndex) {
    assert(blockIndex < numBlocks);
    std::lock_guard<std::mutex> guard(mutex);
    size_t pos = blockIndex * blockSize;
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
DiskSwap::DiskSwap(void *poolAddress, size_t numBlocks, size_t blockSize)
    : numBlocks(numBlocks), blockSize(blockSize), numLevels(2),
      poolAddress(static_cast<char *>(poolAddress)),
      swapTable({new RamSwapLevel(0, numBlocks, blockSize, poolAddress),
                 new DiskSwapLevel(1, numBlocks, blockSize)}) 
{
}

void DiskSwap::LoadBlockIntoRam(size_t blockIndex, size_t id) {
    if (id == swapTable.at(RAM)->at(blockIndex)) {
        ;
    } else {
        size_t swapLevel = FindSwapLevel(blockIndex, id);
        Swap(blockIndex, swapLevel);
    }
}

size_t DiskSwap::FindSwapLevel(size_t blockIndex, size_t id) {
    assert(blockIndex < numBlocks);
    size_t swapLevel = 0;
    for (size_t level = 1; level < numLevels; ++level) {
        if (id == swapTable.at(level)->at(blockIndex)) {
            swapLevel = level;
            break;
        }
    }
    return swapLevel;
}

size_t DiskSwap::FindEmptyLevel(size_t blockIndex) {
    // id == 0 means this level is empty and we can use it
    return FindSwapLevel(blockIndex, 0);
}

size_t DiskSwap::FindLastLevel(size_t blockIndex) {
    assert(blockIndex < numBlocks);
    if (numLevels == 1)
        return 0;

    size_t level = numLevels - 1;
    while (swapTable.at(level)->at(blockIndex) == 0)
        --level;

    return level;
}

void DiskSwap::MarkBlockAllocated(size_t blockIndex, size_t id) {
    swapTable.at(RAM)->at(blockIndex) = id;
}

void DiskSwap::MarkBlockFreed(size_t blockIndex, size_t id) {
    size_t goalLevel = FindSwapLevel(blockIndex, id);
    swapTable.at(goalLevel)->at(blockIndex) = 0;
}

void DiskSwap::Swap(size_t blockIndex, size_t swapLevel) {
    assert(blockIndex < numBlocks);
    std::unique_ptr<char> tmpBlock{new char[blockSize]};
    swapTable.at(swapLevel)->ReadBlock(tmpBlock.get(), blockIndex);
    char *blockAddress = poolAddress + blockIndex * blockSize;
    swapTable.at(swapLevel)->WriteBlock(blockAddress, blockIndex);
    swapTable.at(RAM)->WriteBlock(tmpBlock.get(), blockIndex);
    std::swap(swapTable.at(RAM)->at(blockIndex),
              swapTable.at(swapLevel)->at(blockIndex));
}

bool DiskSwap::isBlockInRam(size_t blockIndex, const size_t id) {
    return id == swapTable.at(RAM)->at(blockIndex);
}

bool DiskSwap::isBlockInSwap(size_t blockIndex, size_t id) {
    return id != swapTable.at(RAM)->at(blockIndex);
}

bool DiskSwap::HasSwappedBlocks(size_t blockIndex) {
    for (size_t level = 1; level < numLevels; ++level) {
        if (swapTable.at(level)->at(blockIndex) != 0) {
            return true;
        }
    }
    return false;
}

void DiskSwap::ReturnLastSwappedBlockIntoRam(size_t blockIndex) {
    size_t lastSwapLevel = FindLastLevel(blockIndex);
    char *ramBlockAddress = poolAddress + blockIndex * blockSize;
    swapTable.at(lastSwapLevel)->ReadBlock(ramBlockAddress, blockIndex);
    swapTable.at(RAM)->at(blockIndex) =
        swapTable.at(lastSwapLevel)->at(blockIndex);
    swapTable.at(lastSwapLevel)->at(blockIndex) = 0; // mark freed
}

SwapIdType DiskSwap::Swap(size_t blockIndex) {
    // just check if pool block is empty (no need to swap in that case)
    if (swapTable.at(RAM) == 0) {
        std::cout << "DiskSwap::Swap(" << blockIndex << ")."
                  << "No need to swap: ram block with this index is empty!"
                  << std::endl;
        return 1; // id for ram level
    }

    size_t swapLevel = FindEmptyLevel(blockIndex);
    if (swapLevel == 0) {
        // empty level not found, let's create it!
        // but in this realisation we can have fixed amount of levels
        assert(numLevels < MAX_SWAP_LEVEL);

        swapTable.push_back(new DiskSwapLevel{numLevels, numBlocks, blockSize});
        swapLevel = numLevels;
        ++numLevels;
    }

    Swap(blockIndex, swapLevel);

    // we are to return a new id for block in ram (after swap it has new id)
    // so, let's just find any free id in interval [2 .. MAX_SWAP_LEVEL]
    std::vector<char> usedIdTable(MAX_SWAP_LEVEL + 1, 0);
    for (size_t level = 0; level < numLevels; ++level) {
        SwapIdType id = swapTable.at(level)->at(blockIndex);
        usedIdTable.at(id) = 1;
    }
    // iterate it reverse order to avoid pitfall with unsigned overflow
    // (infinite loop)
    SwapIdType newId = 0;
    for (SwapIdType id = MAX_SWAP_LEVEL; id >= 2; --id) {
        if (usedIdTable.at(id) == 0) {
            newId = id;
            break;
        }
    }

    assert(newId >= 2); // 0 - empty block, 1 - reserved for allocation in ram
                        // (without swap)
    return newId;
}

DiskSwap::~DiskSwap() {
    for (SwapLevel *swapLevel : swapTable) {
        delete swapLevel;
    }
}
