#pragma once

#include <iostream>

class hh_mm_ss {
	std::chrono::hours hours;
	std::chrono::minutes minutes;
	std::chrono::seconds seconds;
	std::chrono::milliseconds milliseconds;
public:
	hh_mm_ss(std::chrono::milliseconds msec) {
		std::cout << msec.count() << std::endl;
		hours = std::chrono::duration_cast<std::chrono::hours>(msec);
		msec -= hours;
		minutes = std::chrono::duration_cast<std::chrono::minutes>(msec);
		msec -= minutes;
		seconds = std::chrono::duration_cast<std::chrono::seconds>(msec);
		milliseconds = msec - seconds;
	}
	
	friend std::ostream& operator<<(std::ostream& out, const hh_mm_ss& time) {
		return out << time.hours.count() << ":" 
			<< std::setfill('0') 
			<< std::setw(2) << time.minutes.count() << ":" 
			<< std::setw(2) << time.seconds.count() << "." 
			<< std::setw(3) << time.milliseconds.count() 
			<< std::setfill(' ');
	}
};
