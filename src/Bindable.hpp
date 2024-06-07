#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include <functional>

template <typename TBindable>
class Bindable {
  public:
    template <typename TNew>
    TNew bind(std::function<TNew(TBindable)> binder) {
      return binder(this);
    }

    void bind(std::function<void(TBindable)> binder) {
      binder(this);
    }
};

#endif