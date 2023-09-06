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

#include <EdgingFilterService.h>

EdgingFilterService::EdgingFilterService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager) : _httpEndpoint(FilterData::read,
                                                                                                                           FilterData::update,
                                                                                                                           this,
                                                                                                                           server,
                                                                                                                           FILTER_SETTINGS_PATH,
                                                                                                                           securityManager,
                                                                                                                           AuthenticationPredicates::NONE_REQUIRED),
                                                                                                             _fsPersistence(FilterData::read, FilterData::update, this, fs, FILTER_SETTINGS_FILE)
{
    // configure settings service update handler to update state
    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(); },
                     false);
}

void EdgingFilterService::begin()
{
    _fsPersistence.readFromFS();
}

void EdgingFilterService::onConfigUpdated()
{
    pressureKalmanFilter.setEstimateError(_state.estimateError);
    pressureKalmanFilter.setMeasurementError(_state.measurementError);
    pressureKalmanFilter.setProcessNoise(_state.processNoise);
}

float EdgingFilterService::updateEstimate(float input)
{
    return pressureKalmanFilter.updateEstimate(input);
}
