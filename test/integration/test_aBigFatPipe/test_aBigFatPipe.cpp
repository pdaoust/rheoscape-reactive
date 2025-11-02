#include <unity.h>
#include <rheoscape_everything.hpp>
#include <fmt/format.h>
#include <types/au_all_units_noio.hpp>
using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_aBigFatPipeActuallyWorks() {
  //logging::registerSubscriber([](uint8_t logLevel, const char* topic, const char* message) {
  //  printf(fmt::format("[{}:{}] {}\n", LOG_LEVEL_LABEL(logLevel), topic, message).c_str()); });

  auto tempAndHumState = rheo::State<rheo::Fallible<std::tuple<float, float>, int>>();
  auto tempAndHum = tempAndHumState.sourceFn();
  auto clock = fromClock<mock_clock_ulong_millis>();
  auto tempAndHumSmooth = tempAndHum
    | logErrors<std::tuple<float, float>, int>([](int error) {
      return fmt::format("sht2x error {}", error).c_str();
    }, "sht2x")
    | makeInfallible<std::tuple<float, float>, int>()
    | cache<std::tuple<float, float>>()
    | throttle<std::tuple<float, float>>(clock, mock_clock_ulong_millis::duration(250))
    // | liftToTupleLeft<float>(
    //   exponentialMovingAverage<float>(
    //     clock,
    //     mock_clock_ulong_millis::duration(1000),
    //     mapChronoToScalar<unsigned long, typename mock_clock_ulong_millis::duration>
    //   )
    // )
    // | liftToTupleRight<float>(
    //   exponentialMovingAverage<float>(
    //     clock,
    //     mock_clock_ulong_millis::duration(1000),
    //     mapChronoToScalar<unsigned long, typename mock_clock_ulong_millis::duration>
    //   )
    // )
    ;
  
  // auto pullTempAndHum = tempAndHumSmooth
  //   | foreach([](std::tuple<float, float> value) {
  //     logging::debug("chart", fmt::format("writing temp {} and hum {}", std::get<0>(value), std::get<1>(value)).c_str());
  //   });
  auto pullTempAndHum = tempAndHumSmooth(
    [](std::tuple<float, float> value) {
      TEST_ASSERT(std::get<0>(value) == std::get<1>(value));
    }
  );

  for (int i = 0; i < 10; i++) {
    tempAndHumState.set(rheo::Fallible<std::tuple<float, float>, int>(std::make_tuple((float)i, (float)i)));
    mock_clock_ulong_millis::tick();
    pullTempAndHum();
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_aBigFatPipeActuallyWorks);
  UNITY_END();
}