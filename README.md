# Rheoscape

## A functional reactive programming library for embedded devices, written in C++.

Rheoscape was written to fill a gap in embedded programming -- a way to easily compose streams of data in the functional reactive style while keeping control over how the streams are executed. In embedded environments this is important: you want to be able to reason about what's happening, when, and how often, so that you can tune your program to squeeze nicely into the constrained hardware you're building for.

Rheoscape comes with a few conveniences for constructing elegant, readable streams using the `|` operator:

```c++
pull_fn set_servo_to_soil_moisture_reading;
pull_fn handle_watering_threshold_input;

void setup() {
    // Soil moisture sensor on GPIO 3.
    static auto soil_moisture_sensor = analog_pin_source(3)
        // We've calibrated the sensor and it's fully wet at 350 and fully dry at 800.
        // Change that range to 0-100%.
        | normalize(constant(Range(800, 350)), constant(Range(0, 100)));

    // Show the moisture reading on a servo-controlled gauge.
    set_servo_to_soil_moisture_reading = soil_moisture_sensor
        // Translate that into a rotation angle for our servo.
        | normalize(constant(Range(0, 100)), constant(Range(0, 180)))
        // We have a cheap micro servo on GPIO 4.
        | servo_sink(4);

    // Allow the user to calibrate the point where the water pump turns on to water the plants.
    static MemoryState watering_threshold(50);
    // Rotary encoder on pins 4 and 5.
    static auto rotary_encoder = digital_pin_interrupt_source<4, 5>(INPUT_PULLUP)
        | quadrature_encode();
    handle_watering_threshold_input = make_state_editor(
        rotary_encoder,
        watering_threshold,
        [](QuadratureEncodeDirection dir, int threshold) {
            // QuadratureEncodeDirection::Clockwise maps to +1
            // and CounterClockwise maps to -1.
            threshold += dir;
            // Clamp to 0-100%.
            if (threshold < 0) { threshold = 0; }
            else if (threshold > 100) { threshold = 100; }
            return threshold;
        }
    );

    // Turn the pump on or off depending on the soil reading and the watering threshold.
    watering_threshold.get_source_fn()
        | sample(soil_moisture_sensor)
        | map([](int threshold, int reading) {
            return reading < threshold
                // Water pump relay is active low.
                ? LOW
                : HIGH;
        })
        // Water pump relay is on GPIO 6.
        | digital_pin_sink(6);
}

void loop() {
    set_servo_to_soil_moisture_reading();
    handle_watering_threshold_input();
}
```

## Get started

Rheoscape is, unashamedly, a C++20 project. It uses a lot of C++20's features to make for a graceful developer experience.

### Prerequisites

