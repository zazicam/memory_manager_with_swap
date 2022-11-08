#include <chrono>
#include <ostream>
#include <iomanip>

#include "timer.hpp"

namespace utils {

//--------------------------------------------------------------
// Allows to measure time duration 
//--------------------------------------------------------------
Timer timer{}; // global object to use

void Timer::start() {
	startTime = std::chrono::steady_clock::now();
}

std::chrono::milliseconds Timer::elapsed() {
	std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(endTime- startTime);
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

} // namespace utils
