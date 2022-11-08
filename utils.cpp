#include <fstream>
#include <sstream>

#include "utils.hpp"
#include "table.hpp"

namespace utils {
//----------------------------------------------------
// To show progress of reading and writing threads 
//----------------------------------------------------
using std::setw;

std::string ProgressBar(size_t len, size_t total, size_t done) {
	size_t percent = 0;
	size_t numBars = 0;
	if(total > 0) {
		percent = std::lround(100 * static_cast<double>(done) / total);	
		numBars = std::lround(len * static_cast<double>(done) / total);	
	}
	std::string bar = std::string(numBars, '=') + std::string(len - numBars, ' ');
	std::string value = std::to_string(percent) + "%";
	bar.replace(len/2 - value.length()/2, value.size(), value);
	return bar;
}

void ShowProgress(const std::map<fs::path, std::shared_ptr<Progress>>& progressMap) {
	Table table({25, 10, 20, 20});
	table << hr;
	table << "Filename" << "Size" << "Read" << "Write" << hr;

	size_t totalSize = 0;
	size_t totalRead = 0;
	size_t totalWrite = 0;
	for(const auto& [filename, progress] : progressMap) {
		
		table << filename 
			<< HumanReadable{progress->size}
			<< ProgressBar(18, progress->size, progress->read)
			<< ProgressBar(18, progress->size, progress->write);
	
		totalSize += progress->size;
		totalRead += progress->read;
		totalWrite += progress->write;
	}
	table << hr;

	std::cout << table;
	std::cout << "Total progress [Size: " << HumanReadable{totalSize} << ", "
	<< "Read: " << HumanReadable{totalRead} << ", "
	<< "Write: " << HumanReadable{totalWrite} << "]\n";
}

//--------------------------------------------------------------
// class hh_mm_ss - allows to print time in HH:MM:SS.MS format
//--------------------------------------------------------------
hh_mm_ss::hh_mm_ss(std::chrono::milliseconds msec) {
	hours = std::chrono::duration_cast<std::chrono::hours>(msec);
	msec -= hours;
	minutes = std::chrono::duration_cast<std::chrono::minutes>(msec);
	msec -= minutes;
	seconds = std::chrono::duration_cast<std::chrono::seconds>(msec);
	milliseconds = msec - seconds;
}
	
std::ostream& operator<<(std::ostream& out, const hh_mm_ss& time) {
	return out << time.hours.count() << ":" 
		<< std::setfill('0') 
		<< std::setw(2) << time.minutes.count() << ":" 
		<< std::setw(2) << time.seconds.count() << "." 
		<< std::setw(3) << time.milliseconds.count() 
		<< std::setfill(' ');
}

//--------------------------------------------------------------
// help functions 
//--------------------------------------------------------------
size_t FileSize(std::ifstream& fin) {
    fin.seekg(0, std::ios::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);
	return filesize;
}

void CheckIfDirectoryExists(const fs::path& dir) {
    if (!fs::exists(dir)) {
        std::cerr << "Input folder " << dir << " does not exist!"
                  << std::endl;
        exit(1);
    }
}

void CreateDirectoryIfNotExists(const fs::path& dir) {
	if(!fs::exists(dir) && !fs::create_directory(dir)) {
		std::cerr << "Output folder " << dir << " does not exists"
			" and can't create it!" << std::endl;
		exit(1);
	}
}

size_t CheckArgsAndGetMemorySize(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "This program needs an integer argument." << std::endl;
        std::cout << "Usage: " << std::endl;
        std::cout << "\t" << argv[0] << " [size of RAM in Mb]" << std::endl;
        exit(1);
    }

    int memorySizeMb = 0;
    std::istringstream iss(argv[1]);
    iss >> memorySizeMb;
    if (iss.fail() || !iss.eof() || memorySizeMb <= 0) {
        std::cout << "[size of RAM in Mb]: wrong input!" << std::endl;
        exit(1);
    }
	return memorySizeMb;
}

void ClearConsole() { system("@cls||clear"); }

} // namespace utils
