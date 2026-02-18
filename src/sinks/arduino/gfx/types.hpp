#pragma once

#include <functional>
#include <Adafruit_GFX.h>

namespace rheoscape::sinks::arduino::gfx {

  struct Coords {
    int16_t x;
    int16_t y;

    friend Coords operator+(Coords lhs, const Coords& rhs) {
      return Coords {
        .x = static_cast<int16_t>(lhs.x + rhs.x),
        .y = static_cast<int16_t>(lhs.y + rhs.y)
      };
    }

    friend Coords operator-(Coords lhs, const Coords& rhs) {
      return Coords {
        .x = static_cast<int16_t>(lhs.x - rhs.x),
        .y = static_cast<int16_t>(lhs.y - rhs.y)
      };
    }
  };

  struct Dimensions {
    uint16_t w;
    uint16_t h;
  };

  // template <typename TCanvas, typename TMask>
  // using GfxCommand = std::function<void(TCanvas&, TMask&)>;
  template <typename TCanvas>
  using GfxCommand = std::function<void(TCanvas&)>;

  struct DisplayHints {
    uint8_t rotation;
  };

  struct ShapeHints {
    Coords origin;
    std::optional<uint16_t> border_color;
    std::optional<uint16_t> fill_color;
  };

  struct CircleHints : ShapeHints {
    int16_t r;
  };

  struct RectHints : ShapeHints {
    Dimensions dims;
    int8_t corner_r;
  };

  struct TextHints {
    uint16_t color;
    uint8_t size;
    bool wrap;
  };

}