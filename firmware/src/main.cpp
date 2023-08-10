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
#include <EdgingMqttSettingsService.h>
#include <EdgingDataService.h>
#include <NeoPixelBus.h>
#include <board.h>
#include <BatteryMonitor.h>
#include "Adafruit_MPRLS.h"
#include <SimpleKalmanFilter.h>

#define colorSaturation 128

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor pink(colorSaturation, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

#ifdef ADAFRUIT
Adafruit_MPRLS mpr = Adafruit_MPRLS();
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> logo(1, NEOPIXEL);
#else
Adafruit_MPRLS mpr = Adafruit_MPRLS(RST_PIN, EOC_PIN, 0, 25, 10, 90, PSI_to_HPA);
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> logo(2, 0);
#endif

SimpleKalmanFilter pressureKalmanFilter(1, 1, 1.0);
float rawPressure = 0.0;
float filteredPressure = 0.0;
TaskHandle_t sensorTaskHandle;

AsyncWebServer server(80);
ESP32SvelteKit esp32sveltekit(&server);

EdgingMqttSettingsService edgingMqttSettingsService =
    EdgingMqttSettingsService(&server, esp32sveltekit.getFS(), esp32sveltekit.getSecurityManager());

EdgingDataService edgingDataService = EdgingDataService(&server,
                                                        esp32sveltekit.getSecurityManager(),
                                                        esp32sveltekit.getMqttClient(),
                                                        &edgingMqttSettingsService);
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

void sensorTask(void *parameter)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        rawPressure = mpr.readPressure();
        filteredPressure = pressureKalmanFilter.updateEstimate(rawPressure);

        edgingDataService.update([&](EdgingData &data)
                                 { 
            data.timestamp = millis();
            data.rawPressure = rawPressure;
            data.filteredPressure = filteredPressure;
            return StateUpdateResult::CHANGED; },
                                 "sensor");
        // ESP_LOGV("Sensor", "raw: %.2f, kalman %.2f", rawPressure, filteredPressure);

        // 50Hz Update rate
        vTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
    }
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

    logo.Begin();

    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);

    // start the framework and demo project
    esp32sveltekit.setMDNSAppName("SmartAss Plug");
    esp32sveltekit.begin();
    esp32sveltekit.getSleepService()->attachOnSleepCallback(prepareForShutDown);

    // start the MQTT broker settings service
    edgingMqttSettingsService.begin();

    // start the server
    server.begin();

    if (!mpr.begin())
    {
        ESP_LOGE("Main", "MRPLS not found!");
    }

    logo.SetPixelColor(0, pink);
    logo.SetPixelColor(1, pink);
    logo.Show();

    // Start Sensor Task
    xTaskCreatePinnedToCore(
        sensorTask,           /* Task function. */
        "sensorTask",         /* name of task. */
        2048,                 /* Stack size of task */
        NULL,                 /* parameter of the task */
        tskIDLE_PRIORITY + 2, /* priority of the task */
        &sensorTaskHandle,    /* Task handle to keep track of created task */
        1);                   /* pin task to core 1 */
}

void loop()
{
    // Delete Arduino loop task, as it is not needed here
    vTaskDelete(NULL);
}
