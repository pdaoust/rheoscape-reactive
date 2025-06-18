#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <SHT2x.h>

namespace rheo::sources::arduino {

  source_fn<std::variant<std::optional<au::QuantityPoint<au::Celsius, float>>, std::optional<au::Quantity<au::Percent, float>>>> sht21(TwoWire* i2c, uint8_t resolution = 0) {
    return [i2c, resolution](push_fn<std::variant<std::optional<au::QuantityPoint<au::Celsius, float>>, std::optional<au::Quantity<au::Percent, float>>>> push, end_fn end) {
      SHT21 sensor(i2c);
      return [resolution, sensor, push, end, lastRequestedType = 0]() mutable {
        if (!lastRequestedType) {
          if (!sensor.begin()) {
            Serial.print("Couldn't start SHT21; error");
            Serial.println(sensor.getError(), HEX);
            return;
          } else {
            // Initial request.
            // Unfortunately we can only request one type of reading at one time.
            // Note that it might be out of date before the next pull.
            Serial.println("Requesting first humidity reading");
            if (sensor.getResolution() != resolution) {
              sensor.setResolution(resolution);
            }
            if (!sensor.requestHumidity()) {
              Serial.print("Coudn't send humidity request; error ");
              Serial.println(sensor.getError(), HEX);
            } else {
              Serial.println("Requested!");
            }
            // Keep track of whether we pulled humidity (1) or temperature (2) last.
            lastRequestedType = 1;
            return;
          }
        }

        if (sensor.getResolution() != resolution) {
          sensor.setResolution(resolution);
        }
        switch (lastRequestedType) {
          case 1:
            if (sensor.reqHumReady()) {
              Serial.println("Humidity is ready");
              if (sensor.readHumidity()) {
                Serial.println("Read humidity");
                push(au::percent(sensor.getHumidity()));                  
                Serial.println("Pushed humidity");
              } else {
                // Something went wrong; push an empty value.
                // TODO: Is this actually recoverable? Should it end instead?
                Serial.print("Couldn't read humidity; error ");
                Serial.println(sensor.getError(), HEX);
                push((std::optional<au::Quantity<au::Percent, float>>)std::nullopt);
              }
            } // Don't push an empty value if it's just not ready to deliver a value yet.

            // Switch to the next type of reading.
            sensor.requestTemperature();
            lastRequestedType = 2;
            break;
          case 2:
            if (sensor.reqTempReady()) {
              if (sensor.readTemperature()) {
                push(au::celsius_pt(sensor.getTemperature()));                  
              } else {
                // TODO: See above re: the meaning of a false return value.
                push((std::optional<au::QuantityPoint<au::Celsius, float>>)std::nullopt);
              }
            }
            sensor.requestHumidity();
            lastRequestedType = 1;
            break;
        }
      };
    };
  }

}