#include <unity.h>
#include <rheoscape_everything.hpp>
#include <fmt/format.h>
#include <types/au_all_units_noio.hpp>
using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_a_big_fat_pipe_actually_works() {
  //logging::register_subscriber([](uint8_t log_level, const char* topic, const char* message) {
  //  printf(fmt::format("[{}:{}] {}\n", LOG_LEVEL_LABEL(log_level), topic, message).c_str()); });

  auto temp_and_hum_state = rheo::State<rheo::Fallible<std::tuple<float, float>, int>>();
  auto temp_and_hum = temp_and_hum_state.get_source_fn();
  auto clock = from_clock<mock_clock_ulong_millis>();
  auto temp_and_hum_smooth = temp_and_hum
    | log_errors<std::tuple<float, float>, int>([](int error) {
      return fmt::format("sht2x error {}", error).c_str();
    }, "sht2x")
    | make_infallible<std::tuple<float, float>, int>()
    | cache<std::tuple<float, float>>()
    | throttle<std::tuple<float, float>>(clock, mock_clock_ulong_millis::duration(250))
    // | lift_to_tuple_left<float>(
    //   exponential_moving_average<float>(
    //     clock,
    //     mock_clock_ulong_millis::duration(1000),
    //     map_chrono_to_scalar<unsigned long, typename mock_clock_ulong_millis::duration>
    //   )
    // )
    // | lift_to_tuple_right<float>(
    //   exponential_moving_average<float>(
    //     clock,
    //     mock_clock_ulong_millis::duration(1000),
    //     map_chrono_to_scalar<unsigned long, typename mock_clock_ulong_millis::duration>
    //   )
    // )
    ;

  // auto pull_temp_and_hum = temp_and_hum_smooth
  //   | foreach([](std::tuple<float, float> value) {
  //     logging::debug("chart", fmt::format("writing temp {} and hum {}", std::get<0>(value), std::get<1>(value)).c_str());
  //   });
  auto pull_temp_and_hum = temp_and_hum_smooth(
    [](std::tuple<float, float> value) {
      TEST_ASSERT(std::get<0>(value) == std::get<1>(value));
    }
  );

  for (int i = 0; i < 10; i++) {
    temp_and_hum_state.set(rheo::Fallible<std::tuple<float, float>, int>(std::make_tuple((float)i, (float)i)));
    mock_clock_ulong_millis::tick();
    pull_temp_and_hum();
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_a_big_fat_pipe_actually_works);
  UNITY_END();
}