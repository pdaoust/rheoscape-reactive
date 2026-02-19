#pragma once

#include <types/core_types.hpp>

namespace rheoscape::sinks {

  // A sink that does nothing.
  // Why would you want this?
  // To pull on a source that broadcasts its values to multiple sinks at the same time
  // but lacks the ability to proactively push values
  // (an example is an event source such as an ISR-driven digital pin source that's wrapped in `share`).
  // You can use the dummy sink's single pull function as a convenience
  // so you don't have to store and pull on a whole bunch of pull functions.
  // It makes the source behave as if it really were proactive.
  namespace detail {

    template <typename T>
    struct dummy_push_handler {
      RHEOSCAPE_CALLABLE void operator()(T _) const { }
    };

    struct dummy_sink_binder {
      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, source_value_t<SourceFn>>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        using T = source_value_t<SourceFn>;
        return source(dummy_push_handler<T>{});
      }
    };

  } // namespace detail

  auto dummy_sink() {
    return detail::dummy_sink_binder{};
  }

}
