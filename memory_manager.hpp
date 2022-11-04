#pragma once

#include <map>
#include <memory>
#include <vector>

#include "config.hpp"
#include "memory_pool.hpp"

class MemoryManager {
    const std::vector<size_t> blockSizes{16,  32,   64,   128, 256,
                                         512, 1024, 2048, 4096};
    std::map<size_t, std::unique_ptr<MemoryPool>> poolMap;

    MemoryManager(const MemoryManager &) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;

  public:
    MemoryManager(size_t memorySize);
    MemoryBlock getBlock(size_t size);
    size_t maxBlockSize() const;
	void printStatistics() const;
};
