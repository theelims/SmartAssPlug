#pragma once
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

#include <board.h>
#include <Arduino.h>
#include "freertos/timers.h"

#define VIBRATOR_PWM_FREQ 5000
#define VIBRATOR_PWM_RESOLUTION 8
#define VIBRATOR_CHANNEL 0
#define GREET_INTERVAL 250

class Vibrator
{
public:
    Vibrator()
    {
        pinMode(VIBRATE_0, OUTPUT);
        pinMode(VIBRATE_1, OUTPUT);
        ledcSetup(VIBRATOR_CHANNEL, VIBRATOR_PWM_FREQ, VIBRATOR_PWM_RESOLUTION);
        // attach the channel to the GPIO to be controlled
        ledcAttachPin(VIBRATE_0, VIBRATOR_CHANNEL);
        ledcAttachPin(VIBRATE_1, VIBRATOR_CHANNEL);

        // Create the FreeRTOS timer for brief vibration greeting
        timerHandle = xTimerCreate("VibratorGreeting", pdMS_TO_TICKS(GREET_INTERVAL), pdFALSE, this, timerCallbackStatic);
        xTimerStart(timerHandle, portMAX_DELAY);
        vibratorPercentage(50);
    }

    void vibratorPercentage(int speed)
    {
        // constrain speed to 0-100% and map to 8 bit
        _speed = constrain(speed, 0, 100);
        int dutyCycle = 0;
        if (_speed >= 1)
        {
            dutyCycle = map(_speed, 0, 100, 30, 255);
        }

        ESP_LOGV("Vibe", "Vibrator Speed: %d", _speed);
        // make vibrator go BRRR
        ledcWrite(VIBRATOR_CHANNEL, dutyCycle);
    }

private:
    int _speed = 0;
    TimerHandle_t timerHandle;

    static void timerCallbackStatic(TimerHandle_t xTimer)
    {
        // Get the instance of the class from the timer's user data
        Vibrator *instance = static_cast<Vibrator *>(pvTimerGetTimerID(xTimer));
        // Call the member function using the class instance
        if (instance)
        {
            instance->vibratorPercentage(0);
            xTimerDelete(instance->timerHandle, portMAX_DELAY);
        }
    }
};
