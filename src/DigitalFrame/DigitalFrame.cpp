/*
DigitalFrame.cpp

DigitalFrame class implementation.

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

#include "DigitalFrame.h"

DigitalFrame::DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage, String introFile):
    currentState(IMAGE_DISPLAY),
    display(display),
    touch(touch),
    calibration(calibration),
    storage(storage),
    imagesDisplayed(0),
    invalidImages(0),
    lastImageDisTime(0),
    lastTouchTime(0),
    loadIndex(0),
    brightnessLevel(BRIGHTNESS_LEVELS - 1)
{
    // Initialize image load times
    for (uint8_t i = 0; i < BUFFER_LOAD_TIMES; i++) { this->loadTimes[i] = 0; }

    this->countImages();
    
    // Display intro image
    storage->toImage(introFile);
	this->loadImage();
	display->changeDefaultBacklight(brightnessLevels[brightnessLevel]);
	display->setDefaultBacklight();

	delay(INTRO_DISPLAY_TIME);
}

void DigitalFrame::loop() {
    switch(this->currentState) {
    case IMAGE_DISPLAY:
        // Display new image if display time for old image passed
        if (millis() - this->lastImageDisTime > DISPLAY_TIME) {
            this->invalidImages += storage->nextImage();
            display->openWindow(0, 0, display->getWidth(), display->getHeight());

            uint32_t startTime = millis();
            // Load image by portions and check for touch
            for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
                this->loadImagePortion();
                if (checkTouch()) { break; }
            }

            // Full image loaded
            if (this->currentState == IMAGE_DISPLAY) {
                this->imagesDisplayed++;

                // Calculate place for time placement
                this->loadIndex = (this->loadIndex + 1) % BUFFER_LOAD_TIMES;
                
                this->loadTimes[loadIndex] = millis() - startTime;
                this->lastImageDisTime = millis();
            }
        }
        break;
    }

    // Check if touch occurred
    this->checkTouch();
}

uint32_t DigitalFrame::loadImage() {
    uint32_t time = millis();
	uint16_t buffer[IMG_BUFFER];

	// Load image into display
    display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
		storage->readImagePortion(buffer, IMG_BUFFER);
		display->writeBuffer(buffer, IMG_BUFFER);
	}

	return millis() - time;
}

uint32_t DigitalFrame::loadImagePortion() {
    uint32_t time = millis();
	uint16_t buffer[IMG_BUFFER];

    // Load only one portion
	storage->readImagePortion(buffer, IMG_BUFFER);
	display->writeBuffer(buffer, IMG_BUFFER);

	return millis() - time;
}

void DigitalFrame::countImages() {
    this->imageNumberInDir = 0;

    // Count images until same image occurred after directory rewind
    String name = storage->getCurrentImage().name();
    do {
        imageNumberInDir++;
        storage->nextImage();
    } while (name != storage->getCurrentImage().name());
}

bool DigitalFrame::checkTouch() {
    // To prevent multi-touches 
    if (millis() - lastTouchTime < TOUCH_DELAY) { return false; }
    
    bool touched = (touch->tirqTouched() && touch->touched());

    if (!touched) { return false; }

    // If touch occurred
    uint16_t x, y;
    this->getTouch(x, y);
    lastTouchTime = millis();

    // Switch state depending on current state
    switch(this->currentState) {
        case IMAGE_DISPLAY:
            this->displayMenu();
            this->currentState = MENU_DISPLAY;
            break;
        case STATS_DISPLAY:
            this->currentState = IMAGE_DISPLAY;
            break;
        case MENU_DISPLAY:
            this->handleMenuTouch(x, y);
            break;
        case SET_BRIGHTNESS:
            this->handleSetBrightnessTouch(x, y);
    }

    return true;
}

void DigitalFrame::getTouch(uint16_t &x, uint16_t &y) {
    TS_Point p = touch->getPoint();
    calibration->translate(p);

    x = p.x;
    y = p.y;
}

uint32_t DigitalFrame::getLoadTime() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < BUFFER_LOAD_TIMES; i++) {
        sum += this->loadTimes[i];
    }

    // If full buffer not loaded yet
    uint32_t dived = min(BUFFER_LOAD_TIMES, this->imagesDisplayed);
    if (dived == 0) {
        return 0;
    }

    return sum / min(BUFFER_LOAD_TIMES, this->imagesDisplayed);    
}

void DigitalFrame::displayStats() {
    // Display statistics panel
    display->clear();
    display->drawHLine(0, 415, display->getWidth(), ILI9486_WHITE);
        
    String s1 = "Image number: ";
    s1 += this->imageNumberInDir;
    display->drawString(10, 395, (uint8_t*)s1.c_str(), ILI9486::L, ILI9486_WHITE);

    String s2 = "Load time: ";
    s2 += this->getLoadTime();
    s2 += " ms";
    display->drawString(10, 375, (uint8_t*)s2.c_str(), ILI9486::L, ILI9486_WHITE);

    String s3 = "Images displayed: ";
    s3 += this->imagesDisplayed;
    display->drawString(10, 355, (uint8_t*)s3.c_str(), ILI9486::L, ILI9486_WHITE);

    String s4 = "Invalid images: ";
    s4 += this->invalidImages;
    display->drawString(10, 335, (uint8_t*)s4.c_str(), ILI9486::L, ILI9486_WHITE);
}

void DigitalFrame::displayMenu() {
    display->clear();
    
    display->drawString(10, 420, "Set brightness", ILI9486::L, ILI9486_WHITE);
    display->drawHLine(0, 360, display->getWidth(), ILI9486_WHITE);
    display->drawString(10, 300, "Set display time", ILI9486::L, ILI9486_WHITE);
    display->drawHLine(0, 240, display->getWidth(), ILI9486_WHITE);
    display->drawString(10, 180, "Show statistics", ILI9486::L, ILI9486_WHITE);
    display->drawHLine(0, 120, display->getWidth(), ILI9486_WHITE);
    display->drawString(10, 60, "Go back", ILI9486::L, ILI9486_WHITE);
}

void DigitalFrame::handleMenuTouch(uint16_t x, uint16_t y) {
    // Touch on set brightness option
    if (y > 360) {
        this->displaySetBrightness();
        this->currentState = SET_BRIGHTNESS;
    }
    // Touch on set display time option
    else if (y > 240) {
        this->currentState = SET_DISP_TIME;
    }
    // Touch on show statistics option
    else if (y > 120) {
        this->displayStats();
        this->currentState = STATS_DISPLAY;
    }
    // Touch on go back option
    else {
        this->currentState = IMAGE_DISPLAY;
    }
}

void DigitalFrame::displaySetBrightness() {
    display->clear();
    
    display->drawString(10, 420, "Brightness up", ILI9486::L, ILI9486_WHITE);
    display->drawHLine(0, 360, display->getWidth(), ILI9486_WHITE);
    
    String current = "";
    current += this->brightnessLevel + 1;
    display->drawString(10, 300, (uint8_t*)current.c_str(), ILI9486::L, ILI9486_WHITE);

    display->drawHLine(0, 240, display->getWidth(), ILI9486_WHITE);
    display->drawString(10, 180, "Brightness down", ILI9486::L, ILI9486_WHITE);
    display->drawHLine(0, 120, display->getWidth(), ILI9486_WHITE);
    display->drawString(10, 60, "Go back", ILI9486::L, ILI9486_WHITE);
}

void DigitalFrame::handleSetBrightnessTouch(uint16_t x, uint16_t y) {
    // Touch on brightness up
    if (y > 360) {
        if (this->brightnessLevel == BRIGHTNESS_LEVELS - 1) { return; }
        this->brightnessLevel++;
        display->changeDefaultBacklight(brightnessLevels[this->brightnessLevel]);
        display->setDefaultBacklight();
    }
    // Touch on current brightness level
    else if (y > 240) {
    }
    // Touch on brightness down
    else if (y > 120) {
        if (this->brightnessLevel == 0) { return; }
        this->brightnessLevel--;
        display->changeDefaultBacklight(brightnessLevels[this->brightnessLevel]);
        display->setDefaultBacklight();
    }
    // Touch on go back option
    else {
        this->currentState = IMAGE_DISPLAY;
    }

    // Update brightness level
    if ( (y > 360) || ( (y < 240) && (y > 120) ) ) {
        display->fill(0, 241, display->getWidth(), 360, ILI9486_BLACK);
        String current = "";
        current += this->brightnessLevel + 1;
        display->drawString(10, 300, (uint8_t*)current.c_str(), ILI9486::L, ILI9486_WHITE);
    }
}
