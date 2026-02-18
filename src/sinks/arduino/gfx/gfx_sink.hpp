#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/combine.hpp>
#include <sinks/arduino/gfx/types.hpp>
#include <Arduino.h>
#include <Adafruit_GFX.h>

namespace rheoscape::sinks::arduino::gfx {

  // template <typename TCanvas, typename TMask, typename THints>
  // source_fn<GfxCommand<TCanvas>> canvas_sink(
  //   source_fn<std::vector<GfxCommand<TCanvas>>> commands_source,
  //   source_fn<THints> hints_source
  // ) {
  //   return combine(
  //     [](std::vector<GfxCommand<TCanvas>> commands, THints hints) {
  //       TCanvas canvas(display.width(), display.height());
  //       for (const GfxCommand<TCanvas>& command : commands) {
  //         command(&canvas);
  //       }
  //       apply_canvas_func(&display, canvas);
  //     },
  //     commands_source,
  //     hints_source
  //   )
  // }

}