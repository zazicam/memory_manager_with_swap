#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "memory_manager/memory_manager.hpp"
#include "utils/utils.hpp"

namespace fs = std::filesystem;
using utils::Progress;

void CopyFilesInSingleThread(const fs::path &inputDir,
                             const fs::path &outputDir);
void CopyFilesInMultipleThreads(const fs::path &inputDir,
                                const fs::path &outputDir);
void CopyFile(const fs::path &inputFile, const fs::path &outputFile);

std::vector<MemoryBlock> ReadFileByBlocks(const fs::path &inputFile);
void WriteBlocksIntoFile(const std::vector<MemoryBlock> &blocks,
                         const fs::path &outputFile);
void Free(std::vector<MemoryBlock> &blocks);

void PrintStatisticsAndProgress();
void DisplayInformation();

//---------------------------------------------------------------
// The main() function is here with some global variables just
// to control threads execution and display their progress.
//---------------------------------------------------------------
static std::atomic<bool> finished = false;
static std::map<fs::path, std::shared_ptr<Progress>> progressMap;

int main(int argc, char **argv) {
    setlocale(0, "");

    const fs::path inputDir = "./input";
    const fs::path outputDir = "./output";
    utils::CheckIfDirectoryExists(inputDir);
    utils::CreateDirectoryIfNotExists(outputDir);

    size_t memorySizeMb = utils::CheckArgsAndGetMemorySize(argc, argv);
    memoryManager.init(memorySizeMb * 1024 * 1024);

    utils::timer.start();

    //    CopyFilesInSingleThread(inputDir, outputDir);
    CopyFilesInMultipleThreads(inputDir, outputDir);

    std::cout << "\nCopying completed in "
              << utils::hh_mm_ss{utils::timer.elapsed()} << "\n"
              << std::endl;
    return 0;
}

void CopyFilesInSingleThread(const fs::path &inputDir,
                             const fs::path &outputDir) {
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        std::cout << filename << std::endl;
        progressMap[filename] = std::make_shared<Progress>();
    }

    std::thread infoThread(DisplayInformation);
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        CopyFile(inputDir / filename, outputDir / filename);
    }

    finished = true;
    infoThread.join();
}

void CopyFilesInMultipleThreads(const fs::path &inputDir,
                                const fs::path &outputDir) {
    std::cout << "List of files in input folder '" << inputDir
              << "':" << std::endl;
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        std::cout << filename << std::endl;
        progressMap[filename] = std::make_shared<Progress>();
    }

    std::vector<std::thread> threads;
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        const fs::path filename = entry.path().filename();
        threads.emplace_back(CopyFile, inputDir / filename,
                             outputDir / filename);
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

//-------------------------------------------------------------------------
// This is the core function - it makes a copy of a one file in 3 steps:
// 1. read whole input file by random blocks of [1 .. maxBlockSize] bytes
// 2. write all blocks to the output file
// 3. free blocks
//-------------------------------------------------------------------------
void CopyFile(const fs::path &inputFile, const fs::path &outputFile) {
    std::vector<MemoryBlock> blocks = ReadFileByBlocks(inputFile);
    WriteBlocksIntoFile(blocks, outputFile);
    Free(blocks);
}

std::vector<MemoryBlock> ReadFileByBlocks(const fs::path &inputFile) {
    std::ifstream fin(inputFile, std::ios::binary);
    if (!fin) {
        std::cerr << "Can't open file " << inputFile << " for reading"
                  << std::endl;
        exit(1);
    }
    std::shared_ptr<Progress> progress = progressMap[inputFile.filename()];

    size_t filesize = utils::FileSize(fin);
    progress->size = filesize;
    size_t read = 0;

    std::vector<MemoryBlock> blocks;
    while (read < filesize) {
        size_t size = 1 + rand() % memoryManager.maxBlockSize();
        size = std::min(size, filesize - read);

        // Allocating memory
        MemoryBlock block = memoryManager.getBlock(size);

        fin.read(block.data(), size);
        read += size;
        progress->read = read;

        blocks.push_back(std::move(block));
    }
    fin.close();
    return blocks;
}

void WriteBlocksIntoFile(const std::vector<MemoryBlock> &blocks,
                         const fs::path &outputFile) {
    std::ofstream fout(outputFile, std::ios::binary);
    if (!fout) {
        std::cerr << "Can't open file " << outputFile << " for writing"
                  << std::endl;
        exit(1);
    }
    std::shared_ptr<Progress> progress = progressMap[outputFile.filename()];

    for (const auto &block : blocks) {
        fout.write(block.data(), block.size());
        progress->write += block.size();
    }
    fout.close();
}

void Free(std::vector<MemoryBlock> &blocks) {
    for (auto &mb : blocks) {
        mb.free();
    }
}

void DisplayInformation() {
    while (!finished) {
        PrintStatisticsAndProgress();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    PrintStatisticsAndProgress();
}

void PrintStatisticsAndProgress() {
    std::cout << "\nMemory pool statistics:" << std::endl;
    memoryManager.printStatistics();
    std::cout << "\nCopy folder test progress:" << std::endl;
    utils::ShowProgress(progressMap);
}
