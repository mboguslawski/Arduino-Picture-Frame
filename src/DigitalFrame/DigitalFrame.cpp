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
    introFile(introFile),
    lastImageDisTime(0),
    lastTouchTime(0)
{
    this->countImages();
    
    // Display intro image
    storage->toImage("intro.bmp");
	this->loadImage();
	display->changeDefaultBacklight(UINT8_MAX);
	display->setDefaultBacklight();
	delay(INTRO_DISPLAY_TIME);
}

void DigitalFrame::loop() {
    switch(this->currentState) {
    case IMAGE_DISPLAY:
        // Display new image if display time for old image passed
        if (millis() - this->lastImageDisTime > DISPLAY_TIME) {
            storage->nextImage();
            display->openWindow(0, 0, display->getWidth(), display->getHeight());

            // Load image by portions and check for touch
            for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
                this->loadImagePortion();
                if (checkTouch()) { break; }
            }

            this->lastImageDisTime = millis();
        }
        break;

    case STATS_TO_DISPLAY:
        // Display statistics panel
        display->clear();
        display->drawString(10, 440, "Statistics", ILI9486::XL, ILI9486_WHITE);
        display->drawHLine(0, 415, display->getWidth(), ILI9486_WHITE);
        
        String s = "Image number: ";
        s += this->imageNumber;

        display->drawString(10, 400, (uint8_t*)s.c_str(), ILI9486::L, ILI9486_WHITE);

        this->currentState = STATS_DISPLAYED;
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
    this->imageNumber = 0;

    // Count images until same image occurred after directory rewind
    String name = storage->getCurrentImage().name();
    do {
        imageNumber++;
        storage->nextImage();
    } while (name != storage->getCurrentImage().name());
}

bool DigitalFrame::checkTouch() {
    // To prevent multi-touches 
    if (millis() - lastTouchTime < TOUCH_DELAY) { return false; }
    
    bool touched = (touch->tirqTouched() && touch->touched());

    if (!touched) { return false; }

    // If touch occurred
    lastTouchTime = millis();

    // Switch state depending on current state
    switch(this->currentState) {
        case IMAGE_DISPLAY:
            this->currentState = STATS_TO_DISPLAY;
            break;
        case STATS_DISPLAYED:
            this->currentState = IMAGE_DISPLAY;
            break;
        case STATS_TO_DISPLAY:
            this->currentState = IMAGE_DISPLAY;
            break;
    }

    return true;
}

void DigitalFrame::getTouch(uint16_t &x, uint16_t &y) {
    TS_Point p = touch->getPoint();
    calibration->translate(p);

    x = p.x;
    y = p.y;
}