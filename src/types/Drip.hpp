#include <functional>
#include <core_types.hpp>

namespace rheo {

  // Turn a pull-only stream into a push stream
  // by offering a method that pulls from the stream.
  // This class binds to the supplied source on instantiation.
  template <typename T>
  class Drip {
    private:
      pull_fn _pullFn;
    
    public:
      Drip(source_fn<T> sourceFn, push_fn<T> push)
      : _pullFn(sourceFn(push))
      { }

      void drip() {
        _pullFn();
      }
  };

}