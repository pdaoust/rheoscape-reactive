#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <operators/foreach.hpp>
#include <operators/combine.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>

namespace rheo::ui::lvgl {

  // Push handler for applying styles to a widget
  struct widget_style_push_handler {
    lv_obj_t* widget;

    RHEO_NOINLINE void operator()(std::vector<StyleAndSelector> styles) const {
      for (const StyleAndSelector& style : styles) {
        lv_obj_add_style(widget, &style.style, style.selector);
      }
    }
  };

  // Pull handler that holds a shared_ptr to keep the push function alive
  // for LVGL callbacks. The pull itself is a no-op since events are push-only.
  struct widget_event_pull_handler {
    std::shared_ptr<push_fn<lv_event_code_t>> pushStorage;

    RHEO_NOINLINE void operator()() const {
      // No-op: events are pushed by LVGL, not pulled
    }
  };

  struct widget_event_source_binder {
    lv_obj_t* widget;

    RHEO_NOINLINE pull_fn operator()(push_fn<lv_event_code_t> push) const {
      auto pushStorage = std::make_shared<push_fn<lv_event_code_t>>(std::move(push));
      lv_obj_add_event_cb(
        widget,
        [](lv_event_t* event) {
          void* data = lv_event_get_user_data(event);
          (*(push_fn<lv_event_code_t>*)data)(lv_event_get_code(event));
        },
        LV_EVENT_ALL,
        pushStorage.get()
      );
      return widget_event_pull_handler{pushStorage};
    }
  };

  template <typename TData>
  WidgetPullAndEventSource _widget(
    lv_obj_t* widget,
    ApplyDataToWidgetFn<TData> applyDataFn,
    source_fn<TData> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    // FIXME: Is it more performant _and_ equivalent to just do them separately w/o combining?
    pull_fn pullDataAndStyle = rheo::operators::combine(std::make_tuple<TData, std::vector<StyleAndSelector>>, dataSource, styleSource)
      | rheo::operators::foreach([widget, applyDataFn](std::tuple<TData, std::vector<StyleAndSelector>> value) {
        applyDataFn(std::get<0>(value), widget);
        for (const StyleAndSelector& style : std::get<1>(value)) {
          lv_obj_add_style(widget, &style.style, style.selector);
        }
      });

    // Eternal gratitude to the author of https://deplinenoise.wordpress.com/2014/02/23/using-c11-capturing-lambdas-w-vanilla-c-api-functions/
    source_fn<lv_event_code_t> eventSource = widget_event_source_binder{widget};

    return WidgetPullAndEventSource(
      pullDataAndStyle,
      eventSource
    );
  }
  
  WidgetPullAndEventSource _widget(
    lv_obj_t* widget,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    pull_fn pullStyle = styleSource(widget_style_push_handler{widget});

    source_fn<lv_event_code_t> eventSource = widget_event_source_binder{widget};

    return WidgetPullAndEventSource(
      pullStyle,
      eventSource
    );
  }

}