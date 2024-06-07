#ifndef RHEOSCAPE_RUNNABLE_HPP
#define RHEOSCAPE_RUNNABLE_HPP

#include <chrono>
#include <functional>
#include <TSValue.hpp>

class Runnable {
  private:
    std::function<void(mono_time_point)> _run;

  public:
    Runnable(std::function<void(mono_time_point)> run)
    : _run(run)
    { }

    void run(mono_time_point ts) {
      _run(ts);
    }
};

#endif