#pragma once

#include <chrono>
#include <functional>
#include <TaggedValue.hpp>

namespace rheo {

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

}