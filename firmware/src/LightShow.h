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

#include <board.h>
#include <Arduino.h>
#include <NeoPixelBus.h>

#define colorSaturation 128

// RgbColor orgasm(colorSaturation, 0, 0);                /* red */
// RgbColor edging(0, 0, colorSaturation);                /* blue */
// RgbColor aroused(colorSaturation, 0, colorSaturation); /* pink */
// RgbColor stdby(colorSaturation);                       /* white */
// RgbColor off(0);                                       /* black */
// RgbColor neutral(0, colorSaturation, 0);               /* green */

class LightShow
{
public:
    LightShow()
    {
        // begin neopixel lightshow
        logo.Begin();
    }

    void setLight()
    {
        RgbColor stdby(colorSaturation);
        logo.SetPixelColor(0, stdby);
        logo.SetPixelColor(1, stdby);
        logo.Show();
    }

private:
    NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod> logo = NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0800KbpsMethod>(2, NEOPIXEL);
};