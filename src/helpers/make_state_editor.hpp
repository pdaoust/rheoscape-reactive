#pragma once

#include <core_types.hpp>
#include <types/State.hpp>
#include <operators/map.hpp>
#include <operators/sample.hpp>

using namespace rheo;
using namespace rheo::operators;

namespace rheo::helpers {

  template <typename T, typename InputSource, typename MapFn>
    requires concepts::Source<InputSource>
      && concepts::Transformer<MapFn, std::tuple<source_value_t<InputSource>, T>>
      && std::same_as<invoke_maybe_apply_result_t<std::decay_t<MapFn>, std::tuple<source_value_t<InputSource>, T>>, T>
  auto make_state_editor(InputSource input_source, State<T> state, MapFn mapper) {
    return input_source
      | sample(state.get_source_fn())
      | map(mapper)
      | state.get_setter_sink_fn();
  }

}