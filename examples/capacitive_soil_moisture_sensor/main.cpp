#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <rheoscape.hpp>

using namespace rheoscape;
using namespace rheoscape::helpers;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::sources::arduino;
using namespace rheoscape::sinks::arduino;
using namespace rheoscape::sinks::arduino::gfx;

#define sensor_pin 26

static pull_fn refresh_display;
static pull_fn output_to_serial;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

extern "C" void __attribute__((weak)) panic(const char *fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  Serial.flush();
  pinMode(25, OUTPUT);
  while (1) {
    // Rapid blink = panic
    digitalWrite(25, HIGH);
    delay(50);
    digitalWrite(25, LOW);
    delay(50);
  }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting..."); Serial.flush();
    Serial.println("Starting I2C..."); Serial.flush();
    Wire.setSDA(8);
    Wire.setSCL(9);
    Wire.begin();
    Serial.println("I2C done."); Serial.flush();
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
    }
    display.display();
    display.clearDisplay();
    display.setCursor(32, 32);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setTextSize(2);
    display.print("Hello!");
    display.display();
    delay(2000);
    display.clearDisplay();
    display.setCursor(32, 32);
    display.print("........");
    pinMode(sensor_pin, INPUT);

    auto sensor = analog_pin_source(sensor_pin)
        | normalize(constant(Range(350, 800)), constant(Range(100, 0)));

    refresh_display = sensor
        | map([](int v) {
            return std::vector<GfxCommand<Adafruit_GFX>> {
                [v](Adafruit_GFX& canvas) {
                    canvas.setTextColor(SSD1306_WHITE);
                    canvas.setTextSize(3);
                    // Using the 0.49" SSD1306 display with 64x32px,
                    // it only displays columns 32-95 and rows 0-32.
                    canvas.setCursor(32, 32);
                    canvas.print(fmt::format("{}", v).c_str());
                }
            };
        })
        | ssd1306_sink(display);

    output_to_serial = sensor
        | map([](int v) {
            return fmt::format("Reading: {}\n", v);
        })
        | serial_string_sink();
}

void loop() {
    output_to_serial();
    refresh_display();
    delay(500);
}
