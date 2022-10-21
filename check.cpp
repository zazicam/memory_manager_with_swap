#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>

// I took this function code from stackoverflow: 
// https://stackoverflow.com/questions/6163611/compare-two-files
// It's rather slow for big files, but pretty simple to be sure it's correct.
bool compareFiles(const std::string& path1, const std::string& path2) {
	std::ifstream file1(path1, std::ios::binary | std::ios::ate);
	std::ifstream file2(path2, std::ios::binary | std::ios::ate);

	if (file1.fail() || file2.fail()) {
		return false; //file problem
	}

	if (file1.tellg() != file2.tellg()) {
		return false; //size mismatch
	}

	//seek back to beginning and use std::equal to compare contents
	file1.seekg(0, std::ifstream::beg);
	file2.seekg(0, std::ifstream::beg);
	bool res = std::equal(std::istreambuf_iterator<char>(file1.rdbuf()),
			std::istreambuf_iterator<char>(),
			std::istreambuf_iterator<char>(file2.rdbuf()));
	
	file1.close();
	file2.close();
	return res;
}


bool compareAllFilesInFolders(const std::string& dir1, const std::string& dir2) {
	namespace fs = std::filesystem;
	bool result = true;
	for (const auto & entry : fs::directory_iterator(dir1)) {
		const std::string filepath = entry.path();
		fs::path path(filepath);
		const std::string filename = path.filename();
		const std::string dir1_path = dir1 + "/" + filename; 	
		const std::string dir2_path = dir2 + "/" + filename; 	
		bool cmp_res = compareFiles(dir1_path, dir2_path);
		std::cout << filename << " -> " << std::boolalpha 
		          << cmp_res << std::endl;
		if(!cmp_res) {
			result = false;
		}
	}
	return result;
}

int checkResult() {
	std::cout << "Start check" << std::endl;

	std::string input_dir = "./input";
	std::string output_dir = "./output";
	bool res = compareAllFilesInFolders(input_dir, output_dir);
	if(res) {
		std::cout << "Ok" << std::endl;
	} else {
		std::cout << "There are errors!" << std::endl;
	}
	
	std::cout << "Finished check" << std::endl;
	return res;
}
