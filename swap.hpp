#pragma once

#include <fstream>
#include <vector>
#include <cstddef>

using uchar = unsigned char;

class SwapLevel {
protected:
	size_t level;
	size_t numBlocks;
	size_t blockSize;
	size_t totalSize;
	std::vector<uchar> blockId;
public:
	SwapLevel(size_t level, size_t numBlocks, size_t blockSize);

	uchar& at(size_t blockIndex); 
	const uchar& at(size_t blockIndex) const; 

	virtual void WriteBlock(void* data, size_t blockIndex) = 0; 
	virtual void ReadBlock(void* data, size_t blockIndex) = 0; 
	
	virtual ~SwapLevel();
};

class RamSwapLevel : public SwapLevel {
	char* poolAddress;
public:
	RamSwapLevel(size_t level, size_t numBlocks, size_t blockSize, void* poolAddress);

	void WriteBlock(void* data, size_t blockIndex) override;
	void ReadBlock(void* data, size_t blockIndex) override; 

	~RamSwapLevel() override; 
};

class DiskSwapLevel : public SwapLevel {
	std::string filepath;
	std::fstream file;
public:
	DiskSwapLevel(size_t level, size_t numBlocks, size_t blockSize);

	void WriteBlock(void* data, size_t blockIndex) override; 
	void ReadBlock(void* data, size_t blockIndex) override; 

	~DiskSwapLevel() override;
};

class DiskSwap {
	size_t numBlocks;
	size_t blockSize;
	uchar numLevels;
	char* poolAddress;
	std::vector<SwapLevel*> swapTable;

	const size_t RAM = 0;
public:
	DiskSwap(void* poolAddress, size_t numBlocks, size_t blockSize);
		
	void UpdateRamBlockId(size_t blockId, size_t id); 
	void Swap(size_t blockIndex, size_t swapLevel); 
	void Swap(size_t blockIndex); 

	void Print();  // DEBUG

	~DiskSwap(); 
};

