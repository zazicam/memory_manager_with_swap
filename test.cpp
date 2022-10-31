#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// debug only
#include <cstring>

#include "check.hpp"
#include "memory_pool.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

const size_t max_block_size = 1;
MemoryPool memoryPool(4, 8);

//const size_t max_block_size = 1024;
//MemoryPool memoryPool(1024, max_block_size);

// DEBUG ONLY
struct CheckBlock {
	char data[max_block_size];
	size_t size;
};


void CopyFile(const std::string &filename1, const std::string &filename2) {
    std::ifstream fin(filename1, std::ios::binary);
    std::ofstream fout(filename2, std::ios::binary);

    if (!fin) {
        std::cout << "Can't open file " << filename1 << std::endl;
    }

    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);

    size_t readed = 0;
	
	// debug only
	std::vector<CheckBlock> checkBlocks;

	// 1. read whole input file in random blocks [1 .. max_block_size] bytes
	std::vector<MemoryBlock> blocks;
    while (readed < filesize) {
        size_t size = 1 + rand() % max_block_size;
        size = std::min(size, filesize - readed);

        MemoryBlock mb = memoryPool.getBlock(size);
		mb.lock();
        char *buf = mb.getPtr<char>();
		LOG_BEGIN
		logger << "+1 block" << std::endl;
		logger << "buf: " << (void*)buf << std::endl;
		logger << "size: "<< size << std::endl;
		LOG_END
		blocks.push_back(mb);

        fin.read(buf, size);
        readed += size;

		// debug only
		CheckBlock cb;
		cb.size = size;
		std::memcpy(cb.data, buf, size); 
		checkBlocks.push_back(cb);

        mb.unlock();
    }
    fin.close();
	
	std::cout << "All blocks read! Count: " << blocks.size() << std::endl;

	// 2. write all blocks to the output file 
    if (!fout) {
        std::cout << "Can't open file " << filename2 << std::endl;
    }
	
	for(size_t i=0;i<blocks.size();++i) {
		MemoryBlock& mb = blocks.at(i);
		mb.lock();
		char* buf = mb.getPtr<char>();
		size_t size = mb.size();
//		std::cout << "buf: " << (void*)buf << std::endl;
//		std::cout << "size: "<< size << std::endl;
        fout.write(buf, size);

		if(std::strncmp(checkBlocks.at(i).data, buf, size)) {
			LOG_BEGIN
			logger << "WRONG BLOCK!!!" << std::endl;
			logger << "wait for: " << (size_t)(uchar)checkBlocks.at(i).data[0] << std::endl;
			logger << "got: " << (size_t)(uchar)buf[0] << std::endl;
			LOG_END
			mb.debugPrint();
//			exit(1);
		}

        mb.unlock();
	}
    fout.close();
	std::cout << "All blocks written!" << std::endl;

	for(auto& mb : blocks) {
        mb.free();
	}
	std::cout << "All blocks freed!" << std::endl;
}

void CopyAllFilesInSingleThread(const std::string &dir1, const std::string &dir2) {
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const std::string filename = entry.path().filename();
        const std::string path1 = dir1 + "/" + filename;
        const std::string path2 = dir2 + "/" + filename;
		CopyFile(path1, path2);
	}
}

void CopyAllFilesWithManyThreads(const std::string &dir1, const std::string &dir2) {
    std::vector<std::thread> threads;
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const std::string filename = entry.path().filename();
        const std::string path1 = dir1 + "/" + filename;
        const std::string path2 = dir2 + "/" + filename;
        threads.emplace_back(CopyFile, path1, path2);
    }

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();
}

int main() {
    std::string input_dir = "./input";
    std::string output_dir = "./output";
    std::cout << "Started copying" << std::endl;

    fs::remove_all(fs::path(output_dir));
    fs::create_directory(output_dir);

    CopyAllFilesWithManyThreads(input_dir, output_dir);

//	CopyAllFilesInSingleThread(input_dir, output_dir);

    std::cout << "Copying completed" << std::endl;
    std::cout << "Simple, but very slow (bytewise) checker started." << std::endl;
    std::cout << "You can check it manually in folder 'output'! :)" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << std::endl;

    return 0;
}
