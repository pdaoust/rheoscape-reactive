// ======== TYPES
#include <types/core_types.hpp>
#if defined(ARDUINO)
  #include <types/arduino_millis_clock.hpp>
#endif
#include <types/au_all_units_noio.hpp>
#include <types/deserialization_error.hpp>
#include <types/Endable.hpp>
#include <types/Fallible.hpp>
#include <types/KnnStorage.hpp>
#include <types/mock_clock.hpp>
#include <types/Range.hpp>
#include <types/rep_clock.hpp>
#include <types/State.hpp>
#include <types/TaggedValue.hpp>
#include <types/thermal_sim.hpp>
#include <types/TuningStorage.hpp>
#include <types/Wrapper.hpp>

// ======== UTILITIES
#include <util/logging.hpp>
#include <util/misc.hpp>

// ======== SOURCES
#if defined(ARDUINO)
  #include <sources/arduino/analog_pin_source.hpp>
  #include <sources/arduino/bh1750.hpp>
  #include <sources/arduino/digital_pin_source.hpp>
  #include <sources/arduino/ds18b20.hpp>
  #include <sources/arduino/eeprom_source.hpp>
  #include <sources/arduino/sht2x.hpp>
#endif
#include <sources/constant.hpp>
#include <sources/done.hpp>
#include <sources/Emitter.hpp>
#include <sources/empty.hpp>
#include <sources/from_clock.hpp>
#include <sources/from_iterator.hpp>
#include <sources/from_observable.hpp>
#include <sources/knn_interpolate.hpp>
#include <sources/sequence.hpp>

// ======== SINKS
#if defined(ARDUINO)
  #include <sinks/arduino/gfx/types.hpp>
  #include <sinks/arduino/gfx/gfx_sink.hpp>
  #include <sinks/arduino/gfx/ssd1306_sink.hpp>
  #include <sinks/arduino/gfx/text_sink.hpp>
  #include <sinks/arduino/analog_pin_sink.hpp>
  #include <sinks/arduino/digital_pin_sink.hpp>
  #include <sinks/arduino/eeprom_sink.hpp>
  #include <sinks/arduino/serial_sinks.hpp>
  #include <sinks/arduino/servo_sink.hpp>
#endif
#include <sinks/dummy_sink.hpp>
#include <sinks/table_sink.hpp>

// ======== OPERATORS
#include <operators/bang_bang.hpp>
#include <operators/cache.hpp>
#include <operators/choose.hpp>
#include <operators/combine.hpp>
#include <operators/concat.hpp>
#include <operators/count.hpp>
#include <operators/debounce.hpp>
#include <operators/dedupe.hpp>
#include <operators/exponential_moving_average.hpp>
#include <operators/filter.hpp>
#include <operators/filter_map.hpp>
#include <operators/flat_map.hpp>
#include <operators/foreach.hpp>
#include <operators/inspect.hpp>
#include <operators/interval.hpp>
#include <operators/latch.hpp>
#include <operators/lift.hpp>
#include <operators/log_errors.hpp>
#include <operators/map.hpp>
#include <operators/merge.hpp>
#include <operators/normalize.hpp>
#include <operators/pid.hpp>
#include <operators/quadrature_encode.hpp>
#include <operators/sample.hpp>
#include <operators/scan.hpp>
#include <operators/settle.hpp>
#include <operators/share.hpp>
#include <operators/split_and_combine.hpp>
#include <operators/start_when.hpp>
#include <operators/stopwatch.hpp>
#include <operators/take.hpp>
#include <operators/take_while.hpp>
#include <operators/tee.hpp>
#include <operators/throttle.hpp>
#include <operators/timed_latch.hpp>
#include <operators/timestamp.hpp>
#include <operators/toggle.hpp>
#include <operators/unwrap.hpp>
#include <operators/waves.hpp>

// ======== HELPERS
#include <helpers/au_helpers.hpp>
#include <helpers/make_state_editor.hpp>
#include <helpers/chrono_helpers.hpp>

// Note: you explicitly need to pass the RHEOSCAPE_USE_LVGL flag to your compiler to compile in LVGL.
// This is because it has some specific (and annoying) setup requirements
// involving a config file `lv_conf.h` whose path is defined by convention
// to be in the library folder, not your source folder.
// This is dumb and un-portable.
#include <ui/lvgl/everything.hpp>