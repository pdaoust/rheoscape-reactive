#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheo::ui::lvgl {

  struct PointBarData {
    int32_t value;
    Range<int32_t> range;
    bool animate;
  };

  struct RangeBarData {
    Range<int32_t> value;
    Range<int32_t> range;
    bool animate;
  };

  WidgetPullAndEventSource pointBar(
    lv_obj_t* bar,
    source_fn<PointBarData> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    return _widget<PointBarData>(
      bar,
      [](PointBarData data, lv_obj_t* bar) {
        lv_bar_set_value(bar, data.value, data.animate ? LV_ANIM_ON : LV_ANIM_OFF);
        lv_bar_set_range(bar, data.range.min, data.range.max);
      },
      dataSource,
      styleSource
    );
  }

  WidgetPullAndEventSource rangeBar(
    lv_obj_t* bar,
    source_fn<RangeBarData> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    return _widget<RangeBarData>(
      bar,
      [](RangeBarData data, lv_obj_t* bar) {
        lv_bar_set_value(bar, data.value.max, data.animate ? LV_ANIM_ON : LV_ANIM_OFF);
        lv_bar_set_start_value(bar, data.value.min, data.animate ? LV_ANIM_ON : LV_ANIM_OFF);
        lv_bar_set_range(bar, data.range.min, data.range.max);
      },
      dataSource,
      styleSource
    );
  }
}