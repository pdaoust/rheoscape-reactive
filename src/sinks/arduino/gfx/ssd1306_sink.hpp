#pragma once
#if __has_include(<Adafruit_SSD1306.h>)

#include <types/core_types.hpp>
#include <Adafruit_SSD1306.h>

namespace rheoscape::sinks::arduino::gfx {

  namespace detail {

    struct ssd1306_push_handler {
      Adafruit_SSD1306& display;

      RHEOSCAPE_CALLABLE void operator()(std::vector<GfxCommand<Adafruit_GFX>> commands) const {
        GFXcanvas1 canvas(display.width(), display.height(), false);
        display.clearDisplay();
        for (auto& command : commands) {
          command(display);
        }
        //display.drawBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height(), SSD1306_WHITE, SSD1306_BLACK);
        display.display();
      }
    };

    struct ssd1306_sink_binder {
      Adafruit_SSD1306& display;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, std::vector<GfxCommand<Adafruit_GFX>>>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(ssd1306_push_handler{display});
      }
    };

  } // namespace detail

  auto ssd1306_sink(Adafruit_SSD1306& display) {
    return detail::ssd1306_sink_binder{display};
  }

}
#endif // __has_include(<Adafruit_SSD1306.h>)
