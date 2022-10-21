#pragma once

#include <string>

int checkResult();
bool compareFiles(const std::string &path1, const std::string &path2);
bool compareAllFilesInFolders(const std::string &dir1, const std::string &dir2);
