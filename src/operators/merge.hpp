#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <types/Wrapper.hpp>

// Merge two streams together.
// If you pull a merged stream, both the upstream sources get pulled,
// and the first stream gets pulled first.
// If one stream has ended, it'll still keep pushing values from the other one.

namespace rheo::operators {

  template <typename T1, typename T2>
  source_fn<std::variant<T1, T2>> merge(source_fn<T1> source1, source_fn<T2> source2) {
    return [source1, source2](push_fn<std::variant<T1, T2>> push) {
      auto pullSource1 = source1(push);
      auto pullSource2 = source2(push);
      return [pullSource1, pullSource2](){
        pullSource1();
        pullSource2();
      };
    };
  }

  template <typename T1, typename T2>
  pipe_fn<std::variant<T1, T2>, T1> merge(source_fn<T2> source2) {
    return [source2](source_fn<T1> source1) {
      return merge(source1, source2);
    };
  }

  template <typename T>
  source_fn<T> merge(source_fn<T> source1, source_fn<T> source2) {
    return [source1, source2](push_fn<T> push) {
      auto pullSource1 = source1(push);
      auto pullSource2 = source2(push);
      return [pullSource1, pullSource2](){
        pullSource1();
        pullSource2();
      };
    };
  }

  template <typename T>
  pipe_fn<T, T> merge(source_fn<T> source2) {
    return [source2](source_fn<T> source1) {
      return merge(source1, source2);
    };
  }

}