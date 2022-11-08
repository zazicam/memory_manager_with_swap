#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include "memory_manager.hpp"
#include "../utils/utils.hpp"

MemoryManager &memoryManager = MemoryManager::instance();

void MemoryManager::init(size_t memoryLimit) {
    std::lock_guard<std::mutex> guard(mutex);
    assert(memorySize == 0 &&
           "MemoryManager initialized already, can't do it twice");
    memorySize = memoryLimit;

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
    assert(memorySize != 0 && "MemoryManager must be initialized before usage");
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
    assert(memorySize != 0 && "MemoryManager must be initialized before usage");
    return blockSizes.back();
}

//------------------------------
// Show statistics like a table
//------------------------------
using utils::hr;
using utils::Table;

void MemoryManager::printStatistics() const {
    Table table({12, 14, 14, 9, 12, 12});
    table << hr;
    table << "Block size"
          << "Blocks (RAM)"
          << "Used"
          << "Locked"
          << "Swapped"
          << "Swap level" << hr;

    size_t ramUsage = 0;
    size_t swapUsage = 0;
    mutex.lock();
    assert(memorySize != 0 && "MemoryManager must be initialized before usage");
    for (const auto &[size, pool] : poolMap) {
        const PoolStat &stat = pool->getStatistics();
        table << size << pool->getNumBlocks() << stat.usedCounter
              << stat.lockedCounter << stat.swappedCounter << stat.swapLevels;

        ramUsage += size * stat.usedCounter;
        swapUsage += size * stat.swappedCounter;
    }
    mutex.unlock();

    table << hr;
    std::cout << table;
    std::cout << "Memory manager usage [Limit RAM: "
              << utils::HumanReadable{memorySize} << ", "
              << "Used RAM: " << utils::HumanReadable{ramUsage} << ", "
              << "Disk(swap): " << utils::HumanReadable{swapUsage} << "]\n";
}
