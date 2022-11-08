#pragma once

#include <iostream>
#include <sstream>
#include <filesystem>
#include <atomic>
#include <map>
#include <cmath>

#include "timer.hpp"
#include "table.hpp"

namespace fs = std::filesystem;

namespace utils {
//----------------------------------------------------
// To show progress of reading and writing threads
//----------------------------------------------------
struct Progress {
    std::atomic<size_t> size = 0;
    std::atomic<size_t> read = 0;
    std::atomic<size_t> write = 0;
};

//--------------------------------------------------------------
// To show size of file or memory in human readable format
// Copied from here (with small changes):
// https://en.cppreference.com/w/cpp/filesystem/file_size
//--------------------------------------------------------------
struct HumanReadable {
    size_t size;
	bool showInBytes;
	HumanReadable(size_t size, bool showInBytes = false) 
		: size(size), showInBytes(showInBytes) {}
  private:
    friend std::ostream &operator<<(std::ostream &os, HumanReadable hr) {
		std::ostringstream out;
        int i{};
        double mantissa = static_cast<double>(hr.size);
        for (; mantissa >= 1024.; mantissa /= 1024., ++i) {
        }
        mantissa = std::ceil(mantissa * 10.) / 10.;
        out << mantissa << " " << "BKMGTPE"[i];
		if(i>0) {
			out << "B";
			if(hr.showInBytes) 
				out << " (" << hr.size << ')';
		}
		return os << out.str();
    }
};

//--------------------------------------------------------------
// help functions
//--------------------------------------------------------------
size_t FileSize(std::ifstream &fin);
size_t CheckArgsAndGetMemorySize(int argc, char **argv);
void CheckIfDirectoryExists(const fs::path &dir);
void CreateDirectoryIfNotExists(const fs::path &dir);
void ShowProgress(
    const std::map<fs::path, std::shared_ptr<Progress>>& progressMap);

void ClearConsole();
fs::path TruncatePath(const fs::path& path, size_t width); 

} // namespace utils
