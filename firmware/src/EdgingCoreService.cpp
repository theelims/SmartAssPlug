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

uint8_t rawDataFileBytes[CBORS_DEFAULT_ARRAY_SIZE / 2]{0};
cbor::BytesPrint rawDataFilePrint{rawDataFileBytes, sizeof(rawDataFileBytes)};

#ifdef ADAFRUIT
Adafruit_MPRLS mpr = Adafruit_MPRLS();
#else
Adafruit_MPRLS mpr = Adafruit_MPRLS(RST_PIN, EOC_PIN, 0, 25, 10, 90, PSI_to_HPA);
#endif

SimpleKalmanFilter pressureKalmanFilter = SimpleKalmanFilter(1, 1, 1.0);

NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> logo = NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod>(2, NEOPIXEL);

File logFile;
unsigned int logFileStart = 0;
unsigned int timeOffset = 0;

AsyncWebSocket ws = AsyncWebSocket(EDGING_RAW_DATA_SOCKET_PATH);
// TODO: WS-Callback and close unresponsive clients

void printCBOR(uint8_t *message, size_t len)
{
    Serial.print("CBOR: ");
    for (size_t i = 0; i < len; ++i)
    {
        if (message[i] < 0x10)
        {
            Serial.print('0');
        }
        Serial.print(message[i], HEX);
    }
    Serial.println();
}

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
                                                                                          &_edgingMqttSettingsService)
{
    _state.sessionActive = false;
    _state.sessionType = CLASSIFIER;

    // add raw data websocket
    _server->addHandler(&ws);

    // serve rawdata logfile from file system
    _server->serveStatic("/rawdata/datalog.dat", ESPFS, "/rawdata/datalog.dat");

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
        NULL,                 /* parameter of the task */
        tskIDLE_PRIORITY + 2, /* priority of the task */
        &coreTaskHandle,      /* Task handle to keep track of created task */
        1);                   /* pin task to core 1 */
}

void EdgingCoreService::onConfigUpdated()
{
    // Start / Stop recording session

    // Write to a file
    // TODO: Mutex
    if (!logFile && _state.sessionActive)
    {
        logFile = _esp32sveltekit->getFS()->open("/rawdata/datalog.dat", "w", true);
        cbor::Writer rawDataChunk{rawDataFilePrint};
        rawDataFilePrint.reset();
        // TODO: Add file header and context
        rawDataChunk.beginIndefiniteArray();
        size_t length = rawDataChunk.getWriteSize();
        logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
        ESP_LOGI(ecTAG, "Start writing datalog file");
        logFileStart = timeOffset = millis();
    }

    if (logFile && !_state.sessionActive)
    {
        cbor::Writer rawDataChunk{rawDataFilePrint};
        rawDataFilePrint.reset();
        rawDataChunk.endIndefinite();
        size_t length = rawDataChunk.getWriteSize();
        logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
        logFile.close();
        ESP_LOGI(ecTAG, "Closed datalog file: %d bytes written, %d seconds recorded", logFile.size(), (millis() - logFileStart) / 1000);
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

bool EdgingCoreService::storeRawDataInFile(unsigned int rawPressure, unsigned int smoothPressure)
{
    if (logFile)
    {
        // TODO: Mutex
        cbor::Writer rawDataChunk{rawDataFilePrint};
        rawDataFilePrint.reset();

        // check for available space and terminate file if necessary
        if ((ESPFS.totalBytes() - ESPFS.usedBytes()) < 1024)
        {
            rawDataChunk.endIndefinite();
            String errorMessage = "File System Out of Memory";
            rawDataChunk.beginText(errorMessage.length());
            rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(errorMessage.c_str()), errorMessage.length());
            size_t length = rawDataChunk.getWriteSize();
            logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
            ESP_LOGD(ecTAG, "Filesystem out of memory: closed datalog file. %d bytes written, %d seconds recorded", logFile.size(), (millis() - logFileStart) / 1000);
            logFile.close();
            return false;
        }

        // TODO: write data in bigger chunks. Maybe in a separate thread with a queue?
        rawDataChunk.beginArray(3);
        rawDataChunk.writeUnsignedInt(millis() - timeOffset);
        rawDataChunk.writeUnsignedInt(rawPressure);
        rawDataChunk.writeUnsignedInt(smoothPressure);

        size_t length = rawDataChunk.getWriteSize();
        logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);

        return true;
    }
    return false;
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
    int i = 0;
    while (1)
    {
        // Write next chunk of data
        cbor::Writer cborWS{rawDataWSPrint};
        rawDataWSPrint.reset();
        cborWS.beginIndefiniteArray();

        for (size_t i = 0; i < WS_PACKET_AGGREGATION_ARRAY; i++)
        {
            // 40Hz Update rate
            vTaskDelayUntil(&xLastWakeTime, PRESSURE_SAMPLING_INTERVAL / portTICK_PERIOD_MS);

            cborWS.beginArray(3);

            // write timestamp
            cborWS.writeUnsignedInt(millis());

            // read pressure and do math
            rawPressure = (unsigned int)(mpr.readPressure() * 100);
            cborWS.writeUnsignedInt(rawPressure);

            filteredPressure = (unsigned int)pressureKalmanFilter.updateEstimate((float)rawPressure);
            cborWS.writeUnsignedInt(filteredPressure);

            storeRawDataInFile(rawPressure, filteredPressure);

            // ESP_LOGV(ecTAG, "raw: %.2f, kalman %.2f", rawPressure, filteredPressure);
        }

        // end indefinite array
        cborWS.endIndefinite();

        // Send data over websocket
        size_t length = cborWS.getWriteSize();
        ws.binaryAll(reinterpret_cast<uint8_t *>(&rawDataWSBytes), length);
    }
}
