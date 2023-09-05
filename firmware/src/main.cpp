/**
 *   SmartAss Plug
 *
 *   The smartest butt plug you can imagine.
 *   https://github.com/theelims/SmartAssPlug
 *
 *   Copyright (C) 2023 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the Attribution-ShareAlike 4.0 International license.
 *   See the LICENSE file for details.
 *
 **/

#include <ESP32SvelteKit.h>
#include <EdgingCoreService.h>
#include <board.h>
#include <BatteryMonitor.h>

AsyncWebServer server(80);
ESP32SvelteKit esp32sveltekit(&server);

// That's where the magic happens
EdgingCoreService edgingCore(&esp32sveltekit);

#ifndef ADAFRUIT
BatteryMonitor batteryMonitor = BatteryMonitor(esp32sveltekit.getBatteryService(),
                                               esp32sveltekit.getSleepService(),
                                               esp32sveltekit.getNotificationEvents());
#endif

void prepareForShutDown()
{
#ifdef ADAFRUIT
    digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
#else
    // place fuel gauge chip in power down mode
    batteryMonitor.powerDown();
    // depower periphery
    digitalWrite(ENV_EN, HIGH);
#endif
}

void setup()
{
#ifdef ADAFRUIT
    pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_I2C_POWER, HIGH);

#else
    // Power on the 3.3V environment
    pinMode(ENV_EN, OUTPUT);
    digitalWrite(ENV_EN, LOW);

    // Initialize Wire for MPRLS and Fuel Gauge
    Wire.begin(I2C_SDA, I2C_SCL);

    // start battery monitor
    batteryMonitor.begin();
#endif

    // Initialize Wire for IMU
    // Wire1.begin(I2C_SDA_1, I2C_SCL_1);

    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);

    // start the framework and project
    esp32sveltekit.setMDNSAppName("SmartAss Plug");
    esp32sveltekit.begin();
    esp32sveltekit.getSleepService()->attachOnSleepCallback(prepareForShutDown);
#ifdef WS_RAW_DATA_STREAMING
    esp32sveltekit.getFeatureService()->addFeature("raw_data_streaming", true);
#else
    esp32sveltekit.getFeatureService()->addFeature("raw_data_streaming", false);
#endif

    // start the server
    server.begin();

    // begin with edging core functionality
    edgingCore.begin();
}

void loop()
{
    // Delete Arduino loop task, as it is not needed here
    vTaskDelete(NULL);
}
