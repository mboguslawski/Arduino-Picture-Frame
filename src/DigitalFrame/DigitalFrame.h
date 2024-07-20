/*
DigitalFrame.h

DigitalFrame class manages display, touch screen and SD card.
All digital frame functionalities are implemented here.

Copyright (C) 2024 Mateusz Bogusławski, E: mateusz.boguslawski@ibnet.pl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.
*/

#pragma once

#include <Arduino.h>
#include <ILI9486.h>
#include <XPT2046_Touchscreen.h>

#include "../Calibration/Calibration.h"
#include "../SDStorage/SDStorage.h"

class DigitalFrame {
public:
    DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage, String introFile);

    void loop(); // This method must be called in arduino loop function

private:
    ILI9486 *display;
    XPT2046_Touchscreen *touch;
    Calibration *calibration;
    SDStorage *storage;

    String introFile; // Path to file with intro
    uint32_t imageNumber; // Number of images

    uint32_t loadImage();
    void countImages();
};
