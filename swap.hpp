#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

class Swap {
    size_t numBlocks;
    size_t blockSize;
    size_t totalSize;

    size_t nextBlock; // position of the next block in the swap file

    std::filesystem::path filepath;
    std::fstream file;

    Swap(const Swap &) = delete;
    Swap operator=(const Swap &) = delete;

  public:
    Swap(size_t numBlocks, size_t blockSize, std::string filename = "swap");
    ~Swap();

    size_t alloc();
    void free(size_t pos);
    void write(size_t pos, void *data, size_t size);
    void read(size_t pos, void *data, size_t size);
    void swap(size_t pos, void *data, size_t size);
};
