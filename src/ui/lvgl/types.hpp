#pragma once

#if defined(RHEOSCAPE_USE_LVGL)

#include <functional>
#include <types/core_types.hpp>
#include <lvgl.h>

namespace rheoscape::ui::lvgl {

  using WidgetPullAndEventSource = std::tuple<pull_fn, source_fn<lv_event_code_t>>;

  template <typename TData>
  using ApplyDataToWidgetFn = std::function<void(TData, lv_obj_t*)>;

  struct StyleAndSelector {
    lv_style_t style;
    lv_style_selector_t selector;
  };

}

#endif
