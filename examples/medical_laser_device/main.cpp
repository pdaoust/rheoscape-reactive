#include <atomic>
#include <cstdarg>
#include <fmt/format.h>
#include <rheoscape.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#if defined(ARDUINO_ARCH_RP2040)
#include <SerialUSB.h>
#endif
#include <Wire.h>
#include <Adafruit_SSD1306.h>

using namespace rheoscape;
using namespace rheoscape::helpers;
using namespace rheoscape::operators;
using namespace rheoscape::util;
using namespace rheoscape::sinks;
using namespace rheoscape::sinks::arduino;
using namespace rheoscape::sinks::arduino::gfx;
using namespace rheoscape::sources;
using namespace rheoscape::sources::arduino;
using namespace rheoscape::states;

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

const int settings_address = 0;

pull_fn pull_servo;
pull_fn pull_ui_state;
pull_fn pull_display;
pull_fn pull_red_pwm;
pull_fn pull_blue_pwm;
pull_fn pull_heartbeat;

enum UiState {
  EDIT_SERVO_SWEEP_DURATION,
  EDIT_SERVO_MIN_ANGLE,
  EDIT_SERVO_MAX_ANGLE,
  EDIT_RED_PWM_DUTY_CYCLE,
  EDIT_BLUE_PWM_DUTY_CYCLE,
  // Hack to introspect the number of UI states.
  _count
};


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

  static auto system_clock_source = from_clock<arduino_millis_clock>()
    | map(to_duration<arduino_millis_clock>)
    | map(duration_to_clock<float_millis_clock, arduino_millis_clock>);

  // Annoyingly the au::degrees quantity maker clashes with the Arduino library's converter.
  // I thought that's what namespaces were supposed to prevent...
  // Use unsigned char since that's what servo_sink expects; normalize handles the conversion.
  static MemoryState servo_angle_range(Range(au::make_quantity_point<au::Degrees, unsigned char>(35), au::make_quantity_point<au::Degrees, unsigned char>(100)));
  static MemoryState red_pwm_duty_cycle(au::percent(100));
  static MemoryState blue_pwm_duty_cycle(au::percent(100));
  static MemoryState servo_sweep_duration(float_millis_clock::duration(5000.0f));
  static MemoryState programme_index(1);

  // Load settings from flash.
  using programme_t = std::tuple<Range<au::QuantityPoint<au::Degrees, unsigned char>>, au::Quantity<au::Percent, int>, au::Quantity<au::Percent, int>, float_millis_clock::duration>;
  using settings_t = std::tuple<programme_t, programme_t>;

  auto load_settings = eeprom_source<settings_t>(settings_address);

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
  static MemoryState<UiState> ui_state(EDIT_SERVO_SWEEP_DURATION);

  // First, the button and encoder.
  auto encoder_source = digital_pin_interrupt_source<encoder_a_pin, encoder_b_pin>(INPUT_PULLUP)
    | quadrature_encode();
  auto encoder_button_source = digital_pin_source(encoder_btn_pin, INPUT_PULLUP)
    | debounce(system_clock_source, float_millis_clock::duration(50.0f))
    // We only want release events.
    // Turn continuous values into change events.
    | dedupe()
    // Then only take truthy values (released is HIGH).
    | filter([](bool v) { return v == HIGH; });

  // Change to edit next UI state on encoder button release.
  pull_ui_state = make_state_editor(encoder_button_source, ui_state, [](bool _, UiState value) {
    return (UiState)((value + 1) % UiState::_count);
  });

  // Depending on what element of the UI is being edited, apply encoder turns to that element.
  static auto ui_editing = encoder_source
    | combine_with(ui_state.get_source_fn());

  // Servo range.
  pull_edit_servo_range = make_state_editor(
    ui_editing
      | filter([](QuadratureEncodeDirection dir, UiState ui_state) {
        return ui_state == EDIT_SERVO_MIN_ANGLE || ui_state == EDIT_SERVO_MAX_ANGLE;
      }),
    servo_angle_range,
    [](std::tuple<QuadratureEncodeDirection, UiState> ui, Range<au::QuantityPoint<au::Degrees, unsigned char>> range) {
      QuadratureEncodeDirection dir = std::get<0>(ui);
      UiState state = std::get<1>(ui);

      if (state == EDIT_SERVO_MIN_ANGLE) {
        // Compute in int to avoid overflow, then clamp and cast back.
        int new_min_raw = static_cast<int>(range.min.in(au::Degrees{})) + dir;
        if (new_min_raw >= 0 && new_min_raw <= range.max.in(au::Degrees{})) {
          return Range<au::QuantityPoint<au::Degrees, unsigned char>>{
            au::make_quantity_point<au::Degrees, unsigned char>(static_cast<unsigned char>(new_min_raw)),
            range.max
          };
        }
      } else if (state == EDIT_SERVO_MAX_ANGLE) {
        int new_max_raw = static_cast<int>(range.max.in(au::Degrees{})) + dir;
        if (new_max_raw >= range.min.in(au::Degrees{}) && new_max_raw <= 180) {
          return Range<au::QuantityPoint<au::Degrees, unsigned char>>{
            range.min,
            au::make_quantity_point<au::Degrees, unsigned char>(static_cast<unsigned char>(new_max_raw))
          };
        }
      }
      // Should be unreachable.
      return range;
    }
  )

  // Red PWM duty cycle.
  pull_edit_red_duty = make_state_editor(
    ui_editing | filter_map([](QuadratureEncodeDirection dir, UiState ui) {
      return ui == EDIT_RED_PWM_DUTY_CYCLE
        ? dir
        : std::nullopt;
    }),
    red_pwm_duty_cycle,
    [](QuadratureEncodeDirection dir, au::Quantity<au::Percent, int> duty_cycle) {
      // Jump in 10% increments.
      auto new_duty_cycle = duty_cycle + au::percent(dir * 10);
      if (new_duty_cycle >= au::percent(0) && new_duty_cycle <= au::percent(100)) {
        return new_duty_cycle;
      }
      return duty_cycle;
    });

  // Blue PWM duty cycle.
  pull_edit_blue_duty = make_state_editor(
    ui_editing | filter_map([](QuadratureEncodeDirection dir, UiState ui) {
      return ui == EDIT_BLUE_PWM_DUTY_CYCLE
        ? dir
        : std::nullopt;
    }),
    blue_pwm_duty_cycle,
    [](QuadratureEncodeDirection dir, au::Quantity<au::Percent, int> duty_cycle) {
      // Jump in 10% increments.
      auto new_duty_cycle = duty_cycle + au::percent(dir * 10);
      if (new_duty_cycle >= au::percent(0) && new_duty_cycle <= au::percent(100)) {
        return new_duty_cycle;
      }
      return duty_cycle;
    }
  );

  // Servo sweep duration.
  pull_edit_servo_sweep = make_state_editor(
    ui_editing | filter_map([](QuadratureEncodeDirection dir, UiState ui) {
      return ui == EDIT_SERVO_SWEEP_DURATION
        ? dir
        : std::nullopt;
    }),
    servo_sweep_duration,
    [](QuadratureEncodeDirection dir, float_millis_clock::duration duration) {
      auto new_duration = duration + float_millis_clock::duration(dir * 100);
      if (new_duration.count() >= 2000 && new_duration.count() <= 60000) {
        return new_duration;
      }
      return duration;
    }
  );

  // Connect the display to the UI state and the settings.
  pull_display = combine(
    ui_state.get_source_fn(),
    servo_angle_range.get_source_fn(),
    red_pwm_duty_cycle.get_source_fn(),
    blue_pwm_duty_cycle.get_source_fn(),
    servo_sweep_duration.get_source_fn(),
    servo_position
  )
  | map([](
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
    })
  | ssd1306_sink(display);

  pull_heartbeat = sine_wave(
    system_clock_source,
    constant(float_millis_clock::duration(2000))
  )
  // Zero-bias, then stretch to 10-bit PWM range.
  | map([](float v) -> int { return (v + 1.0f) / 2 * 1023; })
  | analog_pin_sink(25, 10);

  Serial.println("=== SETUP COMPLETE ==="); Serial.flush();
}

bool last_read_encoder_button_state = HIGH;

void loop() {
  pull_servo();
  pull_red_pwm();
  pull_blue_pwm();
  pull_display();
  pull_heartbeat();
}
