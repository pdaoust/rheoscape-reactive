#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>
#include <Arduino.h>

namespace rheo::ui::lvgl {

  template <typename T>
  WidgetPullAndEventSource spinbox(
    lv_obj_t* spinbox,
    source_fn<T> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    return _widget<T>(
      spinbox,
      [](T data, lv_obj_t* spinbox) {
        Serial.println("About to set value of spinbox");
        //lv_spinbox_set_value(spinbox, (int32_t)data);
      },
      dataSource,
      styleSource
    );
  }

}