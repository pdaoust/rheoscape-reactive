#pragma once

#include <functional>
#include <core_types.hpp>
#include <sinks/arduino/adafruitGfx/adafruitGfxSink.hpp>
#include <Adafruit_SSD1306.h>

namespace rheo::sinks::arduino::adafruitGfx {
  template <typename TCanvas>
  using GfxCommand = std::function<void(TCanvas&)>;

  pull_fn ssd1306Sink(
    source_fn<std::vector<GfxCommand<GFXcanvas1>>> commandsSource,
    source_fn<uint8_t> rotationSource,
    Adafruit_SSD1306& display
  ) {
    return _adafruitGfxSink<Adafruit_SSD1306, GFXcanvas1>(
      commandsSource,
      rotationSource,
      display,
      [](Adafruit_SSD1306& display, GFXcanvas1 canvas) {
        display.drawBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height(), SSD1306_WHITE, SSD1306_BLACK);
        display.display();
      }
    );
  }

}