#pragma once

#include <functional>
#include <core_types.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheo::ui::lvgl {

  WidgetPullAndEventSource label(
    lv_obj_t* label,
    source_fn<std::string> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    return _widget<std::string>(
      label,
      [](std::string text, lv_obj_t* label) {
        lv_label_set_text(label, text.c_str());
      },
      dataSource,
      styleSource
    );
  }

}