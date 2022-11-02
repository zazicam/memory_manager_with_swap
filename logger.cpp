#include <iostream>
#include <mutex>
#include <sstream>

#include "logger.hpp"

Logger::Logger(std::ostream &str) : output(str) {}

void Logger::Flush() { output.flush(); }

void Logger::Lock() { mutex.lock(); }

void Logger::Unlock() {
    output << str();
    str("");
    Flush();
    mutex.unlock();
}

Logger logger{std::cout};
