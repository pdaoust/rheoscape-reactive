#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>
#include <sinks/arduino/adafruitGfx/types.hpp>
#include <sinks/arduino/adafruitGfx/adafruitGfxSink.hpp>

namespace rheo::sinks::arduino::adafruitGfx {

  source_fn<std::vector<GfxCommand<GFXcanvas1>>> textSink(
    source_fn<std::string> textSource,
    source_fn<TextHints> hintsSource,
    source_fn<Coords> coordsSource
  ) {
    return zip3(
      textSource,
      hintsSource,
      coordsSource,
      (combine3_fn<std::vector<GfxCommand<GFXcanvas1>>, std::string, TextHints, Coords>)[](std::string text, TextHints hints, Coords coords) {
        std::vector<GfxCommand<GFXcanvas1>> commands = {
          [text, hints, coords](GFXcanvas1& canvas) {
            canvas.setCursor(coords.x, coords.y);
            canvas.setTextColor(hints.color);
            canvas.setTextSize(hints.size);
            canvas.setTextWrap(hints.wrap);
            canvas.write(text.c_str());
          }
        };
        return commands;
      }
    );
  }

}