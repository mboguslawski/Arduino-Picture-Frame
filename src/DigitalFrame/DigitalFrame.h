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

#define INTRO_BMP "intro.bmp"
#define MENU_BMP "m.bmp"
#define BRIGHTNESS_BMP "b.bmp"
#define DISP_TIME_BMP "t.bmp"
#define DISP_MODE_BMP "o.bmp"
#define SET_TURN_OFF_BMP "f.bmp"

#define MAX_IMG_N 256 // Maximum number of images in directory (used for mageRandDisplayed array)

#define IMG_BUFFER 40 // Loading image buffer size in pixels
#define INTRO_DISPLAY_TIME 5000 // Time of intro display in miliseconds
#define TOUCH_DELAY 500

#define TURN_OFF_TIMES_N 5
constexpr uint32_t turnOffTimes[TURN_OFF_TIMES_N] = {300000, 900000, 1800000, 2700000, 3600000};

#define DISP_TIME_LEVEL_N 5
#define DEFAULT_DISP_TIME_LEVEL 2
constexpr uint32_t dispTimeLvls[DISP_TIME_LEVEL_N] = {5000, 30000, 60000, 300000, 600000};

#define BRIGHTNESS_LEVELS_N 4
constexpr uint8_t brightnessLvls[BRIGHTNESS_LEVELS_N] = {10, 40, 90, 255};

class DigitalFrame {
public:
    enum State {
        IMAGE_DISPLAY,
        MENU_DISPLAY,
        SET_BRIGHTNESS,
        SET_DISP_TIME,
        SET_DISP_MODE,
        SET_TURN_OFF,
        SLEEP,
        SD_ERROR
    };
    
    enum DispMode {
        RANDOM = 0,
        IN_ORDER = 1,
        ONLY_CURRENT = 2
    };

    DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage);

    void loop(); // This method must be called in arduino loop function
    void(* reset) (void) = 0; // Calling this function will reset arduino

private:
    ILI9486 *display;
    XPT2046_Touchscreen *touch;
    Calibration *calibration;
    SDStorage *storage;
    State state; // Program state
    DispMode dispMode;
    uint32_t imageNumberInDir; // Number of images in directory
    uint32_t randDisplayedN; // Store number of images displayed in random mode
    uint32_t lastImageDisTime; // Time of last image display
    uint32_t lastTouchTime; // Time of last touch
    uint32_t turnOffTime; // Scheduled turn off time
    uint8_t brightnessLvl; // Current brightness level
    uint8_t dispTimeLvl; // Single image display time
    uint8_t turnOffTimeLvl; // Currently displayed time for turn off schedule
    bool turnOffScheduled; // True if turn off was scheduled
    bool forceImageDisplay; // Force image display, do not look on display time
    bool imageRandDisplayed[MAX_IMG_N]; // Store information if image was displayed in random mode

    void moveToNextImg();
    void loadImage();
    void loadImagePortion();
    void countImages();
    bool touched();
    void getTouchPos(uint16_t &x, uint16_t &y);
    void changeState(State newState);

    void dispLevel(uint8_t level, uint8_t max); // Display menu to set brightness
    void dispSelected(uint8_t selected);
    void dispTime(uint32_t time);
    void dispStorageError();

    void handleMenuTouch(uint16_t x, uint16_t y); // Handle screen touch while menu display
    void handleSetBrightnessTouch(uint16_t x, uint16_t y); // Handle screen touch while setting brightness
    void handleSetDispTimeTouch(uint16_t x, uint16_t y); // Handle screen touch while setting display time
    void handleSetDispModeTouch(uint16_t x, uint16_t y); // Handle screen touch while setting display mode
    void handleSetTurnOffTimeTouch(uint16_t x, uint16_t y); // Handle screen touch while scheduling turn off

    void saveSettings();
    void loadSettings();
};
