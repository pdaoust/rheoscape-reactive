#pragma once

#include <functional>
#include <types/core_types.hpp>

namespace rheoscape::sinks {

  // Input type for table_sink.
  // Combines a key and value for storage.
  template <typename TKey, typename TValue>
  struct TableSinkInput {
    TKey key;
    TValue value;
  };

  // Store function signature for IoC.
  // User provides this callable to handle actual storage.
  // Returns true if stored successfully, false if rejected
  // (e.g., quality not better than existing record).
  template <typename TKey, typename TValue>
  using store_fn = std::function<bool(const TKey& key, const TValue& value)>;

  // Named callable for table sink push handler.
  template <typename TKey, typename TValue>
  struct table_sink_push_handler {
    store_fn<TKey, TValue> store;

    RHEOSCAPE_CALLABLE void operator()(TableSinkInput<TKey, TValue> input) const {
      store(input.key, input.value);
    }
  };

  namespace detail {

    // Generic table sink with IoC for storage backend.
    //
    // The sink is agnostic about how data is stored.
    // The caller injects a store function that handles actual storage.
    // This enables:
    // - NVRAM/EEPROM persistence
    // - In-memory storage
    // - Database backends
    // - Quality-based record replacement
    //
    // Usage examples:
    //
    //   // Store to NVRAM
    //   auto nvram_store = [](const PlantParams& key, const PidWeights& value) {
    //     nvram_write(key, value);
    //     return true;
    //   };
    //   auto sink = table_sink<PlantParams, PidWeights>(nvram_store);
    //
    //   // Store with quality check
    //   auto db_store = [&db](const PlantParams& key, const TuningRecord& value) {
    //     auto existing = db.find(key);
    //     if (!existing || value.quality < existing->quality) {
    //       db.upsert(key, value);
    //       return true;
    //     }
    //     return false;
    //   };
    //   auto sink = table_sink<PlantParams, TuningRecord>(db_store);

    template <typename TKey, typename TValue>
    struct table_sink_binder {
      store_fn<TKey, TValue> store;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, TableSinkInput<TKey, TValue>>
      RHEOSCAPE_CALLABLE void operator()(SourceFn source) const {
        table_sink_push_handler<TKey, TValue> handler{store};
        auto pull = source(handler);
        // Pull once to trigger initial storage if source has data.
        pull();
      }
    };

    // Table sink that returns a pull function for manual control.
    // Useful when you want to control when storage happens.
    template <typename TKey, typename TValue>
    struct table_sink_pullable_binder {
      store_fn<TKey, TValue> store;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, TableSinkInput<TKey, TValue>>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        table_sink_push_handler<TKey, TValue> handler{store};
        return source(handler);
      }
    };

  } // namespace detail

  template <typename TKey, typename TValue>
  auto table_sink(store_fn<TKey, TValue> store) {
    return detail::table_sink_binder<TKey, TValue>{std::move(store)};
  }

  template <typename TKey, typename TValue>
  auto table_sink_pullable(store_fn<TKey, TValue> store) {
    return detail::table_sink_pullable_binder<TKey, TValue>{std::move(store)};
  }

  // Convenience function to create a TableSinkInput.
  template <typename TKey, typename TValue>
  TableSinkInput<TKey, TValue> make_table_sink_input(TKey key, TValue value) {
    return TableSinkInput<TKey, TValue>{ std::move(key), std::move(value) };
  }

}
