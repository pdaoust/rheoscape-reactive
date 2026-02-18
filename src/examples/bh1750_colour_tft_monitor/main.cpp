#include <fmt/format.h>
#include <rheoscape_everything.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <lvgl.h>

#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#include <drivers/display/tft_espi/lv_tft_espi.h>
#endif

using namespace rheo;
using namespace rheo::helpers;
using namespace rheo::operators;
using namespace rheo::sources;
using namespace rheo::ui;

#ifndef LOG_LEVEL
#define LOG_LEVEL logging::LOG_LEVEL_DEBUG
#endif

#ifndef LOG_LEVEL_LVGL
#define LOG_LEVEL_LVGL logging::LOG_LEVEL_WARN
#endif

// OneWire bus(1);
// DallasTemperature sensors(&bus);
// const uint64_t temp_address = 0x00000000;

const uint8_t i2cSdaPin = 41;
const uint8_t i2cSclPin = 40;

pull_fn pull_temp_and_hum;
pull_fn pull_setpoint;

lv_display_t* disp;
TFT_eSPI tft = TFT_eSPI();

uint32_t draw_buf[TFT_WIDTH * TFT_HEIGHT / 30 * LV_COLOR_DEPTH];

unsigned long last_lvgl_run;
unsigned long time_till_next_lvgl_run;

const uint8_t encoder_btn_pin = 1;
const uint8_t encoder_a_pin = 2;
const uint8_t encoder_b_pin = 42;

int8_t encoder_quarter_clicks = 0;
int8_t encoder_clicks = 0;
uint8_t last_encoder_state = 3;
static const int8_t encoder_states_lookup[]  = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

