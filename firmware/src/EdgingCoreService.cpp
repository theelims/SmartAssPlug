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

#include <EdgingCoreService.h>

EdgingCoreService::EdgingCoreService(ESP32SvelteKit *esp32sveltekit) : _esp32sveltekit(esp32sveltekit),
                                                                       _server(esp32sveltekit->getServer()),
                                                                       _mqttPubSub(EdgingCore::read,
                                                                                   EdgingCore::update,
                                                                                   this,
                                                                                   esp32sveltekit->getMqttClient()),
                                                                       _webSocketServer(EdgingCore::read,
                                                                                        EdgingCore::update,
                                                                                        this,
                                                                                        esp32sveltekit->getServer(),
                                                                                        EDGING_CONTROL_SOCKET_PATH,
                                                                                        esp32sveltekit->getSecurityManager(),
                                                                                        AuthenticationPredicates::NONE_REQUIRED),
                                                                       _mqttClient(esp32sveltekit->getMqttClient()),
                                                                       _edgingMqttSettingsService(esp32sveltekit->getServer(),
                                                                                                  esp32sveltekit->getFS(),
                                                                                                  esp32sveltekit->getSecurityManager()),
                                                                       _edgingDataService(esp32sveltekit->getServer(),
                                                                                          esp32sveltekit->getSecurityManager(),
                                                                                          esp32sveltekit->getMqttClient(),
                                                                                          &_edgingMqttSettingsService),
#ifdef WS_RAW_DATA_STREAMING
                                                                       _rawDataStreamer(esp32sveltekit->getServer()),
#endif
                                                                       _dataLog(esp32sveltekit->getServer())
{
    _state.sessionActive = false;
    _state.sessionType = CLASSIFIER;

    // configure MQTT callback
    _mqttClient->onConnect(std::bind(&EdgingCoreService::registerConfig, this));

    // configure update handler for when the MQTT settings change
    _edgingMqttSettingsService.addUpdateHandler([&](const String &originId)
                                                { registerConfig(); },
                                                false);
    // start the MQTT broker settings service
    _edgingMqttSettingsService.begin();

    // configure settings service update handler to update state
    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(); },
                     false);
}

void EdgingCoreService::begin()
{
    if (!mpr.begin())
    {
        ESP_LOGE(ecTAG, "MRPLS not found!");
        _esp32sveltekit->getNotificationEvents()->pushNotification("Pressure Sensor defective!", ERROR, millis());

        return;
    }

    _lightShow.setLight();

    // Start Sensor Task
    xTaskCreatePinnedToCore(
        _loopImpl,             /* Task function. */
        "coreEdgingTask",      /* name of task. */
        4096,                  /* Stack size of task */
        this,                  /* parameter of the task */
        tskIDLE_PRIORITY + 10, /* priority of the task */
        &coreTaskHandle,       /* Task handle to keep track of created task */
        1);                    /* pin task to core 1 */
}

void EdgingCoreService::onConfigUpdated()
{
    // Start / Stop recording session
    if (_state.sessionActive)
    {
        _dataLog.startLogFile("Test");
    }
    else
    {
        _dataLog.stopLogFile();
    }

    // Update Vibrator
    _vibrator.vibratorPercentage(_state.vibrationStrength);
}

void EdgingCoreService::registerConfig()
{
    if (!_mqttClient->connected())
    {
        return;
    }

    String pubTopic;

    _edgingMqttSettingsService.read([&](EdgingMqttSettings &settings)
                                    { pubTopic = settings.mqttControlPath; });

    _mqttPubSub.configureTopics(pubTopic, "");
}

/**************************************************************************************
 * @brief main loop function
 * read sensor data
 * smooth sensor data
 * send sensor raw data via websocket
 * record CBOR logfile
 * feed sensor data into edging algorithm
 * send edging data through stateful service
 *
 **************************************************************************************/
void EdgingCoreService::_loop()
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    // All pressures in Pa (1 Pa = 0.01 mbar) to stay in integer
    unsigned int rawPressure = 0;
    unsigned int filteredPressure = 0;
    unsigned int timeStamp = 0;
    float rawSensorReading = 0.0;

    while (1)
    {
        // Check if sensor reading is valid
        rawSensorReading = mpr.readPressure();
        if (std::isnan(rawSensorReading))
        {
            ESP_LOGW(ecTAG, "Invalid pressure reading. Status byte: %d", mpr.readStatus());
        }
        else
        {
            timeStamp = millis();

            // read pressure and do math for Pascal (no float)
            rawPressure = (unsigned int)(rawSensorReading * 100);

            filteredPressure = (unsigned int)pressureKalmanFilter.updateEstimate((float)rawPressure);

            _dataLog.logRawData(timeStamp, rawPressure, filteredPressure, NEUTRAL);
#ifdef WS_RAW_DATA_STREAMING
            _rawDataStreamer.streamRawData(timeStamp, rawPressure, filteredPressure);
#endif

            // ESP_LOGV(ecTAG, "raw: %.2f, kalman %.2f", rawPressure, filteredPressure);

            // 40Hz Update rate
            vTaskDelayUntil(&xLastWakeTime, PRESSURE_SAMPLING_INTERVAL / portTICK_PERIOD_MS);
        }
    }
}
