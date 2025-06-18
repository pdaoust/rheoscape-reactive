#pragma once

#include <functional>
#include <core_types.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheo::ui::lvgl {

  WidgetPullAndEventSource base(
    lv_obj_t* base,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    return _widget(
      base,
      styleSource
    );
  }

  const auto& button = base;

}