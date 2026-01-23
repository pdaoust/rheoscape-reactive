#include <atomic>
#include <cstdarg>
#include <fmt/format.h>
#include <rheoscape_everything.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#ifdef PLATFORM_RP2040
#include <SerialUSB.h>
#endif
#include <Wire.h>
#include <Adafruit_SSD1306.h>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sinks;
using namespace rheo::sinks::arduino;
using namespace rheo::sinks::arduino::gfx;
using namespace rheo::sources;

#ifndef LOG_LEVEL
#define LOG_LEVEL logging::LOG_LEVEL_DEBUG
#endif

const uint8_t i2cSdaPin = 8;
const uint8_t i2cSclPin = 9;

const uint8_t encoder_btn_pin = 7;
const uint8_t encoder_a_pin = 10;
const uint8_t encoder_b_pin = 11;

const uint8_t servo_pin = 2;
const uint8_t red_pwm_pin = 12;
const uint8_t blue_pwm_pin = 13;
const uint32_t laser_pwm_frequency = 1000;

using EncoderEvent = std::variant<std::monostate, int8_t>;

EventSource<EncoderEvent> encoder_events;

enum UiState {
  EDIT_SERVO_SWEEP_DURATION,
  EDIT_SERVO_MIN_ANGLE,
  EDIT_SERVO_MAX_ANGLE,
  EDIT_RED_PWM_DUTY_CYCLE,
  EDIT_BLUE_PWM_DUTY_CYCLE,
  // Hack to introspect the number of UI states.
  _count
};

int8_t encoder_quarter_clicks = 0;
std::atomic<int8_t> encoder_clicks = 0;
uint8_t last_encoder_knob_state = 3;
static const int8_t encoder_states_lookup[]  = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

void encoder_knob_interrupt() {
  // IMPORTANT: No Serial I/O in interrupt handlers - it's too slow and causes lockups!
  // Read both pins together to minimize timing skew.
  uint8_t current_state = (digitalRead(encoder_a_pin) << 1) | digitalRead(encoder_b_pin);
  uint8_t combined = ((last_encoder_knob_state & 0x03) << 2) | current_state;
  last_encoder_knob_state = current_state;
  encoder_quarter_clicks += encoder_states_lookup[combined];
  if (encoder_quarter_clicks > 3) {
    encoder_clicks.fetch_add(1, std::memory_order_relaxed);
    encoder_quarter_clicks = 0;
  } else if (encoder_quarter_clicks < -3) {
    encoder_clicks.fetch_add(-1, std::memory_order_relaxed);
    encoder_quarter_clicks = 0;
  }
}

std::atomic<uint8_t> encoder_button_released_count = 0;

void encoder_button_interrupt() {
  encoder_button_released_count ++;
}

pull_fn pull_servo;
pull_fn pull_display;
pull_fn pull_red_pwm;
pull_fn pull_blue_pwm;
pull_fn pull_heartbeat;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

extern "C" void __attribute__((weak)) panic(const char *fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  Serial.flush();
  pinMode(25, OUTPUT);
  while (1) {
    // Rapid blink = panic
    digitalWrite(25, HIGH);
    delay(50);
    digitalWrite(25, LOW);
    delay(50);
  }
}