* GCC version 10 (you might be able to use Clang too, but I don't have any experience with it).

### Installing

If you're using **PlatformIO**:

1. Open up your project's `platformio.ini` file.
2. Make sure `build_unflags` contains `-std=gnu++11 -std=gnu++14 -std=gnu++17` and `build_flags` contains `-std=gnu++20`.
3. Add `pdaoust/rheoscape-reactive` to the list of `lib_deps` for any environment you want to use Rheoscape in.

If you're using **Arduino IDE**:

1. Open up your project in the IDE.
2. Go to **Sketch** > **Include Library** > **Manage Libraries**.
3. Search for `rheoscape-reactive` and install it into your project.

### Using

Rheoscape is a header-only library, so it gets built right into your binary. Add this to the top of your files:

```c++
#include <rheoscape.hpp>
```

That's it! Happy streaming.

## Dependencies

If dependencies don't get installed automatically, you'll need to at least install `fmtlib/fmt`. Many sources and sinks also require `Arduino.h` and will be disabled if it can't be found. These sources and sinks might also require the following libraries:

* `claws/BH1750@^1.3.0`
* `adafruit/Adafruit GFX Library@^1.12.1`
* `adafruit/Adafruit SSD1306@^2.5.14`
* `lvgl/lvgl@^9.3.0`
* `milesburton/DallasTemperature@^4.0.4`
* `bodmer/TFT_eSPI@^2.5.43` (ESP32 only)
* `robtillaart/SHT2x@^0.5.2`
* `bblanchon/ArduinoJson@^7.4.2`
* `madhephaestus/ESP32Servo@^3.0.9` (ESP32 only)

## Supported platforms

Rheoscape works with any C++ project, but the Arduino sources and sinks only work on embedded devices in programs based on the Arduino framework. Currently the ESP32 and RP2040 Arduino cores are supported across all sources and sinks.

## License

Rheoscape is licensed under the [GPL v3](https://www.gnu.org/licenses/gpl-3.0.en.html).

## Important terms

Rheoscape is based on the idea of **streams** between [**sources**](#source) and [**sinks**](#sink). Almost everything in Rheoscape is just a callable. That's the entire core of Rheoscape -- a spec for agreeing on how a source and a sink should interact to create a stream. (But Rheoscape does come with a decent-sized standard library.)

### Source

A source emits values. It's just a function that receives an observer, or [**push function**](#push-function), and whenever it's ready to emit a value, it passes the value to the push function bound to it. It also returns a [**pull function**](#pull-function) when it's called (we'll get to that in a moment).

Here's a type alias for source, pull, and push functions:

```c++
using pull_fn = std::function<void()>;

template <typename T>
using push_fn = std::function<void(T)>;

template <typename T>
using source_fn = std::function<pull_fn(push_fn<T>)>;
```

(Note that, while Rheoscape does define these types, it doesn't use them much in the standard library because `std::function` is a type-erased type so the compiler can't optimise long chains of them.)

A source might take its own initiative, pushing its values downstream as they become available, like when an HTTP request comes in or an interrupt fires. We call this a **push source**. (Note that very few sources can actually take initiative without some sort of prodding; we'll talk more about this in [Program flow](#program-flow).)

Or a [**sink**](#sink) (we'll get to sinks in a moment too) might request values from the source by calling the source's pull function. This sink might be an OLED display refreshing, or a GPIO that controls a heater and needs to pull a temperature reading. In most cases, streams are pull-based because it works nicely with the way Arduino programs usually run, with a 'main' control loop that polls things that need to be updated regularly.

You can think of a source function as a stream factory -- you can bind a sink to most sources multiple times and get entirely independent streams. That way, when sink A pulls on a source, it doesn't push a value to sink B's push function. (Not usually anyway -- there are some exceptions.)

Here's a simple source, just a lambda that returns the number 3 whenever it's pulled:

```c++
source_fn<int> threes_source = [](push_fn<int> push) {
    return [push]() {
        push(3);
    };
};
```

The returned lambda is the pull function that pushes the number 3 to any push function that is passed to it. You can see that a new pull lambda gets created every time the source is called with a new push function.

#### Push function

A push function is a simple observer function. It takes one argument, typed to the value type that the source emits, and does something useful with it. It doesn't return anything.

#### Pull function

As mentioned, a source returns a pull function when it's called. This is a handle that lets the sink request a new value, which will be delivered to the sink's push function.

Not all sources support pulling. Some of them are push-only, and their pull functions do nothing.

### Sink

A sink isn't a concrete thing; it's more of a concept -- it's just the act of passing a push function to a source and getting a pull function in return. Sinking, or binding, to a source can be as simple as passing a lambda and storing the pull function in a variable for later use. This example uses the `threes_source` from above:

```c++
pull_fn pull_threes = threes_source([](int v) {
    Serial.print("Got a number: ");
    Serial.println(v);
});
```

Every time you call `pull_threes()`, it'll print a message to the serial port.

### State

A state holds, well, state! It keeps track of mutable values and provides both a source function (which emits values whenever the state is changed, but can also be pulled any time you like) and a sink function (which updates the state with each value pushed to it). Other frameworks call them 'reactive values'.

Currently there are two different state types:

* `MemoryState`, which is backed by RAM
* `EepromState`, which is backed by NVRAM via the Arduino `EEPROM` library

State objects have a common interface:

* `get_source_fn(bool initial_push = true)`: Get a source function that presents the value in the state object as a stream. `initial_push` controls whether sinks get pushed a value as soon as they bind to the source.
* `get_setter_sink_fn(bool push_on_set = true)`: Get a pullable sink function that updates the state whenever values are pushed to it. `push_on_set` controls whether the state's source function also pushes values downstream.
* `get_setter_push_fn(bool push_on_set = true)`: Get a push function that you can bind to sources.
* `set(T value, bool push = true)`: Update the state, and optionally push its value to bound sinks.
* `get()`: Get the value. You should guard calls to this function with `has_value()` or use `try_get()` instead.
* `has_value()`: Check whether the state has a value.
* `try_get()`: Get the value wrapped in a `std::optional`, if it exists.
* `add_sink(PushFn push, bool initial_value = true)`: An alternative way of binding a push function to the state rather than using its source function.

### Stream

A stream is just a chain of sources and sinks where values flow from the primary source to a final sink.

#### Operator

Something can be both a source and a sink at the same time -- we call these **operators**, and they transform the shape of the stream as it passes downstream from source to sink. Familiar functional operators like `map`, `filter`, and `scan` are included in the standard library. Operators accept a source function (and maybe other arguments) and return a new source function. Here's a simple implementation of `map`:

```c++
template <typename TOut, typename TIn>
source_fn<TOut> map(source_fn<TIn> source, std::function<TOut(TIn)> mapper) {
    // Return a new source function that pushes a transformed value for every upstream value.
    return [source, mapper](push_fn<TOut> push) {
        // Bind to the upstream source.
        // The upstream source's pull function is passed downstream.
        return source([mapper, push](TIn value) {
            push(mapper(value));
        });
    };
}
```

You'd then use `map` like this:

```c++
source_fn<int> pull_threes_squared = map(
    threes_source,
    [](int v) { return v * v; }
);
```

## Program flow

Rheoscape streams are opinionated about concurrency: they don't do it. Ever. This was an intentional design choice to keep control flow easy to reason about and program for. No mutexes or contention over shared state, no async operations. Either a source pushes a new value all the way down its pipeline, or a sink pulls the next value from its upstream source(s), in a continuous synchronous flow.

\* _That doesn't mean there's no concurrency at all. Some sources use interrupt service routines (ISRs) to collect data in real time, although in practice the ISRs don't actually emit values downstream -- that would make the ISR too heavy and prone to concurrency problems. You still need to pull on the stream to collect waiting values. With ISR-driven sources, we try hard to prevent contention over shared state by temporarily disabling interrupts and taking an atomic snapshot of the values the ISR has written._

Push-only streams flow like this:

```mermaid
sequenceDiagram
    source->>operator: value
    operator->>sink: transformed value
```

Pullable streams flow like this:

```mermaid
sequenceDiagram
    sink->>operator: pull
    operator->>source: pull
    source->>operator: value
    operator->>sink: transformed value
```

In both scenarios, it all happens in a single synchronous stack of function calls. If you want multiple streams to be flowing simultaneously, you have to just pull on them sequentially and hope that they each complete quickly enough to feel simultaneous :)

### A word on exceptions

Rheoscape doesn't throw exceptions. You shouldn't either. On some cores (e.g., RP2040) they're even deprecated and disabled. Instead, if a source can fail, it should emit `Fallible<T, E>` values.

## Patterns

We've mentioned how Rheoscape is just a pattern for sources and sinks (and, by extension, operators which are both source and sink and let you make long pipes). Here are a couple other patterns you'll see a lot of in the standard library, and you should think about them if you're designing your own sources, sinks, and operators to be reused by other people.

(Note that all of the example implementations here are naïve and don't lend themselves well to compiler optimisation. But you'll find more robust implementations of each of them in the standard library.)

### Source factory

A source factory constructs a source function. A common one is `digital_pin_source`, which takes the GPIO pin you want to listen to and returns a source function that streams the high/low state of that pin. Here's a basic implementation of `digital_pin_source` (there's a more complete implementation in the standard library):

```c++
source_fn<bool> digital_pin_source(int pin, int mode) {
    pinMode(pin, INPUT | mode);
    // This is the source function:
    return [pin](push_fn<bool> push) {
        // And this is the pull function:
        return [pin, push]() {
            // Every time downstream requests the pin's state,
            // read it and push the value downstream.
            push(digitalRead(pin));
        };
    };
};

source_fn<bool> gpio_3_source = digital_pin_source(3, INPUT_PULLUP);
pull_fn pull_gpio_3 = gpio_3_source([](bool v) {
    Serial.print("The value of GPIO3 is: ");
    Serial.println(v ? "HIGH" : "LOW");
});
```

### Sink function

A sink function is a function that takes a source and sinks to it -- that is, it binds an internal push function to the source and does something with the source's returned pull function (usually returns it so it can be stored and called later).

```c++
template <typename TReturn, typename T>
using sink_fn = std::function<TReturn(source_fn<T>)>;
```

Here's a sink function that sets the value of GPIO 6:

```c++
sink_fn<pull_fn, bool> gpio_6_sink = [](source_fn<bool> source) {
    pinMode(6, OUTPUT);
    return source([](bool v) { digitalWrite(6, v); });
};
```

That's not very reusable though, so we usually use...

### Sink factory

A sink factory constructs a sink that you can bind to a source.

```c++
sink_fn<pull_fn, bool> digital_pin_sink(int pin) {
    pinMode(pin, OUTPUT);
    // This is the sink function, mostly unchanged from the `gpio_6_sink` example above.
    return [pin](source_fn<bool> source) {
        return source([pin](bool v) { digitalWrite(pin, v); });
    };
}

sink_fn<pull_fn, bool> gpio_6_sink = digital_pin_sink(6);
```

Here's something exciting: Rheoscape has a `|` operator overload so you can pipe a source to a sink. So using the sources and sinks we defined above, we can write:

```c++
pull_fn set_gpio_6_to_gpio_3 = gpio_3_source | gpio_6_sink;
```

Or you can just construct the source and sink inline and save a couple lines:

```c++
pull_fn set_gpio_6_to_gpio_3 = digital_pin_source(3)
    | digital_pin_sink(6);
```

### Pipe function

A pipe function receives a single source function, transforms it, and returns a new source function. You can think of it as a sink that produces a source. You can build your own pipes by chaining pipe functions together with the `|` operator.

```c++
template <typename TOut, typename TIn>
using pipe_fn = sink_fn<source_fn<TOut>, TIn>;
```

#### Pipe function factory

A pipe function factory creates a pipe function. Almost all operators in the standard library have a pipe function factory equivalent. Here's an example for `map`:

```c++
template <typename TOut, typename TIn>
pipe_fn<TOut, TIn> map(std::function<TOut(TIn)> mapper) {
    // Here's the pipe function.
    return [mapper](source_fn<TIn> source) {
        return map(source, mapper);
    };
}

pull_fn set_gpio_6_to_opposite_of_gpio_3 = gpio_3_source
    | map([](bool v) { return !v; })
    | gpio_6_sink;
```

## Useful types

The Rheoscape standard library comes with a lot of good structs, classes, sources, sinks, and operators. Here are a few:

* **Types**
    * `au_all_units_noio.hpp` and `au_noio.hpp`: A third-party library [au from Aurora Opensource](https://aurora-opensource.github.io/au/main/) that provides type-safe measurement unit math. Many Arduino sources and sinks work with streams of au values.
    * `Endable`: A struct that's used in streams that can end (e.g., sequences and iterables).
    * `Fallible`: A struct that's used in streams that can intermittently fail (e.g., sensors that can get unplugged, JSON that can't be
    deserialised). This should always be used instead of throwing exceptions in a source function.
    * `mock_clock`: A `std::chrono` clock that lets you set the exact time. Used in tests.
    * `Range`: A struct that lets you specify an inclusive range between any two values of a comparable type.
    * `rep_clock`: A `std::chrono` clock that doesn't provide a `now()` method; it just lets you define time points and durations with the magnitude and representation types you need.
    * `MemoryState`: A struct that lets you store and mutate state. (In some libraries this is called a 'reactive value'.) It provides a source function and a sink function, and you can choose whether it pushes values immediately or only on pull.
* **Sources**
    * `arduino`: Digital and analogue GPIOs, popular sensors, EEPROM
    * `constant`: Keep on emitting the same value whenever it's pulled
    * `done`: A source that immediately emits an ended `Endable` and shuts up.
    * `Emitter`: A struct that provides a source function. You can push values to it, and it'll proactively push it out to all subscribed sinks. It's like `MemoryState` but doesn't hold any state.
    * `empty`: A source function that doesn't ever produce any values, no matter how many times you pull on it.
    * `from_clock`: A source function that takes a `std::chrono` clock and samples it whenever pulled.
    * `from_iterator`: A source function that iterates over an iterator, producing `Endable<T>` values until it's been fully iterated.
    * `from_observable`: A source function that receives a subscriber function, passes its own observer function to it, and pushes observed values.
    * `sequence`: A source function that counts from a start value to an end value, with optional step increments (default 1).
* **Sinks**
    * `arduino`: Digital and analogue GPIOs, serial console, controls, EEPROM, and Adafruit GFX-based displays.
    * `dummy_sink`: A sink that does nothing. Its sole purpose is to pull from sources that can push to multiple sinks at once, but don't do any pushing until at least one sink pulls on it. (Right now, this means the `share` operator, which takes any stream and broadcasts to all downstream subscribers.)
* **States**
    * `EepromState`: Uses Arduino's EEPROM library to store and retrieve state that survives restarts.
    * `MemoryState`: An in-memory state.
* **Operators**
    * `bang_bang`: A simple thermostat with a configurable dead zone.
    * `cache`: Remember the last value pushed from upstream and emit it if a new value isn't pushed on pull.
    * `choose`: Switch between multiple sources based on the value of a switcher source.
    * `combine`: Join two or more streams together into a stream that emits a tuple of all streams. Only emits a value if all upstream sources can produce a value at the same time.
    * `concat`: Join two endable streams of the same type together.
    * `count`: Two operators, `count`, which emits a stream that counts the number of values received from upstream, and `tag_count`, which emits a stream of the original values tagged with the count.
    * `debounce`: When an upstream source changes, watch for a settling period, then emit the new value if it survives fluctuation after the settling period is over. If you're an electrical engineer, this is like hardware debounce. If you're an FRP programmer, you're probably looking for `settle`.
    * `dedupe`: Turn a continuous stream into a stream that only emits values on a change.
    * `exponential_moving_average`: A single-pole infinite-impulse-response filter. In simpler terms, it's a rolling average that smooths out high-frequency variations.
    * `filter_map`: Filter and map a stream's values at one time.
    * `filter`: Remove values from a stream that don't match the given predicate.
    * `flat_map`: Turn one value into zero or more values, each of which is emitted individually downstream.
    * `inspect`: Execute a function for every value and pass the value downstream.
    * `interval`: Emit a timestamp at intervals. The interval is a source itself, so it can change over time (this could be useful for exponential backoff).
    * `latch`: Transform a stream of `std::optional<T>` values into a stream of values where the last non-empty value is emitted. Kinda like `filter([](std::optional<T> v) { return v.has_value() }) | cache()`, which has me questioning its value.
    * `lift`: Lift a pipeline to a higher-ordered type so it can be fitted into another pipeline that uses that type. An example is taking an operator that works with bare scalar values (like `bang_bang` or `pid`) and making it work with an `Endable`, `Fallible`, or `std::optional` stream.
    * `log_errors`: Log errors in a `Fallible` stream. Uses the `rheoscape::logging` utility.
    * `map`: Transform one value type into another.
    * `merge`: Blend multiple streams with a common value type into one.
    * `normalize`: Map a stream of values from one range to another.
    * `pid`: A proportional/integral/derivative for high-precision system control. Can be trained.
    * `quadrature_encode`: Takes two boolean inputs and applies 'quadrature' or 'Gray coding' to it. Used for rotary encoders. I'd recommend using `digital_pin_interrupt_source<pin_a, pin_b>()` rather than combining two single non-interrupt-driven digital pin sources; the interrupt version is more responsive.
    * `sample`: Like `combine`, but it only produces a combined value when values are pushed to the first stream.
    * `scan`: Consume values over time, keeping state, similar to `fold` or `reduce` but emitting an accumulated value for every received value.
    * `settle`: Only emit a value after it's held stable (no new/changed values) for a given settling period. This is equivalent to FRP `debounce` operators; Rheoscape's `debounce` is more like a hardware debounce.
    * `share`: When a value is received, emit it to _all_ downstream subscribers.
    * `start_when`: Don't start emitting values until at least one value matches a given condition.
    * `stopwatch_changes`: Like `timestamp`, but rather than emitting timestamps, it emits durations that measure how long a value has been stable for. Useful for things like giving a countdown to a newly changed value before saving it to NVRAM.
    * `stopwatch_when`: Like `stopwatch_changes`, but it emits durations since a given 'lap start' condition was first met. Laps start whenever the input stream transitions from _not_ matching to matching the lap start condition, and continue past when the condition stops matching until the condition is met again.
    * `take_while`: Only emit values until at least one value fails to match a given condition.
    * `take`: Emit an endable stream of the first _n_ of the upstream's values.
    * `tee`: Join a side stream to a stream, pushing received values to both streams.
    * `throttle`: Only emit a value every _n_ time units.
    * `timed_latch`: Given a default value, hold any non-default values for a given interval before reverting back to the default.
    * `timestamp`: Tag values with a timestamp.
    * `toggle`: Turn one source on or off with a boolean value from another source.
    * `unwrap`: Operators to unwrap optional, fallible, or endable values; null, error, and ended values respectively are not emitted.
    * `waves`: Generate waveforms using a time source.
* **Helpers**:
    * `au_helpers`: Helper functions for working with `au` measurement values.
    * `chrono_helpers`: Helper functions for working with `std::chrono` measurement values.
    * `make_state_editor`: Construct a pipeline that changes a `state` using an input stream and a mapper function.
* **Utilities**
    * `compose_pipes`: Compose multiple pipe factories into a single callable that applies them left-to-right when given a source. All type deduction is deferred until the composed pipe is called, so unconstrained pipe factories (whose types aren't known until a source is provided) can be chained without needing the `|` operator. Can be combined with `typed_pipe` to produce a `Pipe`-concept-satisfying wrapper.
    * `logging`: A logging implementation that you can configure in a centralised way. Different log levels or topics can have different loggers bound to them.
    * `typed_pipe`: Annotate an arbitrary pipe callable with explicit input and output type aliases, making it satisfy the `Pipe` concept. Useful for wrapping pipe factories whose types are fully generic until called.

## Performance considerations

Rheoscape is designed to do as much of its hard work at compile time as possible. It does this in these ways:

* **No exceptions**: Sources that can error emit streams of `Fallible<T, Err>` values instead of throwing exceptions. Truly exceptional conditions are handled by `assert` rather than `throw`. (This doesn't cover the standard library -- you're still on your own if you try to call `.value()` on a `std::nullopt`.)
* **Disciplined ownership semantics**: Rheoscape avoids duplicating values as they're passed down a stream.
* **Configurable aggressive inlining**: Flow graphs are just stacks of functions, and they can get quite tall (each operator adds at least two functions to a call stack). These stacks can be aggressively inlined (albeit at the cost of larger binary weight) with the `RHEOSCAPE_AGGRESSIVE_INLINE` macro.

    <table>
    <thead>
    <tr>
    <th></th>
    <th colspan="2"><code>RHEOSCAPE_AGGRESSIVE_INLINE</code></th>
    </tr>
    <tr>
    <th></th>
    <th>defined</th>
    <th>undefined</th>
    </tr>
    </thead>

    <tbody>
    <tr>
    <th>Binary weight</th>
    <td>higher ⚠️</td>
    <td>lower ✅</td>
    </tr>
    <tr>
    <th>Call stack size</th>
    <td>lower ✅</td>
    <td>higher ⚠️</td>
    </tr>
    </tbody>
    </table>

## Gratitude and evolution

This project owes a great debt of gratitude to [André Staltz](https://staltz.com/)'s [callbag](https://github.com/callbag/callbag) pattern. I adapted it to C++ idioms and had the insight that binding a sink to a source doesn't need quite as much ceremony -- it can be as simple as passing an observer function and receiving a pull function. That takes care of two of callbag's semaphores (registration and acknowledgment); the other two (termination and errors) are handled by `Endable` and `Fallible` value types.
