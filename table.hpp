#pragma once

#include <vector> 
#include <string> 
#include <sstream> 
#include <filesystem>

namespace utils {
//---------------------------------------
// Convinient class for building tables
// It truncates content automaticaly to
// the column width.
//---------------------------------------

struct HorizontalRule {
	void unused() {}
}; 

extern const HorizontalRule& hr;
 
class Table { 
    std::vector<size_t> columnWidth; 
    std::ostringstream buf; 
    size_t column; 
    size_t row; 
public: 
    explicit Table(std::vector<size_t>&& columnWidth);
 
    Table(const Table&) = delete; 
    Table operator=(const Table&) = delete; 
    Table operator=(Table) = delete; 

	void ProcessStream(const std::ostringstream& oss); 
 
    template<typename T> 
    Table& operator<<(T&& smth) { 
        std::ostringstream oss; 
        oss << smth; 
		ProcessStream(oss);
        return *this; 
    } 

    Table& operator<<(HorizontalRule hr); 
 
    friend std::ostream& operator<<(std::ostream& out, const Table& table) { 
        return out << table.buf.str(); 
    } 
}; 

} // namespace utils
