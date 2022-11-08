#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <vector>

//-------------------------------------------
// swap config
//-------------------------------------------
static const std::string SWAP_DIR_PATH = "swap";

using SwapIdType = uint8_t;
constexpr SwapIdType MAX_SWAP_LEVEL = std::numeric_limits<SwapIdType>::max();
//-------------------------------------------

class SwapLevel {
  protected:
    size_t level;
    size_t numBlocks;
    size_t blockSize;
    size_t totalSize;
    std::vector<SwapIdType> blockId;

  public:
    SwapLevel(size_t level, size_t numBlocks, size_t blockSize);

    SwapIdType &at(size_t blockIndex);
    const SwapIdType &at(size_t blockIndex) const;

    virtual void WriteBlock(void *data, size_t blockIndex) = 0;
    virtual void ReadBlock(void *data, size_t blockIndex) = 0;

    virtual ~SwapLevel();
};

class RamSwapLevel : public SwapLevel {
    char *poolAddress;
    std::mutex mutex;

  public:
    RamSwapLevel(size_t level, size_t numBlocks, size_t blockSize,
                 void *poolAddress);

    void WriteBlock(void *data, size_t blockIndex) override;
    void ReadBlock(void *data, size_t blockIndex) override;

    ~RamSwapLevel() override;
};

class DiskSwapLevel : public SwapLevel {
    std::filesystem::path filepath;
    std::fstream file;
    std::mutex mutex;

  public:
    DiskSwapLevel(size_t level, size_t numBlocks, size_t blockSize);

    void WriteBlock(void *data, size_t blockIndex) override;
    void ReadBlock(void *data, size_t blockIndex) override;

    ~DiskSwapLevel() override;
};

class MemoryPool;

class DiskSwap {
    MemoryPool *pool;
    size_t numBlocks;
    size_t blockSize;
    SwapIdType numLevels;
    char *poolAddress;
    std::vector<SwapLevel *> swapTable;

    const size_t RAM = 0;

    size_t FindEmptyLevel(size_t blockIndex);
    size_t FindLastLevel(size_t blockIndex);
    size_t FindSwapLevel(size_t blockIndex, SwapIdType id);

  public:
    DiskSwap(MemoryPool *ownerPool, void *poolAddress, size_t numBlocks,
             size_t blockSize);

    void MarkBlockAllocated(size_t blockIndex, SwapIdType id);
    void MarkBlockFreed(size_t blockIndex, SwapIdType id);

    bool isBlockInRam(size_t blockIndex, SwapIdType id);
    bool isBlockInSwap(size_t blockIndex, SwapIdType id);

    void LoadBlockIntoRam(size_t blockIndex, SwapIdType id);
    bool HasSwappedBlocks(size_t blockIndex);
    void ReturnLastSwappedBlockIntoRam(size_t blockIndex);

    void Swap(size_t blockIndex, size_t swapLevel);
    SwapIdType Swap(size_t blockIndex);

    ~DiskSwap();
};
