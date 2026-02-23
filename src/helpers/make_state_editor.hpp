#pragma once

#include <types/core_types.hpp>
#include <states/MemoryState.hpp>
#include <operators/map.hpp>
#include <operators/sample.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;

namespace rheoscape::helpers {

  // Construct a pipeline that binds a MemoryState<T> to a stream consisting of input events
  // (for instance, button presses and rotary encoder clicks).
  // The mapper function that you supply should take an input event as its first argument
  // and the current value of the state as its argument,
  // and transform the value of the state to a new value based on the input event.
  // The returned pull function should be pulled regularly,
  // unless the input event source pushes its values without being asked.
  template <typename T, typename InputSource, typename MapFn>
    requires concepts::Source<InputSource>
      && concepts::Transformer<MapFn, std::tuple<source_value_t<InputSource>, T>>
      && std::same_as<invoke_maybe_apply_result_t<std::decay_t<MapFn>, std::tuple<source_value_t<InputSource>, T>>, T>
  auto make_state_editor(InputSource input_source, MemoryState<T> state, MapFn mapper) {
    return input_source
      | sample(state.get_source_fn())
      | map(mapper)
      | state.get_setter_sink_fn();
  }

}