void setup() {
  pinMode(25, OUTPUT);
  Serial.begin(115200);
  // logging::register_subscriber([](uint8_t log_level, std::optional<std::string> topic, std::string message) {
  //   if (log_level <= LOG_LEVEL) {
  //     Serial.print("[");
  //     Serial.print(LOG_LEVEL_LABEL(log_level));
  //     if (topic.has_value()) {
  //       Serial.print(":");
  //       Serial.print(topic.value().c_str());
  //     }
  //     Serial.print("] ");
  //     Serial.println(message.c_str());
  //     Serial.flush();
  //   }
  // });
  Serial.println("Starting..."); Serial.flush();
  Serial.println("Starting I2C..."); Serial.flush();
  Wire.begin();
  Serial.println("I2C done."); Serial.flush();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  }
  display.display();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.print("Hello!");
  display.display();
  delay(2000);

  pinMode(encoder_btn_pin, INPUT_PULLUP);
  pinMode(encoder_a_pin, INPUT_PULLUP);
  pinMode(encoder_b_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoder_a_pin), encoder_knob_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder_b_pin), encoder_knob_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder_btn_pin), encoder_button_interrupt, RISING);

  static auto system_clock_source = from_clock<arduino_millis_clock>()
    | map(to_duration<arduino_millis_clock>)
    | map(duration_to_clock<float_millis_clock, arduino_millis_clock>);

  // Annoyingly the au::degrees quantity maker clashes with the Arduino library's converter.
  // I thought that's what namespaces were supposed to prevent...
  // Use unsigned char since that's what servo_sink expects; normalize handles the conversion.
  static State servo_angle_range(Range(au::make_quantity_point<au::Degrees, unsigned char>(0), au::make_quantity_point<au::Degrees, unsigned char>(180)));
  static State red_pwm_duty_cycle(au::percent(100));
  static State blue_pwm_duty_cycle(au::percent(100));
  static State servo_sweep_duration(float_millis_clock::duration(5000.0f));

  // Connect the servo sweep duration and angle range to the servo.
  auto servo_position = triangle_wave(
    system_clock_source,
    servo_sweep_duration.get_source_fn(),
    constant(float_millis_clock::duration(0.0f))
  )
    | normalize(constant(Range(-1.0f, 1.0f)), servo_angle_range.get_source_fn());
  pull_servo = servo_position | servo_sink(servo_pin);

  // Connect the PWM settings to the lasers.
  pull_red_pwm = red_pwm_duty_cycle.get_source_fn()
    | map([](au::Quantity<au::Percent, int> v) { return (int)((float)v.in(au::Percent{}) / 100 * 1023); })
    | analog_pin_sink(red_pwm_pin, 10, laser_pwm_frequency);
  pull_blue_pwm = blue_pwm_duty_cycle.get_source_fn()
    | map([](au::Quantity<au::Percent, int> v) { return (int)((float)v.in(au::Percent{}) / 100 * 1023); })
    | analog_pin_sink(blue_pwm_pin, 10, laser_pwm_frequency);

  // Set up the editors.
  static State<UiState> ui_state(EDIT_SERVO_SWEEP_DURATION);

  // Change to edit next UI state on encoder button release.
  encoder_events.get_source_fn()
    | filter_map([](EncoderEvent event) -> std::optional<std::monostate> {
        if (std::holds_alternative<std::monostate>(event)) {
          return std::monostate{};
        }
        return std::nullopt;
      })
    | throttle<std::monostate>(system_clock_source, float_millis_clock::duration(50.0))
    | sample<std::monostate>(ui_state.get_source_fn())
    | map([](UiState value) {
        return (UiState)((value + 1) % UiState::_count);
      })
    | ui_state.get_setter_sink_fn();

  // Depending on what element of the UI is being edited, apply encoder turns to that element.
  static auto ui_editing = encoder_events.get_source_fn()
    | filter_map([](EncoderEvent event) -> std::optional<int8_t> {
        if (std::holds_alternative<int8_t>(event)) {
          return std::get<int8_t>(event);
        }
        return std::nullopt;
      })
    | combine_with_boop<int8_t>(ui_state.get_source_fn());

  // Servo range.
  ui_editing
    | filter([](std::tuple<int8_t, UiState> value) {
        return std::get<1>(value) == EDIT_SERVO_MIN_ANGLE || std::get<1>(value) == EDIT_SERVO_MAX_ANGLE;
      })
    | combine_with([](std::tuple<int8_t, UiState> delta, Range<au::QuantityPoint<au::Degrees, unsigned char>> range) {
        auto clicks = std::get<0>(delta);
        UiState state = std::get<1>(delta);

        if (state == EDIT_SERVO_MIN_ANGLE) {
          // Compute in int to avoid overflow, then clamp and cast back.
          int new_min_raw = static_cast<int>(range.min.in(au::Degrees{})) + clicks;
          if (new_min_raw >= 0 && new_min_raw <= range.max.in(au::Degrees{})) {
            return Range<au::QuantityPoint<au::Degrees, unsigned char>>{
              au::make_quantity_point<au::Degrees, unsigned char>(static_cast<unsigned char>(new_min_raw)),
              range.max
            };
          }
        } else if (state == EDIT_SERVO_MAX_ANGLE) {
          int new_max_raw = static_cast<int>(range.max.in(au::Degrees{})) + clicks;
          if (new_max_raw >= range.min.in(au::Degrees{}) && new_max_raw <= 180) {
            return Range<au::QuantityPoint<au::Degrees, unsigned char>>{
              range.min,
              au::make_quantity_point<au::Degrees, unsigned char>(static_cast<unsigned char>(new_max_raw))
            };
          }
        }
        // Should be unreachable.
        return range;
      }, servo_angle_range.get_source_fn())
    | servo_angle_range.get_setter_sink_fn();

  // Red PWM duty cycle.
  ui_editing
    | filter([](std::tuple<int8_t, UiState> value) {
        return std::get<1>(value) == EDIT_RED_PWM_DUTY_CYCLE;
      })
    | combine_with([](std::tuple<int8_t, UiState> delta, au::Quantity<au::Percent, int> duty_cycle) {
        int8_t clicks = std::get<0>(delta);
        // Jump in 10% increments.
        auto new_duty_cycle = duty_cycle + au::percent(clicks * 10);
        if (new_duty_cycle >= au::percent(0) && new_duty_cycle <= au::percent(100)) {
          return new_duty_cycle;
        }
        return duty_cycle;
      }, red_pwm_duty_cycle.get_source_fn())
    | red_pwm_duty_cycle.get_setter_sink_fn();

  // Blue PWM duty cycle.
  ui_editing
    | filter([](std::tuple<int8_t, UiState> value) {
        return std::get<1>(value) == EDIT_BLUE_PWM_DUTY_CYCLE;
      })
    | combine_with([](std::tuple<int8_t, UiState> delta, au::Quantity<au::Percent, int> duty_cycle) {
        int8_t clicks = std::get<0>(delta);
        auto new_duty_cycle = duty_cycle + au::percent(clicks * 10);
        if (new_duty_cycle >= au::percent(0) && new_duty_cycle <= au::percent(100)) {
          return new_duty_cycle;
        }
        return duty_cycle;
      }, blue_pwm_duty_cycle.get_source_fn())
    | blue_pwm_duty_cycle.get_setter_sink_fn();

  // Servo sweep duration.
  ui_editing
    | filter([](std::tuple<int8_t, UiState> value) {
        return std::get<1>(value) == EDIT_SERVO_SWEEP_DURATION;
      })
    | combine_with([](std::tuple<int8_t, UiState> delta, float_millis_clock::duration duration) {
        int8_t clicks = std::get<0>(delta);
        auto new_duration = duration + float_millis_clock::duration(clicks * 100);
        if (new_duration.count() >= 2000 && new_duration.count() <= 60000) {
          return new_duration;
        }
        return duration;
      }, servo_sweep_duration.get_source_fn())
    | servo_sweep_duration.get_setter_sink_fn();

  // Connect the display to the UI state and the settings.
  pull_display = combine(
    [](
      UiState state,
      Range<au::QuantityPoint<au::Degrees, unsigned char>> angle_range,
      au::Quantity<au::Percent, int> red_duty,
      au::Quantity<au::Percent, int> blue_duty,
      float_millis_clock::duration sweep_duration,
      au::QuantityPoint<au::Degrees, uint8_t> current_sweep_position
    ) {
      // Explicitly construct GfxCommand to work around GCC 14 consteval escalation bug
      // with capturing lambdas in std::function.
      GfxCommand<Adafruit_GFX> cmd = [state, sweep_duration, angle_range, red_duty, blue_duty, current_sweep_position](Adafruit_GFX& canvas) {
        canvas.setTextColor(SSD1306_WHITE);
        canvas.setCursor(0, 0);

        canvas.print("Sweep time: ");
        if (state == EDIT_SERVO_SWEEP_DURATION) {
          canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        }
        canvas.print(fmt::format("{:.0f}", sweep_duration.count()).c_str());
        if (state == EDIT_SERVO_SWEEP_DURATION) {
          canvas.setTextColor(SSD1306_WHITE);
        }
        canvas.println("ms");

        canvas.print("Angle range: ");
        if (state == EDIT_SERVO_MIN_ANGLE) {
          canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        }
        canvas.print(angle_range.min.in(au::Degrees{}));
        if (state == EDIT_SERVO_MIN_ANGLE) {
          canvas.setTextColor(SSD1306_WHITE);
        }
        canvas.print("-");
        if (state == EDIT_SERVO_MAX_ANGLE) {
          canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        }
        canvas.print(angle_range.max.in(au::Degrees{}));
        if (state == EDIT_SERVO_MAX_ANGLE) {
          canvas.setTextColor(SSD1306_WHITE);
        }
        canvas.println("\367");

        canvas.print("Red: ");
        if (state == EDIT_RED_PWM_DUTY_CYCLE) {
          canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        }
        canvas.print(red_duty.in(au::Percent{}));
        if (state == EDIT_RED_PWM_DUTY_CYCLE) {
          canvas.setTextColor(SSD1306_WHITE);
        }
        canvas.println("%");

        canvas.print("Blue: ");
        if (state == EDIT_BLUE_PWM_DUTY_CYCLE) {
          canvas.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        }
        canvas.print(blue_duty.in(au::Percent{}));
        if (state == EDIT_BLUE_PWM_DUTY_CYCLE) {
          canvas.setTextColor(SSD1306_WHITE);
        }
        canvas.println("%");
        canvas.println("");

        canvas.print("Current pos: ");
        canvas.print(current_sweep_position.in(au::Degrees{}));
        canvas.println("\367");
      };
      return std::vector<GfxCommand<Adafruit_GFX>>{std::move(cmd)};
    },
    ui_state.get_source_fn(),
    servo_angle_range.get_source_fn(),
    red_pwm_duty_cycle.get_source_fn(),
    blue_pwm_duty_cycle.get_source_fn(),
    servo_sweep_duration.get_source_fn(),
    servo_position
  )
  | ssd1306_sink(display);

  pull_heartbeat = sine_wave(
    system_clock_source,
    constant(float_millis_clock::duration(2000)),
    constant(float_millis_clock::duration(0))
  )
  // Zero-bias, then stretch to 10-bit PWM range.
  | map([](float v) { return (v + 1.0f) / 2 * 1023; })
  | analog_pin_sink(25, 10);

  Serial.println("=== SETUP COMPLETE ==="); Serial.flush();
}

bool last_read_encoder_button_state = HIGH;

void loop() {
  // Read encoder state and emit events.
  if (encoder_button_released_count > 0) {
    // Button has been released at least once.
    for (int i = 0; i < encoder_button_released_count; i ++) {
      encoder_events.push(std::monostate{});
    }
    encoder_button_released_count = 0;
  }
  if (encoder_clicks) {
    int8_t clicks = encoder_clicks.exchange(0);
    encoder_events.push(clicks);
  }

  pull_servo();
  pull_red_pwm();
  pull_blue_pwm();
  pull_display();
  pull_heartbeat();
}
