#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>
#include <sinks/arduino/gfx/types.hpp>
#include <sinks/arduino/gfx/gfx_sink.hpp>

namespace rheo::sinks::arduino::gfx {

  source_fn<std::vector<GfxCommand<GFXcanvas1>>> text_sink(
    source_fn<std::string> text_source,
    source_fn<TextHints> hints_source,
    source_fn<Coords> coords_source
  ) {
    return operators::combine(text_source, hints_source, coords_source)
      | operators::map_tuple([](std::string text, TextHints hints, Coords coords) {
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
        });
  }

}