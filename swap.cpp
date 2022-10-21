#include <cassert>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstring>

#include "swap.hpp"

namespace fs = std::filesystem;

Swap::Swap(size_t numBlocks, size_t blockSize, std::string filename) {
	assert(numBlocks > 0);
	assert(blockSize >= sizeof(size_t)); // empty block contains address of the next empty block in file

	this->numBlocks = numBlocks;
	this->blockSize = blockSize;
	this->totalSize = numBlocks * blockSize;

	filepath = "./" + filename + "_" + std::to_string(numBlocks) + "_" + std::to_string(blockSize) + ".bin";

	// just to create a new file
	std::ofstream tmp(filepath, std::ios::binary);
	tmp.close();

	try {
		std::filesystem::resize_file(filepath, totalSize);
	} catch(const fs::filesystem_error&) {
		std::cerr << "Error: Swap() can't resize file to " << totalSize << " bytes" << std::endl; 
	}

	file = std::move(std::fstream(filepath, std::ios::binary | std::ios::in | std::ios::out));	
	if(!file) {
		std::cerr << "Error: Swap() can't create file " << filepath << " for writing" << std::endl;
		exit(1);
	}

	// create internal list of blocks
	for (size_t i = 0; i < numBlocks; ++i) {
		size_t pos = i * blockSize;
		file.seekp(pos);
		size_t nextPos = pos + blockSize;
		file.write(reinterpret_cast<char*>(&nextPos), sizeof(nextPos));
	}
	nextBlock = 0;
}

Swap::~Swap() {
	file.close();	
	fs::remove(filepath);
}

size_t Swap::alloc() {
	if (nextBlock >= totalSize) {
		std::cerr << "No free space in disk pool!" << std::endl;
		exit(2);
	}

	size_t pos = nextBlock;
	file.seekg(pos);
	file.read(reinterpret_cast<char*>(&nextBlock), sizeof(nextBlock));
	return pos;
}

void Swap::free(size_t pos) {
	file.seekp(pos);
	file.write(reinterpret_cast<char*>(&nextBlock), sizeof(nextBlock));
	nextBlock = pos;
}

void Swap::write(size_t pos, void* data, size_t size) {
	assert(pos < totalSize);
	file.seekp(pos);
	file.write(reinterpret_cast<char*>(data), size);
}

void Swap::read(size_t pos, void* data, size_t size) {
	assert(pos < totalSize);
	file.seekg(pos);
	file.read(reinterpret_cast<char*>(data), size);
}

void Swap::swap(size_t pos, void* data, size_t size) {
	assert(pos < totalSize);
    std::unique_ptr<char> tmpBlock { new char[size] };
    read(pos, tmpBlock.get(), size);
    write(pos, data, size);
    std::memcpy(data, tmpBlock.get(), size);
}

