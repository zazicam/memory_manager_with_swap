#pragma once

namespace utils {

//--------------------------------------------------------------
// Allows to measure time duration
//--------------------------------------------------------------
class Timer {
    std::chrono::steady_clock::time_point startTime;

  public:
    void start();
    std::chrono::milliseconds elapsed();
};

extern Timer timer;

//--------------------------------------------------------------
// class hh_mm_ss - allows to print time in HH:MM:SS.MS format
//--------------------------------------------------------------
class hh_mm_ss {
    friend std::ostream &operator<<(std::ostream &out, const hh_mm_ss &time);

    std::chrono::hours hours;
    std::chrono::minutes minutes;
    std::chrono::seconds seconds;
    std::chrono::milliseconds milliseconds;

  public:
    explicit hh_mm_ss(std::chrono::milliseconds msec);
};
//--------------------------------------------------------------

} // namespace utils
