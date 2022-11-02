#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <numeric>

#include "memory_pool.hpp"

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
	}

	MemoryBlock getBlock(size_t size) {
		auto it = poolMap.lower_bound(size);
		if(it == end(poolMap)) {
			std::cerr << "MemoryManager: getBlock() requested block of " << size << " bytes\n";
			std::cerr << "And there largest block in pool has size " << blockSizes.back() << " bytes\n";
			std::cerr << "Can't do that!" << std::endl;
			exit(1);
		}
		
		std::cout << "MemoryManager: getBlock() requested block of " << size << " bytes"
			<< "-> Ok" << std::endl;
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

	

	return 0;
}
