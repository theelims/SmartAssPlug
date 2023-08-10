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

#include <EdgingMqttSettingsService.h>

#include <MqttPubSub.h>
#include <WebSocketServer.h>

#define EDGING_DATA_SOCKET_PATH "/ws/edgingData"

enum ArousalState
{
    NEUTRAL,
    AROUSED,
    EDGING,
    ORGASM
};

static const char *arousalStateToStr[] =
    {
        "Neutral",
        "Aroused",
        "Edging",
        "Orgasm"};

class EdgingData
{
public:
    int timestamp;
    float rawPressure;
    float filteredPressure;
    float arousalValue;
    ArousalState arousalState;

    static void read(EdgingData &settings, JsonObject &root)
    {
        root["timestamp"] = settings.timestamp;
        root["raw_pres"] = settings.rawPressure;
        root["filtered_pres"] = settings.filteredPressure;
        root["arousal_value"] = settings.arousalValue;
        root["arousal_state"] = arousalStateToStr[settings.arousalState];
    }

    // WS and MQTT are read only
    static StateUpdateResult noUpdate(JsonObject &root, EdgingData &edgingState)
    {
        return StateUpdateResult::UNCHANGED;
    }
};

class EdgingDataService : public StatefulService<EdgingData>
{
public:
    EdgingDataService(AsyncWebServer *server,
                      SecurityManager *securityManager,
                      AsyncMqttClient *mqttClient,
                      EdgingMqttSettingsService *edgingMqttSettingsService);

private:
    MqttPubSub<EdgingData> _mqttPubSub;
    WebSocketServer<EdgingData> _webSocketServer;
    AsyncMqttClient *_mqttClient;
    EdgingMqttSettingsService *_edgingMqttSettingsService;

    void registerConfig();
};
