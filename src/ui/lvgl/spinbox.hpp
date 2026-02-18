#pragma once

#if defined(RHEOSCAPE_USE_LVGL)

#include <functional>
#include <types/core_types.hpp>
#include <types/Range.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>
#include <Arduino.h>

namespace rheoscape::ui::lvgl {

  template <typename T>
  WidgetPullAndEventSource spinbox(
    lv_obj_t* spinbox,
    source_fn<T> data_source,
    source_fn<std::vector<StyleAndSelector>> style_source
  ) {
    return _widget<T>(
      spinbox,
      [](T data, lv_obj_t* spinbox) {
        Serial.println("About to set value of spinbox");
        //lv_spinbox_set_value(spinbox, (int32_t)data);
      },
      data_source,
      style_source
    );
  }

}

#endif
