#include <rheoscape_everything.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <lvgl.h>

#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#include <drivers/display/tft_espi/lv_tft_espi.h>
#endif

using namespace rheo;
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
// const uint64_t tempAddress = 0x00000000;

const uint8_t i2cSdaPin = 41;
const uint8_t i2cSclPin = 40;

pull_fn pullTempAndHum;
pull_fn pullSetpoint;

lv_display_t* disp;
TFT_eSPI tft = TFT_eSPI();

uint32_t drawBuf[TFT_WIDTH * TFT_HEIGHT / 30 * LV_COLOR_DEPTH];

unsigned long lastLvglRun;
unsigned long timeTillNextLvglRun;

const uint8_t encoderBtnPin = 1;
const uint8_t encoderAPin = 2;
const uint8_t encoderBPin = 42;

int8_t encoderQuarterClicks = 0;
int8_t encoderClicks = 0;
uint8_t lastEncoderState = 3;
static const int8_t encoderStatesLookup[]  = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

void touchRead(lv_indev_t* indev, lv_indev_data_t* data) {
  uint16_t t_x = 0, t_y = 0;
  if (tft.getTouch(&t_x, &t_y, 200U)) {
    data->point.x = t_x;
    data->point.y = 240 - t_y;
    data->state = LV_INDEV_STATE_PRESSED;
    logging::debug("touch", "x: %d, y: %d", &t_x, &t_y);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void encoderInterrupt() {
  lastEncoderState <<= 2;
  if (digitalRead(encoderAPin)) lastEncoderState |= 0x02;
  if (digitalRead(encoderBPin)) lastEncoderState |= 0x01;
  encoderQuarterClicks += encoderStatesLookup[( lastEncoderState & 0x0f )];
  if (encoderQuarterClicks > 3) {
    encoderClicks ++;
    encoderQuarterClicks = 0;
  } else if (encoderQuarterClicks < -3) {
    encoderClicks --;
    encoderQuarterClicks = 0;
  }
}

void encoderRead(lv_indev_t* indev, lv_indev_data_t* data) {
  // `encoderClicks` is fed by the IRQ handler.
  data->enc_diff = encoderClicks;
  // Reset it every time we read the value.
  encoderClicks = 0;
  // Handle the button by a simple digitalRead.
  if (digitalRead(encoderBtnPin)) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void logLvglMessage(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  uint8_t rheoLogLevel;
  // LVGL's log messages are backwards; map them to our log levels.
  switch (level) {
    case LV_LOG_LEVEL_ERROR: rheoLogLevel = logging::LOG_LEVEL_ERROR; break;
    case LV_LOG_LEVEL_WARN:  rheoLogLevel = logging::LOG_LEVEL_WARN;  break;
    case LV_LOG_LEVEL_INFO:  rheoLogLevel = logging::LOG_LEVEL_INFO;  break;
    case LV_LOG_LEVEL_TRACE: rheoLogLevel = logging::LOG_LEVEL_TRACE; break;
    default:                 rheoLogLevel = logging::LOG_LEVEL_INFO;  break;
  }
  if (rheoLogLevel > LOG_LEVEL_LVGL) {
    return;
  }
  // Trim the newline.
  char trimmedBuf[strlen(buf)];
  strcpy(trimmedBuf, buf);
  trimmedBuf[strlen(buf) - 1] = '\0';
  logging::log(rheoLogLevel, "lvgl", trimmedBuf);
}

lv_obj_t* smoothTempLabel;

lv_obj_t* setpointEditor;

void setup() {
  Serial.begin(115200);
  logging::registerSubscriber([](uint8_t logLevel, const char* topic, const char* message) {
    if (logLevel <= LOG_LEVEL) {
      Serial.print("[");
      Serial.print(LOG_LEVEL_LABEL(logLevel));
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
  logging::debug(NULL, "instantiating SHT21...");
  // Because we've got something pulling for temp and something pulling for humidity,
  // the first one to pull will drain the read value.
  // Cache it so it can be pulled constantly by either one.
  // Jerk! You're a mean jerk! You're made of beef jerky! You're a beefy jerk!
  // Some of us eat donuts, and some of us don't.
  // Some of us switch to daylight savings time, and some of us just won't.
  // All of these differences would tear us apart, if us strong bond weren't there.
  // Fortunately, there is one thing all of us like to share.
  // What do all Canadians do together? (We talk about the weather!)
  // What do all Canadians do together? (We talk about the weather!)
  // What do all Canadians do together? (We talk about the weather!)
  auto clock = fromClock<arduino_millis_clock>();
  auto tempAndHum = arduino::sht21::sht21(&Wire);
  auto infallibleTempAndHum = makeInfallibleAndLogErrors(
    tempAndHum,
    fp2sf(arduino::sht21::formatError),
    "sht2x"
  );
  auto tempAndHumSmooth = Pipe(arduino::sht21::sht21(&Wire))
    .pipe(makeInfallibleAndLogErrors<arduino::sht21::Sht2xReading, arduino::sht21::Sht2xError>(fp2sf(arduino::sht21::formatError), "sht2x"))
    .pipe(cache<arduino::sht21::Sht2xReading>())
    .pipe(throttle<arduino::sht21::Sht2xReading>(clock, arduino_millis_clock::duration(250)))
    .pipe(splitAndZip<arduino::sht21::Sht2xReading, arduino::sht21::Sht2xTemperature, arduino::sht21::Sht2xHumidity>(
      [](arduino::sht21::Sht2xReading value) { return std::get<0>(value); },
      exponentialMovingAverage<arduino::sht21::Sht2xTemperature, typename arduino_millis_clock::time_point, typename arduino_millis_clock::duration, float>(
        clock,
        arduino_millis_clock::duration(1000),
        mapChronoToScalar<float, typename arduino_millis_clock::duration>
      ),
      [](arduino::sht21::Sht2xReading value) { return std::get<1>(value); },
      exponentialMovingAverage<arduino::sht21::Sht2xHumidity, typename arduino_millis_clock::time_point, typename arduino_millis_clock::duration, float>(
        clock,
        arduino_millis_clock::duration(1000),
        mapChronoToScalar<float, typename arduino_millis_clock::duration>
      ),
      [](arduino::sht21::Sht2xTemperature temp, arduino::sht21::Sht2xHumidity hum) { return arduino::sht21::Sht2xReading(temp, hum); }
    ));
  auto setpoint = rheo::State<TempC>(au::celsius_pt(20.0f), false);

  auto emptyStyleSource = constant(std::vector<lvgl::StyleAndSelector>());

  // Set up the input.
  pinMode(encoderAPin, INPUT_PULLUP);
  pinMode(encoderBPin, INPUT_PULLUP);
  pinMode(encoderBtnPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderAPin), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderBPin), encoderInterrupt, CHANGE);

  // Set up the display.
  lv_init();
  logging::debug(NULL, "LVGL initialised");
  lv_log_register_print_cb(logLvglMessage);
  logging::debug(NULL, "Log callback setup");
  disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, &drawBuf, sizeof(drawBuf));

  // INPUT DEVICES
  // We need to be able to access the TFT's touch API.
  // So instantiate another copy of the TFT_eSPI driver.
  tft.init();
  lv_indev_t* touch = lv_indev_create();
  logging::debug(NULL, "Touch created");
  lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
  logging::debug(NULL, "Touch type set");
  lv_indev_set_read_cb(touch, touchRead);

  // Set up the rotary encoder.
  lv_indev_t* encoder = lv_indev_create();
  logging::debug(NULL, "Encoder created");
  lv_indev_set_type(encoder, LV_INDEV_TYPE_ENCODER);
  logging::debug(NULL, "Encoder type set");
  lv_indev_set_read_cb(encoder, encoderRead);
  logging::debug(NULL, "Encoder callback bound");
  lastLvglRun = millis();
  logging::debug(NULL, "Display setup complete!");

  // // Create the UI.
  lv_obj_t* uiContainer = lv_obj_create(lv_screen_active());
  lv_obj_set_width(uiContainer, 320);
  lv_obj_set_height(uiContainer, 240);
  lv_obj_set_layout(uiContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(uiContainer, LV_FLEX_FLOW_COLUMN);

  lv_obj_t* actualTempLabel = lv_label_create(uiContainer);
  lv_label_set_text(actualTempLabel, "Starting...");
  lv_obj_align(actualTempLabel, LV_ALIGN_TOP_LEFT, 0, 0);
  auto mapActualTempToLabel = map<std::string, TempC>(infallibleTemp, [](TempC v) {
    return string_format("Actual: %s", au_to_string(v, 2).c_str());
  });
  //mapActualTempToLabel = tap(mapActualTempToLabel, rheo::sinks::arduino::serialStringLineSink());
  auto pullAndSourceActualTempLabel = lvgl::label(actualTempLabel, mapActualTempToLabel, emptyStyleSource);
  pullActualTemp = std::get<0>(pullAndSourceActualTempLabel);

  smoothTempLabel = lv_label_create(uiContainer);
  lv_label_set_text(smoothTempLabel, "Starting...");
  lv_obj_align(smoothTempLabel, LV_ALIGN_LEFT_MID, 0, 0);
  auto mapSmoothTempToLabel = map<std::string, TempC>(smoothTemp, [](TempC v) {
    return string_format("Smooth: %s", au_to_string(v, 2).c_str());
  });
  //mapSmoothTempToLabel = tap(mapSmoothTempToLabel, rheo::sinks::arduino::serialStringLineSink());
  auto pullAndSourceSmoothTempLabel = lvgl::label(smoothTempLabel, mapSmoothTempToLabel, emptyStyleSource);
  pullSmoothTemp = std::get<0>(pullAndSourceSmoothTempLabel);

  // Setpoint editor
  Serial.println("Setting up setpoint editor...");
  lv_obj_t* setpointContainer = lv_obj_create(uiContainer);
  lv_obj_set_size(setpointContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(setpointContainer, 0, 0);
  lv_obj_set_style_border_width(setpointContainer, 0, 0);
  lv_obj_set_layout(setpointContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(setpointContainer, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_flex_cross_place(setpointContainer, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_t* setpointLabel = lv_label_create(setpointContainer);
  lv_label_set_text(setpointLabel, "Setpoint");
  lv_obj_t* setpointMinusButton = lv_button_create(setpointContainer);
  lv_obj_set_size(setpointMinusButton, 20, 20);
  lv_obj_set_style_bg_image_src(setpointMinusButton, LV_SYMBOL_MINUS, 0);
  lv_obj_add_event_cb(
    setpointMinusButton,
    [](lv_event_t* e){
      lv_event_code_t code = lv_event_get_code(e);
      if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_decrement(setpointEditor);
      }
    },
    LV_EVENT_ALL, NULL
  );
  setpointEditor = lv_spinbox_create(setpointContainer);
  lv_spinbox_set_value(setpointEditor, 20);
  lv_spinbox_set_digit_format(setpointEditor, 3, 0);
  lv_obj_t* setpointPlusButton = lv_button_create(setpointContainer);
  lv_obj_set_size(setpointPlusButton, 20, 20);
  lv_obj_set_style_bg_image_src(setpointPlusButton, LV_SYMBOL_PLUS, 0);
  lv_obj_add_event_cb(
    setpointPlusButton,
    [](lv_event_t* e){
      lv_event_code_t code = lv_event_get_code(e);
      if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_increment(setpointEditor);
      }
    },
    LV_EVENT_ALL, NULL
  );
  setpoint.addSink(
    [](auto value) {
      logging::debug("setpoint", "Setting setpoint to %s", au_to_string(value, 2).c_str());
    },
    [](){}
  );
  auto setpointC = map<float, TempC>(setpoint.sourceFn(false), [](TempC value) { return value.in(au::Celsius{}); });
  // auto pullAndSourceSetpointEditor = lvgl::spinbox(setpointEditor, setpointC, emptyStyleSource);
  // auto setpointEditorSourceFn = std::get<1>(pullAndSourceSetpointEditor);
  // setpointEditorSourceFn(
  //   [&setpoint, setpointEditor](lv_event_code_t event) {
  //     if (event == LV_EVENT_VALUE_CHANGED) {
  //       Serial.println("Handling setpoint editing event");
  //       setpoint.set(au::celsius_pt((float)lv_spinbox_get_value(setpointEditor)), false);
  //     }
  //   },
  //   [](){}
  // );
  lv_obj_t* setpointUnitsLabel = lv_label_create(setpointContainer);
  lv_label_set_text(setpointUnitsLabel, "°C");
  logging::debug(NULL, "Setup completed!");
}

int16_t lastReadEncoderClicks = 0;

void loop() {
  logging::trace(NULL, "Pulling temp and hum...");
  pullTempAndHum();

  unsigned long now = millis();
  if (lastLvglRun + timeTillNextLvglRun <= now) {
    timeTillNextLvglRun = lv_timer_handler();
    if (timeTillNextLvglRun == LV_NO_TIMER_READY) {
      timeTillNextLvglRun = LV_DEF_REFR_PERIOD;
    } else {
      lv_tick_inc(now - lastLvglRun);
      lastLvglRun = now;
    }
  }
}