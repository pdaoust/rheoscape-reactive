#pragma once

#include <chrono>

namespace rheo {

  template <typename Rep, typename Period = std::milli>
  class mock_clock {
    private:
      static Rep _timestamp;
      mock_clock() = delete;
      ~mock_clock() = delete;
      mock_clock(mock_clock<Rep, Period> const&) = delete;

    public:
      typedef Rep rep;
      typedef Period period;
      typedef std::chrono::duration<rep, period> duration;
      typedef std::chrono::time_point<mock_clock<Rep, Period>> time_point;

      static constexpr bool is_steady = true;

      static time_point now() noexcept {
        return time_point(duration(_timestamp));
      }

      static void setTime(rep timestamp) {
        _timestamp = timestamp;
      }

      static void tick() {
        _timestamp ++;
      }
  };

}