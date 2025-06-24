#include <functional>
#include <core_types.hpp>

namespace rheo {

  // A wrapper that lets you write source function transformations in a method-chaining style.
  // This class does not bind to the supplied source function on instantiation,
  // which means it's more like a source function factory or a pipe function itself.
  template <typename T>
  class Pipe {
    private:
      source_fn<T> _sourceFn;
    
    public:
      Pipe(source_fn<T> sourceFn)
      : _sourceFn(sourceFn)
      { }

      pull_fn sink(push_fn<T> pushFn, end_fn endFn) {
        return _sourceFn(pushFn, endFn);
      }

      pull_fn sink(pullable_sink_fn<T> sink) {
        return sink(_sourceFn);
      }

      template <typename TReturn>
      void cap(cap_fn<T> cap) {
        cap(_sourceFn);
      }

      template <typename TOut>
      Pipe<TOut> pipe(pipe_fn<TOut, T> pipe) {
        return Pipe<TOut>(pipe(_sourceFn));
      }
  };
 
}