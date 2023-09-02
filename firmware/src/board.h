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

#define SERIAL_BAUD_RATE 115200

#define NEOPIXEL 27

#define ENV_EN 4

#define EOC_PIN 35
#define RST_PIN -1

#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_SDA_1 32
#define I2C_SCL_1 33

#define USER_BUTTON 36

#define VIBRATE_0 20
#define VIBRATE_1 13

#define BAT_ALRT 26

#define MAG_READY 25
#define MAG_INT 39

#define GYRO_INT_1 38
#define GYRO_INT_2 37

#define BATTERY_UPDATE_INTERVAL 2000
#define LOW_BATTERY_SHUT_OFF_SOC 4 // We can alert at anywhere between 1% - 32%:
#define VBUS 19
