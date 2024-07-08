#include <Arduino.h>
#include <types/au_noio.hpp>
#include <rheoscape_everything.hpp>

using namespace rheo;

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  auto measured = constant(au::kelvins_pt(16.0f));
  auto clock = clockSource<arduino_millis_clock>();
  auto auClock = map(
    map(clock, convertTimePointToDuration<arduino_millis_clock>()),
    convertToQuantity<typename arduino_millis_clock::duration>()
  );
  auto avg = exponentialMovingAverage<
    au::QuantityPoint<au::Kelvins, float>,
    typename arduino_millis_clock::time_point,
    typename arduino_millis_clock::duration,
    float
  >(
    measured,
    clock,
    arduino_millis_clock::duration((unsigned long)1400)
  );

  auto scalarClock = constant((unsigned long)1);
  auto scalarMeasured = constant(16.0f);
  auto scalarAvg = exponentialMovingAverage<
    float,
    unsigned long,
    unsigned long,
    float
  >(scalarMeasured, scalarClock, (unsigned long)1400);

  //auto pidController = pid(constant(300.0f), constant(300.0f), arduinoMillisClock(), constant(PidWeights(0.0f, 0.0f, 0.0f)));

  auto gg = std::chrono::steady_clock::now();
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}