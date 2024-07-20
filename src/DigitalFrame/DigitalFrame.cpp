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

#define IMG_BUFFER 20 // Loading image buffer size in pixels
#define DISPLAY_TIME 5000 // Time of photo display in miliseconds
#define INTRO_DISPLAY_TIME 5000 // Time of intro display in miliseconds

DigitalFrame::DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage, String introFile):
    display(display),
    touch(touch),
    calibration(calibration),
    storage(storage),
    introFile(introFile)
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
    storage->nextImage();
    this->loadImage();
    delay(DISPLAY_TIME);
}

uint32_t DigitalFrame::loadImage() {
    uint32_t time = millis();
	uint16_t buffer[IMG_BUFFER];

	display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
		storage->readImagePortion(buffer, IMG_BUFFER);
		display->writeBuffer(buffer, IMG_BUFFER);
	}

	return millis() - time;
}

void DigitalFrame::countImages() {
    this->imageNumber = 0;

    String name = storage->getCurrentImage().name();

    do {
        imageNumber++;
        storage->nextImage();
    } while (name != storage->getCurrentImage().name());
}
