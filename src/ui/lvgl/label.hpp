#pragma once

#if defined(RHEOSCAPE_USE_LVGL)

#include <types/core_types.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>
#include <ui/lvgl/widget.hpp>

namespace rheoscape::ui::lvgl {

  WidgetPullAndEventSource label(
    lv_obj_t* label,
    source_fn<std::string> data_source,
    source_fn<std::vector<StyleAndSelector>> style_source
  ) {
    return _widget<std::string>(
      label,
      [](std::string text, lv_obj_t* label) {
        lv_label_set_text(label, text.c_str());
      },
      data_source,
      style_source
    );
  }

}

#endif
