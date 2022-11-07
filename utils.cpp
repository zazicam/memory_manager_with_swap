#include <fstream>
#include <sstream>

#include "utils.hpp"

namespace utils {
//----------------------------------------------------
// To show progress of reading and writing threads 
//----------------------------------------------------
using std::setw;

void ProgressBar(size_t len, size_t total, size_t done, std::ostringstream& out) {
	size_t numBars = 0;
	if(total > 0) {
		numBars = std::lround(len * static_cast<double>(done) / total);	
	}
	out << "[" << std::string(numBars, '=') << std::string(len - numBars, ' ') << "]";
}

void ShowFileProgress(const fs::path& filename, std::shared_ptr<Progress> progress, std::ostringstream& out) {
	// truncate filename if it's long 
	size_t filenameWidth = 30;
	std::ostringstream tmp;
	tmp << filename;
	out << std::left << std::setw(filenameWidth) << tmp.str().substr(0, filenameWidth-1) << " ";
	ProgressBar(23, progress->size, progress->read, out);
	out << "  ";
	ProgressBar(23, progress->size, progress->write, out);
	out << std::endl;
}

void ShowProgress(
    const std::map<fs::path, std::shared_ptr<Progress>>& progressMap) {
	std::ostringstream out;
	out << std::left << setw(30) << "filename" << " "
		<< setw(23) << "read" << "  "
		<< setw(23) << "write" << std::endl;
	for(const auto& [filename, progress] : progressMap) {
		ShowFileProgress(filename, progress, out);
	}
	//std::cout << out.str() << std::endl;
    puts(out.str().c_str());
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