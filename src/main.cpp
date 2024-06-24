#include <Arduino.h>
#include <types/au_noio.hpp>
#include <rheoscape_everything.hpp>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  auto measured = constant(16.0f);
  auto clock = map_(arduinoMillisClock(), stripChrono<arduino_millis_clock, typename arduino_millis_clock::duration::rep, typename arduino_millis_clock::duration::period>());
  auto blup = liftToQuantity<au::Kelvins, float>(exponentialMovingAverage<float>(clock, (unsigned long)1600));
  auto flob = 13.0f * au::Kelvins{};
  
  auto pidController = pid_(constant(300.0f), constant(300.0f), arduinoMillisClock(), constant(PidWeights(0.0f, 0.0f, 0.0f)));
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}