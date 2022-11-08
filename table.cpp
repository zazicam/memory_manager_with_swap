#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "table.hpp"

namespace fs = std::filesystem;

namespace utils {
//---------------------------------------
// Convinient class for building tables
// It truncates content automaticaly to
// the column width.
//---------------------------------------

fs::path TruncatePath(const fs::path &path, size_t width) {
    return fs::path{path.u32string().substr(0, width)};
}

const HorizontalRule &hr = HorizontalRule{};

Table::Table(std::vector<size_t> &&columnWidth)
    : columnWidth(columnWidth), column(0), row(0) {}

void Table::ProcessStream(const std::ostringstream &oss) {
    std::string value = oss.str();

    // fs::path store a string platform independent
    // useful for truncate a string regardless of
    // it's internal format (Windows or Linux)
    fs::path uniStr{value};
    size_t valueLen = uniStr.u32string().length();

    size_t width = columnWidth.at(column) - 2;
    if (valueLen > width) {
        value = TruncatePath(uniStr, width).string();
    } else {
        value += std::string(width - valueLen, ' ');
    }
    if (column == 0)
        buf << "|";
    buf << " " << value << " |";
    ++column;
    if (column >= columnWidth.size()) {
        column = 0;
        ++row;
        buf << "\n";
    }
}

Table &Table::operator<<(HorizontalRule hr) {
    hr.unused();
    for (const auto &width : columnWidth) {
        buf << '+' << std::string(width, '-');
    }
    buf << "+\n";
    ++row;
    return *this;
}

} // namespace utils
