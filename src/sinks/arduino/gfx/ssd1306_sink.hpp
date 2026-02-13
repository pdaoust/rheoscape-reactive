#pragma once

#include <functional>
#include <core_types.hpp>
#include <Adafruit_SSD1306.h>

namespace rheo::sinks::arduino::gfx {

  pullable_sink_fn<std::vector<GfxCommand<Adafruit_GFX>>> ssd1306_sink(
    Adafruit_SSD1306& display
  ) {
    struct SinkBinder {
      Adafruit_SSD1306& display;

      RHEO_CALLABLE pull_fn operator()(source_fn<std::vector<GfxCommand<Adafruit_GFX>>> source) const {
        struct PushHandler {
          Adafruit_SSD1306& display;

          RHEO_CALLABLE void operator()(std::vector<GfxCommand<Adafruit_GFX>> commands) const {
            GFXcanvas1 canvas(display.width(), display.height(), false);
            display.clearDisplay();
            for (auto& command : commands) {
              command(display);
            }
            //display.drawBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height(), SSD1306_WHITE, SSD1306_BLACK);
            display.display();
          }
        };

        return source(PushHandler{display});
      }
    };

    return SinkBinder{display};
  }

}
