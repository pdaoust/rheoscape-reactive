#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/combine.hpp>
#include <sinks/arduino/gfx/types.hpp>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace rheo::sinks::arduino::gfx {

  template <typename TCanvas, typename TMask, typename THints>
  source_fn<GfxCommand<TCanvas, TMask>> canvasSink(
    source_fn<std::vector<GfxCommand<TCanvas, TMask>>> commandsSource,
    source_fn<THints> hintsSource
  ) {
    return combine(
      commandsSource,
      hintsSource,
      [](std::vector<GfxCommand<TCanvas>> commands, THints hints) {
        TCanvas canvas(display.width(), display.height());
        for (const GfxCommand& command : commands) {
          command(&canvas);
        }
        applyCanvasFunc(&display, canvas);
      }
    )
  }

}