#pragma once

#include <functional>
#include <Adafruit_GFX.h>

namespace rheo::sinks::arduino::gfx {

  struct Coords {
    int16_t x;
    int16_t y;

    friend Coords operator+(Coords lhs, const Coords& rhs) {
      return Coords {
        x: lhs.x + rhs.x,
        y: lhs.y + rhs.y
      };
    }

    friend Coords operator-(Coords lhs, const Coords& rhs) {
      return Coords {
        x: lhs.x - rhs.x,
        y: lhs.y - rhs.y
      };
    }
  };

  struct Dimensions {
    uint16_t w;
    uint16_t h;
  };

  template <typename TCanvas, typename TMask>
  using GfxCommand = std::function<void(TCanvas&, TMask&)>;

  struct DisplayHints {
    uint8_t rotation;
  };

  struct ShapeHints {
    Coords origin;
    std::optional<uint16_t> borderColor;
    std::optional<uint16_t> fillColor;
  };

  struct CircleHints : ShapeHints {
    int16_t r;
  };

  struct RectHints : ShapeHints {
    Dimensions dims;
    int8_t cornerR;
  };

  struct TextHints {
    uint16_t color;
    uint8_t size;
    bool wrap;
  };

}