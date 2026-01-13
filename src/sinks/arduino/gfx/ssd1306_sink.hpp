#pragma once

#include <functional>
#include <core_types.hpp>
#include <sinks/arduino/gfx/gfx_sink.hpp>
#include <Adafruit_SSD1306.h>

namespace rheo::sinks::arduino::gfx {
  template <typename TCanvas>
  using GfxCommand = std::function<void(TCanvas&)>;

  pull_fn ssd1306_sink(
    source_fn<std::vector<GfxCommand<GFXcanvas1>>> commands_source,
    source_fn<uint8_t> rotation_source,
    Adafruit_SSD1306& display
  ) {
    return _gfx_sink<Adafruit_SSD1306, GFXcanvas1>(
      commands_source,
      rotation_source,
      display,
      [](Adafruit_SSD1306& display, GFXcanvas1 canvas) {
        display.draw_bitmap(0, 0, canvas.get_buffer(), canvas.width(), canvas.height(), SSD1306_WHITE, SSD1306_BLACK);
        display.display();
      }
    );
  }

}