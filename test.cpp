#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>
#include <map>

#include "memory_manager.hpp"
#include "utils.hpp"

static std::atomic<bool> finished = false;
static std::map<fs::path, std::shared_ptr<Progress>> progressMap;

void PrintStatisticsAndProgress() {
	memoryManager.printStatistics();
	std::cout << std::endl;
	ShowProgress(progressMap);
}

void DisplayInformation() {
	while(!finished) {
		PrintStatisticsAndProgress();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	PrintStatisticsAndProgress();
}

std::vector<MemoryBlock> ReadFileByBlocks(const fs::path& inputFile) {
    std::ifstream fin(inputFile, std::ios::binary);
    if (!fin) {
        std::cerr << "Can't open file " << inputFile << " for reading" << std::endl;
        exit(1);
    }
	std::shared_ptr<Progress> progress = progressMap[inputFile.filename()];

	size_t filesize = FileSize(fin);	
	progress->size = filesize;
    size_t read = 0;

    std::vector<MemoryBlock> blocks;
    while (read < filesize) {
        size_t size = 1 + rand() % memoryManager.maxBlockSize();
        size = std::min(size, filesize - read);

        MemoryBlock block = memoryManager.getBlock(size);

        fin.read(block.data(), size);
        read += size;
		progress->read = read;

        blocks.push_back(std::move(block));
    }
    fin.close();
	return blocks;
}

void WriteBlocksIntoFile(const std::vector<MemoryBlock>& blocks, const fs::path& outputFile) {
    std::ofstream fout(outputFile, std::ios::binary);
    if (!fout) {
        std::cerr << "Can't open file " << outputFile << " for writing" << std::endl;
        exit(1);
    }
	std::shared_ptr<Progress> progress = progressMap[outputFile.filename()];
	
    for (const auto& block : blocks) {
        fout.write(block.data(), block.size());
		progress->write += block.size();
    }
    fout.close();
}

void Free(std::vector<MemoryBlock>& blocks) {
    for (auto &mb : blocks) {
        mb.free();
    }
}

void CopyFile(const fs::path& inputFile, const fs::path& outputFile) {
    // 1. read whole input file by random blocks of [1 .. maxBlockSize] bytes
	std::vector<MemoryBlock> blocks = ReadFileByBlocks(inputFile);

    // 2. write all blocks to the output file 
	WriteBlocksIntoFile(blocks, outputFile);

    // 3. free blocks
	Free(blocks);
}

void CopyFilesInSingleThread(const fs::path &inputDir, const fs::path &outputDir) {
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
		progressMap[filename] = std::make_shared<Progress>();
    }

	std::thread infoThread(DisplayInformation);
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        CopyFile(inputDir/filename, outputDir/filename);
    }

	finished = true;
	infoThread.join();
}

void CopyFilesInMultipleThreads(const fs::path &inputDir, const fs::path &outputDir) {
    std::cout << "List of files in input folder '" << inputDir << "':" << std::endl;
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        std::cout << filename << std::endl;
		progressMap[filename] = std::make_shared<Progress>();
    }

    std::vector<std::thread> threads;
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        threads.emplace_back(CopyFile, inputDir/filename, outputDir/filename);
    }
    std::cout << "\nStarted copying in " << threads.size() << " threads..."
              << std::endl;

	std::thread infoThread(DisplayInformation);

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();

	finished = true;
	infoThread.join();
}

int main(int argc, char** argv) {
    const fs::path inputDir = "./input";
    const fs::path outputDir = "./output";
	CheckIfDirectoryExists(inputDir);
	CreateDirectoryIfNotExists(outputDir);
	
	size_t memorySizeMb = CheckArgsAndGetMemorySize(argc, argv);
    memoryManager.init(memorySizeMb * 1024 * 1024);

	auto startTime = std::chrono::steady_clock::now();

//    CopyFilesInSingleThread(inputDir, outputDir);
    CopyFilesInMultipleThreads(inputDir, outputDir);

	auto endTime = std::chrono::steady_clock::now();

    std::cout << "\nCopying completed in " 
		<< hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime) }
		<< "\n" << std::endl;

    return 0;
}
