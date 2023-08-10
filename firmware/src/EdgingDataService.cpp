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

#include <EdgingDataService.h>

EdgingDataService::EdgingDataService(AsyncWebServer *server,
                                     SecurityManager *securityManager,
                                     AsyncMqttClient *mqttClient,
                                     EdgingMqttSettingsService *edgingMqttSettingsService) : _mqttPubSub(EdgingData::read, EdgingData::noUpdate, this, mqttClient),
                                                                                             _webSocketServer(EdgingData::read,
                                                                                                              EdgingData::noUpdate,
                                                                                                              this,
                                                                                                              server,
                                                                                                              EDGING_DATA_SOCKET_PATH,
                                                                                                              securityManager,
                                                                                                              AuthenticationPredicates::NONE_REQUIRED),
                                                                                             _mqttClient(mqttClient),
                                                                                             _edgingMqttSettingsService(edgingMqttSettingsService)
{
    _state.timestamp = 0;
    _state.rawPressure = 1024.0;
    _state.filteredPressure = 1024.0;
    _state.arousalValue = 0.0;
    _state.arousalState = NEUTRAL;

    // configure MQTT callback
    _mqttClient->onConnect(std::bind(&EdgingDataService::registerConfig, this));

    // configure update handler for when the MQTT settings change
    _edgingMqttSettingsService->addUpdateHandler([&](const String &originId)
                                                 { registerConfig(); },
                                                 false);
}

void EdgingDataService::registerConfig()
{
    if (!_mqttClient->connected())
    {
        return;
    }

    String pubTopic;

    _edgingMqttSettingsService->read([&](EdgingMqttSettings &settings)
                                     { pubTopic = settings.mqttDataPath; });

    _mqttPubSub.configureTopics(pubTopic, "");
}
