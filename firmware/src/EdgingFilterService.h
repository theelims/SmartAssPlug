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
#include <SimpleKalmanFilter.h>

#define FILTER_SETTINGS_FILE "/config/filterSettings.json"
#define FILTER_SETTINGS_PATH "/rest/filterSettings"

class FilterData
{
public:
    float measurementError = 1.0;
    float estimateError = 1.0;
    float processNoise = 1.0;

    static void read(FilterData &settings, JsonObject &root)
    {
        root["measurement_error"] = settings.measurementError;
        root["estimate_error"] = settings.estimateError;
        root["process_oise"] = settings.processNoise;
    }

    static StateUpdateResult update(JsonObject &root, FilterData &settings)
    {
        settings.measurementError = root["measurement_error"] | 1.0;
        settings.estimateError = root["estimate_error"] | 1.0;
        settings.processNoise = root["process_noise"] | 1.0;
        return StateUpdateResult::CHANGED;
    }
};

class EdgingFilterService : public StatefulService<FilterData>
{
public:
    EdgingFilterService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager);

    void begin();

    void onConfigUpdated();

    float updateEstimate(float input);

private:
    HttpEndpoint<FilterData> _httpEndpoint;
    FSPersistence<FilterData> _fsPersistence;
    SimpleKalmanFilter pressureKalmanFilter = SimpleKalmanFilter(_state.measurementError, _state.estimateError, _state.processNoise);
};
