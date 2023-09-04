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

#define colorSaturation 128

RgbColor orgasm(colorSaturation, 0, 0);                /* red */
RgbColor edging(0, 0, colorSaturation);                /* blue */
RgbColor aroused(colorSaturation, 0, colorSaturation); /* pink */
RgbColor stdby(colorSaturation);                       /* white */
RgbColor off(0);                                       /* black */
RgbColor neutral(0, colorSaturation, 0);               /* green */

uint8_t rawDataWSBytes[CBORS_DEFAULT_ARRAY_SIZE]{0};
cbor::BytesPrint rawDataWSPrint{rawDataWSBytes, sizeof(rawDataWSBytes)};

#ifdef ADAFRUIT
Adafruit_MPRLS mpr = Adafruit_MPRLS();
#else
Adafruit_MPRLS mpr = Adafruit_MPRLS(RST_PIN, EOC_PIN, 0, 25, 10, 90, PSI_to_HPA);
#endif

SimpleKalmanFilter pressureKalmanFilter = SimpleKalmanFilter(1, 1, 1.0);

NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> logo = NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod>(2, NEOPIXEL);

AsyncWebSocket ws = AsyncWebSocket(EDGING_RAW_DATA_SOCKET_PATH);
// TODO: WS-Callback and close unresponsive clients

EdgingCoreService::EdgingCoreService(ESP32SvelteKit *esp32sveltekit) : _esp32sveltekit(esp32sveltekit),
                                                                       _server(esp32sveltekit->getServer()),
                                                                       _mqttPubSub(EdgingCore::read, EdgingCore::update, this, esp32sveltekit->getMqttClient()),
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
                                                                       _dataLog(esp32sveltekit->getServer())
{
    _state.sessionActive = false;
    _state.sessionType = CLASSIFIER;

    // add raw data websocket
    _server->addHandler(&ws);

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

    // begin neopixel lightshow
    logo.Begin();
}

void EdgingCoreService::begin()
{
    if (!mpr.begin())
    {
        ESP_LOGE(ecTAG, "MRPLS not found!");
        _esp32sveltekit->getNotificationEvents()->pushNotification("Pressure Sensor defective!", ERROR, millis());

        return;
    }

    logo.SetPixelColor(0, stdby);
    logo.SetPixelColor(1, stdby);
    logo.Show();

    // Start Sensor Task
    xTaskCreatePinnedToCore(
        _loopImpl,            /* Task function. */
        "coreEdgingTask",     /* name of task. */
        8192,                 /* Stack size of task */
        this,                 /* parameter of the task */
        tskIDLE_PRIORITY + 2, /* priority of the task */
        &coreTaskHandle,      /* Task handle to keep track of created task */
        1);                   /* pin task to core 1 */
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
    int i = 0;
    while (1)
    {
        // Write next chunk of data
        cbor::Writer cborWS{rawDataWSPrint};
        rawDataWSPrint.reset();
        cborWS.resetWriteSize();
        cborWS.beginIndefiniteArray();

        for (size_t i = 0; i < WS_PACKET_AGGREGATION_ARRAY; i++)
        {
            // 40Hz Update rate
            vTaskDelayUntil(&xLastWakeTime, PRESSURE_SAMPLING_INTERVAL / portTICK_PERIOD_MS);

            cborWS.beginArray(3);

            // write timestamp
            timeStamp = millis();
            cborWS.writeUnsignedInt(timeStamp);

            // read pressure and do math
            rawPressure = (unsigned int)(mpr.readPressure() * 100);
            cborWS.writeUnsignedInt(rawPressure);

            filteredPressure = (unsigned int)pressureKalmanFilter.updateEstimate((float)rawPressure);
            cborWS.writeUnsignedInt(filteredPressure);

            _dataLog.logRawData(timeStamp, rawPressure, filteredPressure, NEUTRAL);

            // ESP_LOGV(ecTAG, "raw: %.2f, kalman %.2f", rawPressure, filteredPressure);
        }

        // end indefinite array
        cborWS.endIndefinite();

        // Send data over websocket
        size_t length = cborWS.getWriteSize();
        ws.binaryAll(reinterpret_cast<uint8_t *>(&rawDataWSBytes), length);
    }
}
