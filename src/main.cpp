#include <Arduino.h>
#include <types/au_noio.hpp>
#include <rheoscape_everything.hpp>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  auto blup = exponentialMovingAverage_(constant(16.0f), arduinoMillisClock(), (unsigned long)1600);
  auto blop = au::seconds(1500) / au::seconds(500.0f);
  
  auto pidController = pid_(constant(300.0f), constant(300.0f), arduinoMillisClock(), constant(PidWeights(0.0f, 0.0f, 0.0f)));
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}