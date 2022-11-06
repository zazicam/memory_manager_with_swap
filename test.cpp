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
#include "format_time.hpp"

namespace fs = std::filesystem;

struct Progress {
	std::atomic<size_t> size = 0;
	std::atomic<size_t> read = 0;
	std::atomic<size_t> write = 0;
};

static std::atomic<bool> finished = false;
static std::map<fs::path, std::shared_ptr<Progress>> progressMap;

using std::setw;
void ShowFileProgress(const fs::path& filename, std::shared_ptr<Progress> progress) {
	// truncate filename if it's long 
	size_t filenameWidth = 40;
	std::ostringstream tmp;
	tmp << filename;
	std::ostringstream out;
	out << std::left << std::setw(filenameWidth) << tmp.str().substr(0, filenameWidth-1) << " ";
	
	out << setw(12) << progress->size << " "
		<< setw(12) << progress->read << " " 
		<< setw(12) << progress->write << std::endl;
	std::cout << out.str();
}

void ShowProgress() {
	std::cout << std::left << setw(40) << "filename" << " "
		<< setw(12) << "size" << " "
		<< setw(12) << "read" << " " 
		<< setw(12) << "write" << std::endl;
	for(const auto& [filename, progress] : progressMap) {
		ShowFileProgress(filename, progress);
	}
	std::cout << std::endl;
}

void ShowStatistics() {
	while(!finished) {
		memoryManager.printStatistics();
		std::cout << std::endl;
		ShowProgress();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	memoryManager.printStatistics();
	std::cout << std::endl;
}

size_t FileSize(std::ifstream& fin) {
    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);
	return filesize;
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
        CopyFile(inputDir/filename, outputDir/filename);
    }
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

	std::thread statThread(ShowStatistics);

    for (std::thread &t : threads) {
        t.join();
    }
    threads.clear();

	finished = true;
	statThread.join();
}

size_t CheckArgsAndGetMemorySize(int argc, char** argv) {
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
	return memorySizeMb;
}

void CheckIfDirectoryExists(const fs::path& dir) {
    if (!fs::exists(dir)) {
        std::cerr << "Input folder " << dir << " does not exist!"
                  << std::endl;
        exit(1);
    }
}

void CreateDirectoryIfNotExists(const fs::path& dir) {
	if(!fs::exists(dir) && !fs::create_directory(dir)) {
		std::cerr << "Output folder " << dir << " does not exists"
			" and can't create it!" << std::endl;
		exit(1);
	}
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
