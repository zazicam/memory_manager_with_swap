#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "check.hpp"
#include "memory_pool.hpp"

namespace fs = std::filesystem;

const size_t max_block_size = 1024;
MemoryPool memoryPool(10, max_block_size);

void CopyFile(const std::string &filename1, const std::string &filename2) {
    std::ifstream fin(filename1, std::ios::binary);
    std::ofstream fout(filename2, std::ios::binary);

    if (!fin) {
        std::cout << "Can't open file " << filename1 << std::endl;
    }
    if (!fout) {
        std::cout << "Can't open file " << filename2 << std::endl;
    }

    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);

    size_t readed = 0;

    while (readed < filesize) {
        size_t size = 1 + rand() % max_block_size;
        size = std::min(size, filesize - readed);

        MemoryBlock mb = memoryPool.getBlock();
        char *buf = mb.getPtr<char>();

        fin.read(buf, size);
        readed += size;
        mb.unlock();

        mb.lock();
        fout.write(buf, size);
        mb.unlock();
        mb.free();
    }

    fin.close();
    fout.close();
}

void CopyAllFiles(const std::string &dir1, const std::string &dir2) {
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
    CopyAllFiles(input_dir, output_dir);

    std::cout << "Copying completed" << std::endl;

    bool res = checkResult();
    std::cout << "Result: " << std::boolalpha << res << std::endl;
    return 0;
}
