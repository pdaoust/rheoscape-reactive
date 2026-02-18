#pragma once

#if defined(RHEOSCAPE_USE_LVGL)

#include <functional>
#include <types/core_types.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheoscape::ui::lvgl {

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

#endif
