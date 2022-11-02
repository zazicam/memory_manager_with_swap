#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <numeric>

class MemoryManager {
//	std::vector<unique_ptr<MemoryPool>> memoryPool;

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;
public:
	MemoryManager(size_t memorySize) {
		std::vector<size_t> blockSizes {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
		const size_t packOfBlocksSize = std::reduce(begin(blockSizes), end(blockSizes));
		int N = memorySize / packOfBlocksSize;
		std::cout << "Memory size = " << memorySize << " bytes" << std::endl;
		std::cout << "N = " << N << std::endl;
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
