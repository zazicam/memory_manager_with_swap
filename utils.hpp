#pragma once

#include <iostream>
#include <filesystem>
#include <atomic>
#include <map>

namespace fs = std::filesystem;

//----------------------------------------------------
// To show progress of reading and writing threads 
//----------------------------------------------------
struct Progress {
	std::atomic<size_t> size = 0;
	std::atomic<size_t> read = 0;
	std::atomic<size_t> write = 0;
};

//--------------------------------------------------------------
// class hh_mm_ss - allows to print time in HH:MM:SS.MS format
//--------------------------------------------------------------
class hh_mm_ss {
	friend std::ostream& operator<<(std::ostream& out, const hh_mm_ss& time); 

	std::chrono::hours hours;
	std::chrono::minutes minutes;
	std::chrono::seconds seconds;
	std::chrono::milliseconds milliseconds;
public:
	explicit hh_mm_ss(std::chrono::milliseconds msec); 
};

//--------------------------------------------------------------
// help functions 
//--------------------------------------------------------------
size_t FileSize(std::ifstream& fin);
size_t CheckArgsAndGetMemorySize(int argc, char** argv); 
void CheckIfDirectoryExists(const fs::path& dir); 
void CreateDirectoryIfNotExists(const fs::path& dir);
void ShowProgress(const std::map<fs::path, std::shared_ptr<Progress>> progressMap); 

