#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>
#include <sinks/arduino/adafruitGfx/types.hpp>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace rheo::sinks::arduino::adafruitGfx {

  template <typename TDisplay, typename TCanvas>
  pull_fn _adafruitGfxSink(
    source_fn<std::vector<GfxCommand<TCanvas>>> commandsSource,
    source_fn<uint8_t> rotationSource,
    TDisplay& display,
    std::function<void(TDisplay&, TCanvas)> applyCanvasFunc 
  ) {
    return zip(
      commandsSource,
      rotationSource,
      [&display](std::vector<GfxCommand<TCanvas>> commands, uint8_t rotation) {
        display.setRotation(rotation);
        TCanvas canvas(display.width(), display.height());
        for (const GfxCommand& command : commands) {
          command(&canvas);
        }
        applyCanvasFunc(&display, canvas);
      }
    )
  }

}