#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "check.hpp"
#include "memory_pool.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

// bytewise
//const size_t max_block_size = 1;
//const size_t num_blocks = 3;
//MemoryPool memoryPool(3, 8);

// small 
//const size_t max_block_size = 16;
//const size_t num_blocks = 3;
//MemoryPool memoryPool(num_blocks, max_block_size);

// medium files (~100Mb)
const size_t max_block_size = 1024;
const size_t num_blocks = 1024; 
MemoryPool memoryPool(num_blocks, max_block_size);

// large files (~2Gb)
//const size_t max_block_size = 1024;
//const size_t num_blocks = 16 * 1024;
//MemoryPool memoryPool(1024, max_block_size);


void CopyFile(const fs::path &filename1, const fs::path &filename2) {
    std::ifstream fin(filename1, std::ios::binary);
    std::ofstream fout(filename2, std::ios::binary);

    if (!fin) {
        std::cerr << "Can't open file " << filename1 << " for reading" << std::endl;
		exit(1);
    }
    if (!fout) {
        std::cerr << "Can't open file " << filename2 << " for writing" << std::endl;
		exit(1);
    }

    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);

    size_t readed = 0;
	
	// 1. read whole input file in random blocks [1 .. max_block_size] bytes
	std::vector<MemoryBlock> blocks;
    while (readed < filesize) {
        size_t size = 1 + rand() % max_block_size;
        size = std::min(size, filesize - readed);

        MemoryBlock mb = memoryPool.getBlock(size);
		mb.lock();

        char *buf = mb.getPtr<char>();

        fin.read(buf, size);
        readed += size;
		blocks.push_back(std::move(mb));

        mb.unlock();
    }
    fin.close();

	// 2. write all blocks to the output file 
    if (!fout) {
        std::cerr << "Can't open file " << filename2 << " for writing" << std::endl;
		exit(1);
    }
	
	for(size_t i=0;i<blocks.size();++i) {
		MemoryBlock& mb = blocks.at(i);
		mb.lock();

		char* buf = mb.getPtr<char>();
		size_t size = mb.size();
        fout.write(buf, size);

        mb.unlock();
	}
    fout.close();

	// 3. free blocks
	for(auto& mb : blocks) {
        mb.free();
	}
}

void CopyAllFilesInSingleThread(const fs::path &dir1, const fs::path &dir2) {
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const fs::path filename = entry.path().filename();
        const fs::path path1 = dir1 / filename;
        const fs::path path2 = dir2 / filename;
		CopyFile(path1, path2);
	}
}

void CopyAllFilesWithManyThreads(const fs::path &dir1, const fs::path &dir2) {
    std::cout << "List of files in input folder '" << dir1 << "':" << std::endl;
    std::vector<std::thread> threads;
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const fs::path filename = entry.path().filename();
        std::cout << filename << std::endl;
        const fs::path path1 = dir1 / filename;
        const fs::path path2 = dir2 / filename;
        threads.emplace_back(CopyFile, path1, path2);
    }
    std::cout << "Started copying in " << threads.size() << " threads..." << std::endl;

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();
}

int main() {
    const fs::path inputDir = "./input";
    const fs::path outputDir = "./output";

    bool directoryExists = fs::exists(inputDir);
    if (!directoryExists) {
        std::cerr << "Input folder '" << inputDir << "' does not exist!"
                  << std::endl;
        exit(1);
    }

    fs::remove_all(fs::path(outputDir));
    fs::create_directory(outputDir);

    CopyAllFilesWithManyThreads(inputDir, outputDir);

//	CopyAllFilesInSingleThread(input_dir, output_dir);

    std::cout << "Copying completed" << std::endl;
	std::cout << "-----------------" << std::endl;
	std::cout << std::endl;

    return 0;
}
