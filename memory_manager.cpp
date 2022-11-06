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
#include "utils.hpp"

MemoryManager& memoryManager = MemoryManager::instance();

MemoryManager::MemoryManager() {}

void MemoryManager::init(size_t memorySize) {
	std::lock_guard<std::mutex> guard(mutex);
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
	std::lock_guard<std::mutex> guard(mutex);
	assert(initialized == true && "MemoryManager must be initialized before usage");
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
	std::lock_guard<std::mutex> guard(mutex);
	assert(initialized == true && "MemoryManager must be initialized before usage");
	return blockSizes.back(); 
}

//------------------------------
// Show statistics like a table 
//------------------------------
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

void Header(std::ostringstream& oss, int w) {
	oss << std::left << "|"
		<< " " << setw(w) << "Size (byte)" << "|"
		<< " " << setw(w) << "Blocks in RAM" << "|"
		<< " " << setw(w) << "Used" << "|" 
		<< " " << setw(w) << "Locked" << "|" 
		<< " " << setw(w) << "Swapped" << "|" 
		<< endl;
}

void MemoryManager::printStatistics() const {
	const int w = 14;
	std::ostringstream oss;
	HorizontalSplit(oss, w);
	Header(oss, w);
	HorizontalSplit(oss, w);

	size_t ramUsage = 0;
	size_t swapUsage = 0;
	mutex.lock();
	assert(initialized == true && "MemoryManager must be initialized before usage");
	for(const auto& [size, pool] : poolMap) {
		const PoolStat& stat = pool->getStatistics();
		oss << std::left << "|"
		    << " " << setw(w) << size << "|"
			<< " " << setw(w) << pool->getNumBlocks() << "|"
			<< " " << setw(w) << stat.usedCounter << "|"
			<< " " << setw(w) << stat.lockedCounter << "|" 
			<< " " << setw(w) << stat.swappedCounter << "|"
			<< endl;
		ramUsage += size * stat.usedCounter;
		swapUsage += size * stat.swappedCounter;
	}
	mutex.unlock();

	HorizontalSplit(oss, w);
	oss << "Memory manager usage [ RAM: " << HumanReadable{ramUsage}  
		<< "Disk (swap): " << HumanReadable{swapUsage} << "]" << std::endl;
	std::cout << oss.str().c_str();
}
