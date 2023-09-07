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

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>

#define AROUSAL_BROKER_SETTINGS_FILE "/config/brokerSettings.json"
#define AROUSAL_BROKER_SETTINGS_PATH "/rest/brokerSettings"

class EdgingMqttSettings
{
public:
    String mqttDataPath;
    String mqttControlPath;
    String mqttVibratePath;
    String uniqueId;

    static void read(EdgingMqttSettings &settings, JsonObject &root)
    {
        root["mqtt_data_path"] = settings.mqttDataPath;
        root["mqtt_control_path"] = settings.mqttControlPath;
        root["unique_id"] = settings.uniqueId;
    }

    static StateUpdateResult update(JsonObject &root, EdgingMqttSettings &settings)
    {
        settings.mqttDataPath = root["mqtt_data_path"] | SettingValue::format("smartassplug/#{unique_id}/data");
        settings.mqttControlPath = root["mqtt_control_path"] | SettingValue::format("smartassplug/#{unique_id}/control");
        settings.uniqueId = root["unique_id"] | SettingValue::format("smartassplug-#{unique_id}");
        return StateUpdateResult::CHANGED;
    }
};

class EdgingMqttSettingsService : public StatefulService<EdgingMqttSettings>
{
public:
    EdgingMqttSettingsService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager);
    void begin();

private:
    HttpEndpoint<EdgingMqttSettings> _httpEndpoint;
    FSPersistence<EdgingMqttSettings> _fsPersistence;
};
