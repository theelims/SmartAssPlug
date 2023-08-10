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

EdgingMqttSettingsService::EdgingMqttSettingsService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager) : _httpEndpoint(EdgingMqttSettings::read,
                                                                                                                                       EdgingMqttSettings::update,
                                                                                                                                       this,
                                                                                                                                       server,
                                                                                                                                       AROUSAL_BROKER_SETTINGS_PATH,
                                                                                                                                       securityManager,
                                                                                                                                       AuthenticationPredicates::IS_ADMIN),
                                                                                                                         _fsPersistence(EdgingMqttSettings::read, EdgingMqttSettings::update, this, fs, AROUSAL_BROKER_SETTINGS_FILE)
{
}

void EdgingMqttSettingsService::begin()
{
    _fsPersistence.readFromFS();
}
