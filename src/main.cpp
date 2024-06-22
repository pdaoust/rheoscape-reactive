#include <Arduino.h>
#include <types/au_noio.hpp>
#include <rheoscape_everything.hpp>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  auto blup = exponentialMovingAverage_(constant(16.0f), arduinoMillisClockFloat(), make_arduino_ms(1600.0f));
  auto blop = au::seconds(1500) / au::seconds(500.0f);

  auto prevTime = au::seconds(1500);
  auto nextTime = au::seconds(1600);
  auto timeConstant = au::seconds(3);
  auto timeDelta = nextTime - prevTime;
  auto alpha = 1 - pow(M_E, -timeDelta / au::seconds(3));
  auto prevValue = au::kelvins_pt(32.0f);
  auto nextValue = au::kelvins_pt(34.4f);
  auto integrated = prevValue + alpha * (nextValue - prevValue);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}