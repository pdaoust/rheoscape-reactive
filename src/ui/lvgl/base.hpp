#pragma once

#include <functional>
#include <core_types.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheo::ui::lvgl {

  WidgetPullAndEventSource base(
    lv_obj_t* base,
    source_fn<std::vector<StyleAndSelector>> style_source
  ) {
    return _widget(
      base,
      style_source
    );
  }

  const auto& button = base;

}