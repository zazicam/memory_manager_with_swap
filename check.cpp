#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// I took this function code from stackoverflow:
// https://stackoverflow.com/questions/6163611/compare-two-files
// It's rather slow for big files, but pretty simple to be sure it's correct.
bool compareFiles(const fs::path &path1, const fs::path &path2) {
    std::ifstream file1(path1, std::ios::binary | std::ios::ate);
    std::ifstream file2(path2, std::ios::binary | std::ios::ate);

    if (file1.fail() || file2.fail()) {
        return false; // file problem
    }

    if (file1.tellg() != file2.tellg()) {
        return false; // size mismatch
    }

    // seek back to beginning and use std::equal to compare contents
    file1.seekg(0, std::ifstream::beg);
    file2.seekg(0, std::ifstream::beg);
    bool res = std::equal(std::istreambuf_iterator<char>(file1.rdbuf()),
                          std::istreambuf_iterator<char>(),
                          std::istreambuf_iterator<char>(file2.rdbuf()));

    file1.close();
    file2.close();
    return res;
}

bool compareAllFilesInFolders(const fs::path &dir1, const fs::path &dir2) {
    bool result = true;
    for (const auto &entry : fs::directory_iterator(dir1)) {
        const fs::path filepath = entry.path();
        fs::path path(filepath);
        const fs::path filename = path.filename();
        const fs::path dir1_path = dir1 / filename;
        const fs::path dir2_path = dir2 / filename;
        bool cmp_res = compareFiles(dir1_path, dir2_path);
        std::cout << filename << " -> " << std::boolalpha << cmp_res
                  << std::endl;
        if (!cmp_res) {
            result = false;
        }
    }
    return result;
}

int checkResult() {
    std::cout << "This is very slow bytewise check" << std::endl;
    std::cout << "Start check" << std::endl;

    fs::path input_dir = "./input";
    fs::path output_dir = "./output";
    bool res = compareAllFilesInFolders(input_dir, output_dir);
    std::cout << "Finished check ... ";
    return res;
}

int main() {
    bool res = checkResult();
    if (res == false) {
        std::cout << "ERROR!" << std::endl;
        exit(1);
    } else {
        std::cout << "[OK]" << std::endl;
    }
    return 0;
}
