#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/foreach.hpp>
#include <operators/combine.hpp>
#include <lvgl.h>
#include <ui/lvgl/types.hpp>

namespace rheo::ui::lvgl {

  template <typename TData>
  WidgetPullAndEventSource _widget(
    lv_obj_t* widget,
    ApplyDataToWidgetFn<TData> applyDataFn,
    source_fn<TData> dataSource,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    // FIXME: Is it more performant _and_ equivalent to just do them separately w/o combining?
    pull_fn pullDataAndStyle = rheo::operators::combine(dataSource, styleSource, std::make_tuple<TData, std::vector<StyleAndSelector>>)
      | rheo::operators::foreach([widget, applyDataFn](std::tuple<TData, std::vector<StyleAndSelector>> value) {
        applyDataFn(std::get<0>(value), widget);
        for (const StyleAndSelector& style : std::get<1>(value)) {
          lv_obj_add_style(widget, &style.style, style.selector);
        }
      });

    // Eternal gratitude to the author of https://deplinenoise.wordpress.com/2014/02/23/using-c11-capturing-lambdas-w-vanilla-c-api-functions/
    source_fn<lv_event_code_t> eventSource = [widget](push_fn<lv_event_code_t> push) {
      lv_obj_add_event_cb(
        widget,
        [](lv_event_t* event) {
          void* push = lv_event_get_user_data(event);
          (*(push_fn<lv_event_code_t>*)push)(lv_event_get_code(event));
        },
        LV_EVENT_ALL,
        // If/when I need to start passing in user or custom data,
        // just create a struct to hold that data here.
        &push
      );
      return [](){};
    };

    return WidgetPullAndEventSource(
      pullDataAndStyle,
      eventSource
    );
  }
  
  WidgetPullAndEventSource _widget(
    lv_obj_t* widget,
    source_fn<std::vector<StyleAndSelector>> styleSource
  ) {
    pull_fn pullStyle = styleSource([widget](std::vector<StyleAndSelector> styles) {
      for (const StyleAndSelector& style : styles) {
        lv_obj_add_style(widget, &style.style, style.selector);
      }
    });

    source_fn<lv_event_code_t> eventSource = [widget](push_fn<lv_event_code_t> push) {
      lv_obj_add_event_cb(
        widget,
        [](lv_event_t* event) {
          void* push = lv_event_get_user_data(event);
          (*(push_fn<lv_event_code_t>*)push)(lv_event_get_code(event));
        },
        LV_EVENT_ALL,
        &push
      );
      return [](){};
    };

    return WidgetPullAndEventSource(
      pullStyle,
      eventSource
    );
  }

}