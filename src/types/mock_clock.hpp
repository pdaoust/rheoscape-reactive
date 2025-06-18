#pragma once

#include <chrono>

namespace rheo {

  template <typename Rep, typename Period>
  class mock_clock {
    private:
      mock_clock() = delete;
      ~mock_clock() = delete;
      mock_clock(mock_clock<Rep, Period> const&) = delete;

    public:
      static Rep _timestamp;
      typedef Rep rep;
      typedef Period period;
      typedef std::chrono::duration<rep, period> duration;
      typedef std::chrono::time_point<mock_clock<Rep, Period>> time_point;

      static constexpr bool is_steady = true;

      static time_point now() noexcept;

      static void setTime(Rep timestamp);

      static void tick(Rep ticks = 1);
  };

  template <typename Rep, typename Period>
  typename mock_clock<Rep, Period>::rep mock_clock<Rep, Period>::_timestamp;

  template <typename Rep, typename Period>
  typename mock_clock<Rep, Period>::time_point mock_clock<Rep, Period>::now() noexcept {
    return time_point(duration(_timestamp));
  }

  template <typename Rep, typename Period>
  void mock_clock<Rep, Period>::setTime(Rep timestamp) {
    _timestamp = timestamp;
  }

  template <typename Rep, typename Period>
  void mock_clock<Rep, Period>::tick(Rep ticks) {
    _timestamp += ticks;
  }

  using mock_clock_ulong_millis = mock_clock<unsigned long, std::milli>;
}