void touch_read(lv_indev_t* indev, lv_indev_data_t* data) {
  uint16_t t_x = 0, t_y = 0;
  if (tft.get_touch(&t_x, &t_y, 200U)) {
    data->point.x = t_x;
    data->point.y = 240 - t_y;
    data->state = LV_INDEV_STATE_PRESSED;
    logging::debug("touch", fmt::format("x: {}, y: {}", t_x, 240 - t_y).c_str());
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void encoder_interrupt() {
  last_encoder_state <<= 2;
  if (digitalRead(encoder_a_pin)) last_encoder_state |= 0x02;
  if (digitalRead(encoder_b_pin)) last_encoder_state |= 0x01;
  encoder_quarter_clicks += encoder_states_lookup[( last_encoder_state & 0x0f )];
  if (encoder_quarter_clicks > 3) {
    encoder_clicks ++;
    encoder_quarter_clicks = 0;
  } else if (encoder_quarter_clicks < -3) {
    encoder_clicks --;
    encoder_quarter_clicks = 0;
  }
}

void encoder_read(lv_indev_t* indev, lv_indev_data_t* data) {
  // `encoder_clicks` is fed by the IRQ handler.
  data->enc_diff = encoder_clicks;
  // Reset it every time we read the value.
  encoder_clicks = 0;
  // Handle the button by a simple digitalRead.
  if (digitalRead(encoder_btn_pin)) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void log_lvgl_message(lv_log_level_t level, const char * buf) {
  uint8_t rheo_log_level;
  // LVGL's log messages are backwards; map them to our log levels.
  switch (level) {
    case LV_LOG_LEVEL_ERROR: rheo_log_level = logging::LOG_LEVEL_ERROR; break;
    case LV_LOG_LEVEL_WARN:  rheo_log_level = logging::LOG_LEVEL_WARN;  break;
    case LV_LOG_LEVEL_INFO:  rheo_log_level = logging::LOG_LEVEL_INFO;  break;
    case LV_LOG_LEVEL_TRACE: rheo_log_level = logging::LOG_LEVEL_TRACE; break;
    default:                 rheo_log_level = logging::LOG_LEVEL_INFO;  break;
  }
  if (rheo_log_level > LOG_LEVEL_LVGL) {
    return;
  }
  // Trim the newline.
  char trimmed_buf[strlen(buf)];
  strcpy(trimmed_buf, buf);
  trimmed_buf[strlen(buf) - 1] = '\0';
  logging::log(rheo_log_level, "lvgl", trimmed_buf);
}

lv_obj_t* smooth_temp_label;

lv_obj_t* setpoint_editor;

void setup() {
  Serial.begin(115200);
  logging::register_subscriber([](uint8_t log_level, const char* topic, const char* message) {
    if (log_level <= LOG_LEVEL) {
      Serial.print("[");
      Serial.print(LOG_LEVEL_LABEL(log_level));
      if (topic != NULL) {
        Serial.print(":");
        Serial.print(topic);
      }
      Serial.print("] ");
      Serial.println(message);
    }
  });

  logging::debug(NULL, "Starting...");

  // Set up the thermostat.
  logging::debug(NULL, "starting I2C...");
  Wire.begin(i2cSdaPin, i2cSclPin);
  logging::debug(NULL, "instantiating SHT2x...");
  auto clock = from_clock<arduino_millis_clock>();
  auto temp_and_hum = arduino::sht2x::sht2x(&Wire);
  auto temp_and_hum_smooth = temp_and_hum
    | log_errors([](arduino::sht2x::Error error) {
      return arduino::sht2x::format_error(error);
    }, "sht2x")
    | make_infallible()
    | cache()
    | throttle(clock, arduino_millis_clock::duration(250))
    // We've gotta do the averaging in two parts.
    // First the temperature...
    | lift_to_tuple_left(
      exponential_moving_average(
        clock,
        arduino_millis_clock::duration(1000),
        map_chrono_to_scalar<unsigned long, typename arduino_millis_clock::duration>
      )
    )
    /// ... Then the humidity.
    | lift_to_tuple_right(
      exponential_moving_average(
        clock,
        arduino_millis_clock::duration(1000),
        map_chrono_to_scalar<unsigned long, typename arduino_millis_clock::duration>
      )
    );
  /*
  auto setpoint = rheo::State<TempC>(au::celsius_pt(20.0f), false);

  auto empty_style_source = constant(std::vector<lvgl::StyleAndSelector>());

  // Set up the input.
  pin_mode(encoder_a_pin, INPUT_PULLUP);
  pinMode(encoder_b_pin, INPUT_PULLUP);
  pinMode(encoder_btn_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoder_a_pin), encoder_interrupt, CHANGE);
  attachInterrupt(digital_pinToInterrupt(encoder_b_pin), encoder_interrupt, CHANGE);

  // Set up the display.
  lv_init();
  logging::debug(NULL, "LVGL initialised");
  lv_log_register_print_cb(log_lvgl_message);
  logging::debug(NULL, "Log callback setup");
  disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, &draw_buf, sizeof(draw_buf));

  // INPUT DEVICES
  // We need to be able to access the TFT's touch API.
  // So instantiate another copy of the TFT_eSPI driver.
  tft.init();
  lv_indev_t* touch = lv_indev_create();
  logging::debug(NULL, "Touch created");
  lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
  logging::debug(NULL, "Touch type set");
  lv_indev_set_read_cb(touch, touch_read);

  // Set up the rotary encoder.
  lv_indev_t* encoder = lv_indev_create();
  logging::debug(NULL, "Encoder created");
  lv_indev_set_type(encoder, LV_INDEV_TYPE_ENCODER);
  logging::debug(NULL, "Encoder type set");
  lv_indev_set_read_cb(encoder, encoder_read);
  logging::debug(NULL, "Encoder callback bound");
  last_lvgl_run = millis();
  logging::debug(NULL, "Display setup complete!");

  // // Create the UI.
  lv_obj_t* ui_container = lv_obj_create(lv_screen_active());
  lv_obj_set_width(ui_container, 320);
  lv_obj_set_height(ui_container, 240);
  lv_obj_set_layout(ui_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(ui_container, LV_FLEX_FLOW_COLUMN);

  // Temp/humidity chart.
  lv_obj_t* chart = lv_chart_create(ui_container);
  lv_obj_set_size(chart, LV_PCT(100), 100);
  lv_obj_center(chart);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, -100, 500);
  lv_chart_set_axis_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
  lv_chart_set_point_count(chart, 80);
  lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
  lv_chart_series_t* temp_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_series_t* hum_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);

  // Current temp/humidity.
  lv_obj_t* temp_hum_wrapper = lv_obj_create(ui_container);
  lv_obj_remove_style_all(temp_hum_wrapper);
  lv_obj_set_size(temp_hum_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(temp_hum_wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temp_hum_wrapper, LV_FLEX_FLOW_ROW);
  lv_obj_t* temp_label = lv_label_create(temp_hum_wrapper);
  lv_obj_set_flex_grow(temp_label, 1);
  lv_obj_set_style_text_color(temp_label, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_set_style_text_align(temp_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_t* temp_hum_divider = lv_label_create(temp_hum_wrapper);
  lv_obj_set_width(temp_hum_divider, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(temp_hum_divider, 0);
  lv_label_set_text(temp_hum_divider, " | ");
  lv_obj_t* hum_label = lv_label_create(temp_hum_wrapper);
  lv_obj_set_flex_grow(hum_label, 1);
  lv_obj_set_style_text_color(hum_label, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_text_align(hum_label, LV_TEXT_ALIGN_LEFT, 0);
  */

  pull_temp_and_hum = temp_and_hum_smooth
    | foreach([chart, temp_series, hum_series, temp_label, hum_label](arduino::sht2x::Reading value) {
      logging::debug("chart", fmt::format("writing temp {} and hum {}", std::get<0>(value).in(au::Celsius{}), std::get<1>(value).in(au::Percent{})).c_str());
      lv_chart_set_next_value(chart, temp_series, (int32_t)(std::get<0>(value).in(au::Celsius{}) * 10));
      lv_chart_set_next_value(chart, hum_series, (int32_t)(round(std::get<1>(value).in(au::Percent{}))));
      lv_chart_refresh(chart);
      lv_label_set_text(temp_label, au_to_string(std::get<0>(value), 1).c_str());
      lv_label_set_text(hum_label, fmt::format("{} RH", au_to_string(std::get<1>(value), 0).c_str()).c_str());
    });

  // lv_obj_t* hum_label = lv_label_create(ui_container);
  // lv_label_set_text(hum_label, "Starting...");
  // lv_obj_align(hum_label, LV_ALIGN_TOP_LEFT, 0, 0);
  // auto map_hum_to_label = map<std::string, Percent>(infallible_hum, [](Percent v) {
  //   return string_format("Humidity: %s", au_to_string(v, 2).c_str());
  // });
  // //map_hum_to_label = tee(map_hum_to_label, rheo::sinks::arduino::serial_string_line_sink());
  // auto pull_and_source_hum_label = lvgl::label(hum_label, map_hum_to_label, empty_style_source);
  // pull_hum = std::get<0>(pull_and_source_hum_label);

  // smooth_temp_label = lv_label_create(ui_container);
  // lv_label_set_text(smooth_temp_label, "Starting...");
  // lv_obj_align(smooth_temp_label, LV_ALIGN_LEFT_MID, 0, 0);
  // auto map_smooth_temp_to_label = map<std::string, TempC>(smooth_temp, [](TempC v) {
  //   return string_format("Temperature: %s", au_to_string(v, 2).c_str());
  // });
  // //map_smooth_temp_to_label = tee(map_smooth_temp_to_label, rheo::sinks::arduino::serial_string_line_sink());
  // auto pull_and_source_smooth_temp_label = lvgl::label(smooth_temp_label, map_smooth_temp_to_label, empty_style_source);
  // pull_smooth_temp = std::get<0>(pull_and_source_smooth_temp_label);
  // Now to step on a skinned cat who wears a stupid green hat.
  // That's Japanese for :(
  logging::debug(NULL, "Setting up setpoint editor...");
  lv_obj_t* setpoint_container = lv_obj_create(ui_container);
  lv_obj_remove_style_all(setpoint_container);
  lv_obj_set_size(setpoint_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(setpoint_container, 0, 0);
  lv_obj_set_style_border_width(setpoint_container, 0, 0);
  lv_obj_set_layout(setpoint_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(setpoint_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_flex_cross_place(setpoint_container, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_t* setpoint_label = lv_label_create(setpoint_container);
  lv_label_set_text(setpoint_label, "Setpoint");
  lv_obj_t* setpoint_minus_button = lv_button_create(setpoint_container);
  lv_obj_set_size(setpoint_minus_button, 20, 20);
  lv_obj_set_style_bg_image_src(setpoint_minus_button, LV_SYMBOL_MINUS, 0);
  lv_obj_add_event_cb(
    setpoint_minus_button,
    [](lv_event_t* e){
      lv_event_code_t code = lv_event_get_code(e);
      if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_decrement(setpoint_editor);
      }
    },
    LV_EVENT_ALL, NULL
  );
  setpoint_editor = lv_spinbox_create(setpoint_container);
  lv_spinbox_set_value(setpoint_editor, 20);
  lv_spinbox_set_digit_format(setpoint_editor, 3, 0);
  lv_obj_t* setpoint_plus_button = lv_button_create(setpoint_container);
  lv_obj_set_size(setpoint_plus_button, 20, 20);
  lv_obj_set_style_bg_image_src(setpoint_plus_button, LV_SYMBOL_PLUS, 0);
  lv_obj_add_event_cb(
    setpoint_plus_button,
    [](lv_event_t* e){
      lv_event_code_t code = lv_event_get_code(e);
      if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_increment(setpoint_editor);
      }
    },
    LV_EVENT_ALL, NULL
  );
  setpoint.add_sink(
    [](auto value) {
      logging::debug("setpoint", fmt::format("Setting setpoint to {}", au_to_string(value, 2)).c_str());
    },
    [](){}
  );
  auto setpoint_c = map(setpoint.get_source_fn(false), [](TempC value) { return value.in(au::Celsius{}); });
  auto pull_and_source_setpoint_editor = lvgl::spinbox(setpoint_editor, setpoint_c, empty_style_source);
  // auto setpoint_editor_source_fn = std::get<1>(pull_and_source_setpoint_editor);
  // setpoint_editor_source_fn(
  //   [&setpoint](lv_event_code_t event) {
  //     if (event == LV_EVENT_VALUE_CHANGED) {
  //       Serial.println("Handling setpoint editing event");
  //       setpoint.set(au::celsius_pt((float)lv_spinbox_get_value(setpoint_editor)), false);
  //     }
  //   }
  // );
  lv_obj_t* setpoint_units_label = lv_label_create(setpoint_container);
  lv_label_set_text(setpoint_units_label, "°C");
  logging::debug(NULL, "Setup completed!");
}

int16_t last_read_encoder_clicks = 0;

void loop() {
  logging::trace(NULL, "Pulling temp and hum...");
  pull_temp_and_hum();

  unsigned long now = millis();
  if (last_lvgl_run + time_till_next_lvgl_run <= now) {
    time_till_next_lvgl_run = lv_timer_handler();
    if (time_till_next_lvgl_run == LV_NO_TIMER_READY) {
      time_till_next_lvgl_run = LV_DEF_REFR_PERIOD;
    } else {
      lv_tick_inc(now - last_lvgl_run);
      last_lvgl_run = now;
    }
  }
}