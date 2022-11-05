#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#include "memory_manager.hpp"
#include "chrono_utils.hpp"

MemoryManager *memoryManager = nullptr;

namespace fs = std::filesystem;

static std::atomic<bool> finished = false;

void ShowStatistics() {
	while(!finished) {
		memoryManager->printStatistics();
		std::cout << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	memoryManager->printStatistics();
}

size_t FileSize(std::ifstream& fin) {
    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);
	return filesize;
}

std::vector<MemoryBlock> ReadFileByBlocks(std::ifstream& fin) {
	size_t filesize = FileSize(fin);	
    size_t readed = 0;

    std::vector<MemoryBlock> blocks;
    while (readed < filesize) {
        size_t size = 1 + rand() % memoryManager->maxBlockSize();
        size = std::min(size, filesize - readed);

        MemoryBlock block = memoryManager->getBlock(size);

        fin.read(block.data(), size);
        readed += size;

        blocks.push_back(std::move(block));
    }
    fin.close();
	return blocks;
}

void WriteBlocksIntoFile(const std::vector<MemoryBlock>& blocks, std::ofstream& fout) {
    for (const auto& block : blocks) {
        fout.write(block.data(), block.size());
    }
    fout.close();
}

void Free(std::vector<MemoryBlock>& blocks) {
    for (auto &mb : blocks) {
        mb.free();
    }
}

void CopyFile(const fs::path filename1, const fs::path filename2) {
    std::ifstream fin(filename1, std::ios::binary);
    std::ofstream fout(filename2, std::ios::binary);

    if (!fin) {
        std::cerr << "Can't open file " << filename1 << " for reading"
                  << std::endl;
        exit(1);
    }
    if (!fout) {
        std::cerr << "Can't open file " << filename2 << " for writing"
                  << std::endl;
        exit(1);
    }

    // 1. read whole input file by random blocks of [1 .. maxBlockSize] bytes
	std::vector<MemoryBlock> blocks = ReadFileByBlocks(fin);

    // 2. write all blocks to the output file 
	WriteBlocksIntoFile(blocks, fout);

    // 3. free blocks
	Free(blocks);
}

void CopyFilesInSingleThread(const fs::path &dir1, const fs::path &dir2) {
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const fs::path filename = entry.path().filename();
        const fs::path path1 = dir1 / filename;
        const fs::path path2 = dir2 / filename;
        CopyFile(path1, path2);
    }
}

void CopyFilesInMultipleThreads(const fs::path &dir1, const fs::path &dir2) {
    std::cout << "List of files in input folder '" << dir1 << "':" << std::endl;
    std::vector<std::thread> threads;
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const fs::path filename = entry.path().filename();
        std::cout << filename << std::endl;
        const fs::path path1 = dir1 / filename;
        const fs::path path2 = dir2 / filename;
        threads.emplace_back(CopyFile, path1, path2);
    }
    std::cout << "Started copying in " << threads.size() << " threads..."
              << std::endl;

	std::thread statThread(ShowStatistics);

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();

	finished = true;
	statThread.join();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "This program needs an integer argument." << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "\t" << argv[0] << " [size of RAM in Mb]" << std::endl;
        exit(1);
    }

    int memorySizeMb = 0;
    std::istringstream iss(argv[1]);
    iss >> memorySizeMb;
    if (iss.fail() || !iss.eof() || memorySizeMb <= 0) {
        std::cout << "[size of RAM in Mb]: wrong input!" << std::endl;
        exit(1);
    }

	auto startTime = std::chrono::steady_clock::now();
	
    memoryManager = new MemoryManager(memorySizeMb * 1024 * 1024);
    const fs::path inputDir = "./input";
    const fs::path outputDir = "./output";

    bool directoryExists = fs::exists(inputDir);
    if (!directoryExists) {
        std::cerr << "Input folder " << inputDir << " does not exist!"
                  << std::endl;
        exit(1);
    }

    fs::remove_all(fs::path(outputDir));
    fs::create_directory(outputDir);

//    CopyFilesInSingleThread(inputDir, outputDir);
    CopyFilesInMultipleThreads(inputDir, outputDir);

    delete memoryManager;

	auto endTime = std::chrono::steady_clock::now();
	std::chrono::milliseconds executeTime(
		std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime));

    std::cout << std::endl;
    std::cout << "Copying completed in " << hh_mm_ss{executeTime}  
		<< std::endl << std::endl;

    return 0;
}
