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

#define IMG_BUFFER 40 // Loading image buffer size in pixels
#define INTRO_DISPLAY_TIME 5000 // Time of intro display in miliseconds
#define TOUCH_DELAY 500
#define BUFFER_LOAD_TIMES 5 // How many last load times to store

#define DISP_TIME_LEVELS 5
#define DEFAULT_DISP_TIME_LEVEL 2
constexpr uint32_t dispTimeLevels[DISP_TIME_LEVELS] = {5000, 30000, 60000, 300000, 600000};

#define BRIGHTNESS_LEVELS 4
constexpr uint8_t brightnessLevels[BRIGHTNESS_LEVELS] = {10, 40, 90, 255};

class DigitalFrame {
public:
    enum State {
        IMAGE_DISPLAY,
        STATS_DISPLAY,
        MENU_DISPLAY,
        SET_BRIGHTNESS,
        SET_DISP_TIME
    };
    
    DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage, String introFile);

    void loop(); // This method must be called in arduino loop function

private:
    State currentState; // Program state
    ILI9486 *display;
    XPT2046_Touchscreen *touch;
    Calibration *calibration;
    SDStorage *storage;
    uint32_t imagesDisplayed; // Number of images displayed so far 
    uint32_t imageNumberInDir; // Number of images in directory
    uint32_t invalidImages; // Number of skipped images (invalid)
    uint32_t lastImageDisTime; // Time of last image display
    uint32_t lastTouchTime; // Time of last touch
    uint32_t loadTimes[BUFFER_LOAD_TIMES]; // Last images load time
    uint8_t loadIndex; // Index into which last image load time was saved (always 0 <= loadIndex < BUFFER_LOAD_TIMES)
    uint8_t brightnessLevel; // Current brightness level
    uint32_t dispTimeLevel; // Single image display time
    bool forceImageDisplay; // Force image display, do not look on display time

    uint32_t loadImage();
    uint32_t loadImagePortion();
    uint32_t getLoadTime(); // Get average load time of last few images
    void countImages();
    bool checkTouch();
    void getTouch(uint16_t &x, uint16_t &y);
    void changeState(State newState);

    void displayStats(); // Display statistic on the screen
    void displayMenu(); // Display menu with options on the screen
    void displaySetBrightness(); // Display menu to set brightness
    void displaySetDispTime(); // Display menu to set brightness
    
    void handleMenuTouch(uint16_t x, uint16_t y); // Handle screen touch while menu display
    void handleSetBrightnessTouch(uint16_t x, uint16_t y); // Handle screen touch while setting brightness
    void handleSetDispTimeTouch(uint16_t x, uint16_t y); // Handle screen touch while setting display time

    void saveSettings();
    void loadSettings();
};
