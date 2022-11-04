#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>
#include <iomanip>

#include "memory_manager.hpp"

MemoryManager::MemoryManager(size_t memorySize) {
    const size_t packOfBlocksSize =
        std::reduce(begin(blockSizes), end(blockSizes));
    const size_t N = memorySize / packOfBlocksSize;
    std::cout << "Memory size = " << memorySize << " bytes" << std::endl;
    std::cout << "N = " << N << std::endl;

    for (size_t size : blockSizes) {
        poolMap[size] = std::make_unique<MemoryPool>(N, size);
    }
    std::cout << "MAX_SWAP_LEVEL = " << static_cast<size_t>(MAX_SWAP_LEVEL)
              << std::endl;
}

MemoryBlock MemoryManager::getBlock(size_t size) {
    auto it = poolMap.lower_bound(size);
    if (it == end(poolMap)) {
        std::cerr << "Can't allocate more than " +
                         std::to_string(blockSizes.back()) +
                         " bytes for one block.";
        throw std::bad_alloc();
    }

    return it->second->getBlock(size);
}

size_t MemoryManager::maxBlockSize() const { return blockSizes.back(); }

//-----------------
// Show statistics 
//-----------------
using std::setw;
using std::endl;

void HorizontalSplit(int w) {
	std::cout << std::setfill('-');
	std::cout << std::left << "+"
		<< setw(w+1) << "-" << "+"
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< endl << std::setfill(' ');
}

void MemoryManager::printStatistics() const {
	const int w = 12;
	HorizontalSplit(w);
	std::cout << std::left << "|"
		<< " " << setw(w) << "Size (byte)" << "|"
		<< " " << setw(w) << "Number (ram)" << "|"
		<< " " << setw(w) << "Used" << "|" 
		<< " " << setw(w) << "Locked" << "|" 
		<< " " << setw(w) << "Swapped" << "|" 
		<< endl;
	HorizontalSplit(w);
	for(const auto& [size, pool] : poolMap) {
		const PoolStat& stat = pool->getStatistics();
		std::cout << std::left << "|"
		    << " " << setw(w) << size << "|"
			<< " " << setw(w) << pool->getNumBlocks() << "|"
			<< " " << setw(w) << stat.usedCounter << "|"
			<< " " << setw(w) << stat.lockedCounter << "|" 
			<< " " << setw(w) << stat.swappedCounter << "|"
			<< endl;
	}
	HorizontalSplit(w);
}
