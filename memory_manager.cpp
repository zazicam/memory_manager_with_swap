#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <numeric>

#include "memory_pool.hpp"
#include "config.hpp"

class MemoryManager {
	const std::vector<size_t> blockSizes {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
	std::map<size_t, std::unique_ptr<MemoryPool>> poolMap;

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;
public:
	MemoryManager(size_t memorySize) {
		const size_t packOfBlocksSize = std::reduce(begin(blockSizes), end(blockSizes));
		const size_t N = memorySize / packOfBlocksSize;
		std::cout << "Memory size = " << memorySize << " bytes" << std::endl;
		std::cout << "N = " << N << std::endl;

		for(size_t size : blockSizes) {
			poolMap[size] = std::make_unique<MemoryPool>(N, size);
		}
		std::cout << "MAX_SWAP_LEVEL = " << static_cast<size_t>(MAX_SWAP_LEVEL) << std::endl;
	}

	MemoryBlock getBlock(size_t size) {
		auto it = poolMap.lower_bound(size);
		if(it == end(poolMap)) {
			std::cerr << "Can't allocate more than " + std::to_string(blockSizes.back()) + 
				" bytes for one block.";
			throw std::bad_alloc();
		}
		
		return it->second->getBlock(size);
	}

};

int main(int argc, char** argv) {
	if(argc < 2) {
		std::cout << "This program needs an integer argument." << std::endl;
		std::cout << "Usage: " << std::endl;
		std::cout << "\t" << argv[0] << " [size of RAM in Mb]" << std::endl;
		return 0;
	}

	int memorySizeMb = 0;
	std::istringstream iss(argv[1]);
	iss >> memorySizeMb;
	if(iss.fail() || !iss.eof() || memorySizeMb <= 0) {
		std::cout << "[size of RAM in Mb]: wrong input!" << std::endl;
		return 0;
	}

	MemoryManager memory(memorySizeMb * 1024 * 1024);
	
	size_t blockSize = 1;
	for(int i=0; i<20; i++) {

		std::cout << "need block of " << blockSize << " bytes -> ";
		try {
			MemoryBlock mb = memory.getBlock(blockSize);
			std::cout << "got " << mb.size() << " bytes" << std::endl;
			mb.free();
		} catch(std::bad_alloc& ex) {
			std::cout << "exception: " << ex.what() << std::endl;	
		}
	
		blockSize *= 2;
	}
	

	return 0;
}
