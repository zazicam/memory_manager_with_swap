#include <iostream>
#include <sstream>
#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>
#include <iomanip>

#include "memory_manager.hpp"

MemoryManager& memoryManager = MemoryManager::instance();

MemoryManager::MemoryManager() {}

void MemoryManager::init(size_t memorySize) {
	assert(initialized == false && "MemoryManager can be initialized only once");
	initialized = true;

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
	assert(initialized == true && "MemoryManager must be initialized before usage");
	std::lock_guard<std::mutex> guard(mutex);
    auto it = poolMap.lower_bound(size);
    if (it == end(poolMap)) {
        std::cerr << "Can't allocate more than " +
                         std::to_string(blockSizes.back()) +
                         " bytes for one block.";
        throw std::bad_alloc();
    }

    return it->second->getBlock(size);
}

size_t MemoryManager::maxBlockSize() const { 
	assert(initialized == true && "MemoryManager must be initialized before usage");
	return blockSizes.back(); 
}

//-----------------
// Show statistics 
//-----------------
using std::setw;
using std::endl;

void HorizontalSplit(std::ostringstream& oss, int w) {
	oss << std::setfill('-') << std::left << "+";
	oss	<< setw(w+1) << "-" << "+"
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< setw(w+1) << "-" << "+" 
		<< endl << std::setfill(' ');
}

void MemoryManager::printStatistics() const {
	assert(initialized == true && "MemoryManager must be initialized before usage");
	std::ostringstream oss;
	const int w = 12;
	HorizontalSplit(oss, w);
	oss << std::left << "|"
		<< " " << setw(w) << "Size (byte)" << "|"
		<< " " << setw(w) << "Number (ram)" << "|"
		<< " " << setw(w) << "Used" << "|" 
		<< " " << setw(w) << "Locked" << "|" 
		<< " " << setw(w) << "Swapped" << "|" 
		<< endl;
	HorizontalSplit(oss, w);

	mutex.lock();
	for(const auto& [size, pool] : poolMap) {
		const PoolStat& stat = pool->getStatistics();
		oss << std::left << "|"
		    << " " << setw(w) << size << "|"
			<< " " << setw(w) << pool->getNumBlocks() << "|"
			<< " " << setw(w) << stat.usedCounter << "|"
			<< " " << setw(w) << stat.lockedCounter << "|" 
			<< " " << setw(w) << stat.swappedCounter << "|"
			<< endl;
	}
	mutex.unlock();

	HorizontalSplit(oss, w);
	std::cout << oss.str().c_str();
}
