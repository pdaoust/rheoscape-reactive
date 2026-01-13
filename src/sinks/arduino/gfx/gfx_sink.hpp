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
  source_fn<GfxCommand<TCanvas, TMask>> canvas_sink(
    source_fn<std::vector<GfxCommand<TCanvas, TMask>>> commands_source,
    source_fn<THints> hints_source
  ) {
    return combine(
      [](std::vector<GfxCommand<TCanvas>> commands, THints hints) {
        TCanvas canvas(display.width(), display.height());
        for (const GfxCommand& command : commands) {
          command(&canvas);
        }
        apply_canvas_func(&display, canvas);
      },
      commands_source,
      hints_source
    )
  }

}