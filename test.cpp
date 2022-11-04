#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "memory_manager.hpp"

MemoryManager *memoryManager = nullptr;

namespace fs = std::filesystem;

std::vector<MemoryBlock> ReadFileByBlocks(std::ifstream& fin) {
    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);
	
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

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();
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

    std::cout << "Copying completed" << std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << std::endl;

    return 0;
}
