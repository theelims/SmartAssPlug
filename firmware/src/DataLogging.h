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
#include <CBOR.h>
#include <CBOR_parsing.h>
#include <CBOR_streams.h>
#include <EdgingCoreService.h>
#include "freertos/queue.h"

#define CBORS_LOGGING_ARRAY_SIZE 256
#define LOGFILE_WRITE_INTERVAL 250
#define LOG_QUEUE_SIZE 15

#define dlTAG "DataLog"

#define DATALOG_FILE "/rawdata/datalog.dat"

namespace cbor = ::qindesign::cbor;

struct rawDataItem
{
    unsigned int time;
    unsigned int rawPres;
    unsigned int smoothPres;
    byte state;
};

class DataLog
{
public:
    DataLog(AsyncWebServer *server)
    {
        // serve rawdata logfile from file system
        server->serveStatic(DATALOG_FILE, ESPFS, DATALOG_FILE);

        // Use the station's MAC address to create a unique id for each device.
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char macStr[13] = {0};
        sprintf(macStr, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        _uniqueID = String(macStr);

        // Create queue and semaphore
        stopSemaphore = xSemaphoreCreateBinary();
        logQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(struct rawDataItem));
        if (logQueue == 0)
        {
            ESP_LOGE(dlTAG, "Failed to create logging queue= %p\n", logQueue);
        }
    };

    void startLogFile(String sessionType)
    {
        if (loggingTaskHandle == nullptr)
        {
            _sessionType = sessionType;

            xQueueReset(logQueue);
            xSemaphoreGive(stopSemaphore);

            // Start File write Task
            xTaskCreatePinnedToCore(
                _loopImpl,            /* Task function. */
                "loggingTask",        /* name of task. */
                8192,                 /* Stack size of task */
                this,                 /* parameter of the task */
                tskIDLE_PRIORITY + 1, /* priority of the task */
                &loggingTaskHandle,   /* Task handle to keep track of created task */
                1);                   /* pin task to core 1 */
        }
    }

    void stopLogFile()
    {
        if (loggingTaskHandle != nullptr)
        {
            // Notify logging task to stop operation and delete itself
            boolean success = xSemaphoreTake(stopSemaphore, 0);
        }
    }

    bool logRawData(unsigned int time, unsigned int rawPressure, unsigned int smoothPressure, ArousalState state)
    {
        if (loggingTaskHandle != nullptr)
        {
            struct rawDataItem txBuffer;
            txBuffer.time = time;
            txBuffer.rawPres = rawPressure;
            txBuffer.smoothPres = smoothPressure;
            txBuffer.state = state;
            if (xQueueSend(logQueue, &txBuffer, 0))
            {
                ESP_LOGE(dlTAG, "Failed to enqueue logging data");
                return true;
            }
        }

        // return false on error / file system full
        return false;
    }

private:
    TaskHandle_t loggingTaskHandle = nullptr;
    QueueHandle_t logQueue;
    SemaphoreHandle_t stopSemaphore = nullptr;
    String _uniqueID;
    String _sessionType;

    static void _loopImpl(void *_this) { static_cast<DataLog *>(_this)->_loop(); }

    void _loop()
    {
        uint8_t rawDataFileBytes[CBORS_LOGGING_ARRAY_SIZE]{0};
        cbor::BytesPrint rawDataFilePrint{rawDataFileBytes, sizeof(rawDataFileBytes)};
        struct rawDataItem rxBuffer;
        File logFile;
        unsigned int logFileStart = millis();
        String keyDate = "Date";
        String keySessionType = "SessionType";
        String keyDeviceId = "DeviceID";
        String keyRawData = "RawData";

        // open file
        logFile = ESPFS.open(DATALOG_FILE, "w", true);
        cbor::Writer rawDataChunk{rawDataFilePrint};
        rawDataFilePrint.reset();

        // Write header
        // Date, Session Type, Device ID, RawData[*]
        rawDataChunk.beginMap(4);

        // Date
        rawDataChunk.beginText(keyDate.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(keyDate.c_str()), keyDate.length());
        rawDataChunk.writeInt(time(nullptr));

        // Session Type
        rawDataChunk.beginText(keySessionType.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(keySessionType.c_str()), keySessionType.length());
        rawDataChunk.beginText(_sessionType.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(_sessionType.c_str()), _sessionType.length());

        // Device ID
        rawDataChunk.beginText(keyDeviceId.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(keyDeviceId.c_str()), keyDeviceId.length());
        rawDataChunk.beginText(_uniqueID.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(_uniqueID.c_str()), _uniqueID.length());

        // RawData[*]
        rawDataChunk.beginText(keyRawData.length());
        rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(keyRawData.c_str()), keyRawData.length());
        rawDataChunk.beginIndefiniteArray();

        // Write header to log file
        size_t length = rawDataChunk.getWriteSize();
        logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
        // _printCBOR(rawDataFileBytes, length);

        ESP_LOGI(dlTAG, "Start writing datalog file");

        while (1)
        {
            // Reset CBOR chunk and start over
            rawDataFilePrint.reset();
            rawDataChunk.resetWriteSize();

            // check for available space and terminate file if necessary
            if (((ESPFS.totalBytes() - ESPFS.usedBytes()) < 5016) || xSemaphoreGive(stopSemaphore) == pdTRUE)
            {
                // String errorMessage = "File System Out of Memory";
                // rawDataChunk.beginText(errorMessage.length());
                // rawDataChunk.writeBytes(reinterpret_cast<const uint8_t *>(errorMessage.c_str()), errorMessage.length());
                // ESP_LOGD(dlTAG, "Filesystem out of memory!");
                rawDataChunk.endIndefinite();
                size_t length = rawDataChunk.getWriteSize();
                logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
                // _printCBOR(rawDataFileBytes, length);
                ESP_LOGD(dlTAG, "Closed datalog file. %d bytes written, %d seconds recorded", logFile.size(), (millis() - logFileStart) / 1000);
                logFile.close();

                // Terminate Task
                loggingTaskHandle = nullptr;
                vTaskDelete(NULL);
            }

            // Iterate over queue and write data chunks to file
            while (xQueueReceive(logQueue, &rxBuffer, (TickType_t)0) == pdTRUE)
            {
                rawDataChunk.beginArray(4);
                rawDataChunk.writeUnsignedInt(rxBuffer.time - logFileStart);
                rawDataChunk.writeUnsignedInt(rxBuffer.rawPres);
                rawDataChunk.writeUnsignedInt(rxBuffer.smoothPres);
                rawDataChunk.writeSimpleValue(rxBuffer.state);
            }

            size_t length = rawDataChunk.getWriteSize();
            logFile.write(reinterpret_cast<uint8_t *>(&rawDataFileBytes), length);
            // _printCBOR(rawDataFileBytes, length);

            // Sleep
            vTaskDelay(LOGFILE_WRITE_INTERVAL / portTICK_PERIOD_MS);
        }
    }

    void _printCBOR(uint8_t *message, size_t len)
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
};
