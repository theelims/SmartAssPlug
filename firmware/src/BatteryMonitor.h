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
#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <BatteryService.h>
#include <SleepService.h>
#include <NotificationEvents.h>
#include "freertos/timers.h"

#define bmTAG "BatteryMonitor"

class BatteryMonitor
{
public:
    BatteryMonitor(BatteryService *batteryService, SleepService *sleepService, NotificationEvents *notificationEvents) : _lipo(MAX1704X_MAX17048),
                                                                                                                         _batteryService(batteryService),
                                                                                                                         _sleepService(sleepService),
                                                                                                                         _notificationEvents(notificationEvents){};

    void begin()
    {
        if (_lipo.begin() == false)
        { // Connect to the MAX17043 using the default wire port
            ESP_LOGE(bmTAG, "Error: Fuel Gauge MAX17048 not detected");
        }

        // Create the FreeRTOS timer
        timerHandle = xTimerCreate("BatteryTimer", pdMS_TO_TICKS(BATTERY_UPDATE_INTERVAL), pdTRUE, this, timerCallbackStatic);
        xTimerStart(timerHandle, portMAX_DELAY);

        // Quick start restarts the MAX17043 in hopes of getting a more accurate
        // guess for the SOC.
        _lipo.quickStart();

        // We can set an interrupt to alert when the battery SoC gets too low.
        // We can alert at anywhere between 1% - 32%:
        _lipo.setThreshold(LOW_BATTERY_SHUT_OFF_SOC); // Set alert threshold to 4%.
    }

    void end()
    {
        xTimerStop(timerHandle, portMAX_DELAY);
    }

    void powerDown()
    {
        _lipo.sleep();
    }

protected:
    BatteryService *_batteryService;
    SleepService *_sleepService;
    NotificationEvents *_notificationEvents;
    SFE_MAX1704X _lipo; // Create a MAX17048
    double soc = 0;     // Variable to keep track of _lipo state-of-charge (SOC)
    bool alert = false; // Variable to keep track of whether alert has been triggered
    boolean charging = false;
    TimerHandle_t timerHandle;

    void batteryMonitorWrapper()
    {
        soc = _lipo.getSOC();
        charging = (_lipo.getChangeRate() > 0);
        // ESP_LOGV(bmTAG, "Battery SOC: %.1f %%, charging: %d", soc, charging);
        _batteryService->updateSOC(soc);
        _batteryService->setCharging(charging);

        // if battery is dangerously low, shut down
        if ((soc <= 4.0) && !charging)
        {
            ESP_LOGW(bmTAG, "Battery low: Shut off device.");
            _notificationEvents->pushNotification("Battery low, shuting down!", ERROR, millis());
            _sleepService->sleepNow();
        }
    }

    static void timerCallbackStatic(TimerHandle_t xTimer)
    {
        // Get the instance of the class from the timer's user data
        BatteryMonitor *instance = static_cast<BatteryMonitor *>(pvTimerGetTimerID(xTimer));
        // Call the member function using the class instance
        if (instance)
        {
            instance->batteryMonitorWrapper();
        }
    }
};
