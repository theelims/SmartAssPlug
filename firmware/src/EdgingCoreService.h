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

#include <ESP32SvelteKit.h>
#include <EdgingMqttSettingsService.h>
#include <board.h>
#include <BatteryMonitor.h>
#include <SparkFun_MicroPressure.h>
#include <MqttPubSub.h>
#include <WebSocketServer.h>
#include <EdgingDataService.h>
#include <DataLogging.h>
#include <Vibrator.h>
#include <WebSocketRawDataStreaming.h>
#include <LightShow.h>
#include <EdgingFilterService.h>

#define EDGING_CONTROL_SOCKET_PATH "/ws/control"

#define PRESSURE_SAMPLING_INTERVAL 25

#define colorSaturation 128

#define ecTAG "EdgingCore"

namespace cbor = ::qindesign::cbor;

enum SessionType
{
    CLASSIFIER,
    ONWRIKBAAR,
    NOGASM,
    TRAINING
};

static const char *sessionTypeToStr[] =
    {
        "Classifier",
        "Onwrikbaar",
        "Nogasm",
        "Training"};

class EdgingCore
{
public:
    bool sessionActive;
    SessionType sessionType;
    int analogValue;
    int vibrationStrength;

    static void read(EdgingCore &settings, JsonObject &root)
    {
        root["active"] = settings.sessionActive;
        root["session_type"] = settings.sessionType;
        root["value"] = settings.analogValue;
        root["vibrate"] = settings.vibrationStrength;
    }

    static StateUpdateResult update(JsonObject &root, EdgingCore &edgingSession)
    {
        boolean newActive = root["active"] | false;
        int newValue = root["value"] | 0;
        int newVibrate = root["vibrate"] | 0;

        if (edgingSession.sessionActive != newActive || edgingSession.analogValue != newValue || edgingSession.vibrationStrength != newVibrate)
        {
            edgingSession.sessionActive = newActive;
            edgingSession.analogValue = newValue;
            edgingSession.vibrationStrength = newVibrate;
            // only accept changes to session type with start of a new session
            if (edgingSession.sessionActive != newActive && newActive == true)
                edgingSession.sessionType = root["session_type"] | CLASSIFIER;
            return StateUpdateResult::CHANGED;
        }
        return StateUpdateResult::UNCHANGED;
    }
};

class EdgingCoreService : public StatefulService<EdgingCore>
{
public:
    EdgingCoreService(ESP32SvelteKit *esp32sveltekit);

    void begin();

private:
    ESP32SvelteKit *_esp32sveltekit;
    AsyncWebServer *_server;
    MqttPubSub<EdgingCore> _mqttPubSub;
    WebSocketServer<EdgingCore> _webSocketServer;
    AsyncMqttClient *_mqttClient;
    EdgingMqttSettingsService _edgingMqttSettingsService;
    EdgingDataService _edgingDataService;
    DataLog _dataLog;
    Vibrator _vibrator;
    LightShow _lightShow;
#ifdef WS_RAW_DATA_STREAMING
    WebSocketRawDataStreamer _rawDataStreamer;
#endif
    EdgingFilterService _edgingFilterService;
#ifdef ADAFRUIT
    SparkFun_MicroPressure mpr;
#else
    SparkFun_MicroPressure mpr = SparkFun_MicroPressure(EOC_PIN, RST_PIN);
#endif

    void registerConfig();
    void onConfigUpdated();

    // vector containing the various edging algorithms

    TaskHandle_t coreTaskHandle;

    static void _loopImpl(void *_this) { static_cast<EdgingCoreService *>(_this)->_loop(); }
    void _loop();
};
