#pragma once

#include <functional>

namespace rheo::sinks::arduino::adafruitGfx {

  struct Coords {
    int16_t x;
    int16_t y;
  };

  struct Dimensions {
    uint16_t w;
    uint16_t h;
  };

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

  template <typename TCanvas>
  using GfxCommand = std::function<void(TCanvas&)>;

}