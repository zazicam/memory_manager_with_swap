#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "memory_pool.hpp"

//-------------------------------------
// Singleton (uses single swap folder)
//-------------------------------------
class MemoryManager {
	bool initialized = false;
	size_t memorySize;
    const std::vector<size_t> blockSizes{16,  32,   64,   128, 256,
                                         512, 1024, 2048, 4096};
    std::map<size_t, std::unique_ptr<MemoryPool>> poolMap;
	mutable std::mutex mutex;

  	MemoryManager();
  public:
	void init(size_t memorySize);

	static MemoryManager& instance() {
		static MemoryManager memory;
		return memory;
	}

    MemoryManager(const MemoryManager &) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;
    MemoryManager(MemoryManager &&) = delete;
    MemoryManager &operator=(MemoryManager &&) = delete;

    MemoryBlock getBlock(size_t size);
    size_t maxBlockSize() const;
	void printStatistics() const;
};

// An instance created in memory_manager.cpp 
extern MemoryManager& memoryManager;
