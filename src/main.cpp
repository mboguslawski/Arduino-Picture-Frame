/*
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

#include <Arduino.h>
#include <ILI9486.h>
#include <XPT2046_Touchscreen.h>

#include "SDStorage.h"
#include "Calibration.h"

#define DISPLAY_TIME 5000 // 5 seconds
#define INTRO_TIME 5000 // Two seconds

// Pin configuration
#define ILI9486_CS 10
#define ILI9486_BL 9
#define ILI9486_RST 8
#define ILI9486_DC 7
#define XPT2046_CS 4
#define XPT2046_IRQ 3

// XPT2046 touch coordinates calibration 
#define X_BEGIN 555
#define X_END 3551
#define Y_BEGIN 3783
#define Y_END 392

#define BUFFER_SIZE 40

ILI9486 *display;
XPT2046_Touchscreen *touch;
Calibration *calibration;
SDStorage *storage;

// Load image marked as current in SDStorage object
// Return time needed for display (ms)
uint32_t loadImage();

void setup() {
	display = new ILI9486(ILI9486_CS, ILI9486_BL, ILI9486_RST, ILI9486_DC, ILI9486::R2L_U2D, 0, ILI9486_BLACK);
	
	digitalWrite(ILI9486_CS, 1);
	digitalWrite(4, 1);
	storage = new SDStorage(5, display->getWidth(), display->getHeight(), "/images");
	
	touch = new XPT2046_Touchscreen(XPT2046_CS, XPT2046_IRQ);
	touch->begin();
	
	calibration = new Calibration(true, display, touch);
	calibration->calibrate(X_BEGIN, X_END, Y_BEGIN, Y_END);

	storage->toImage("intro.bmp");
	loadImage();
	display->changeDefaultBacklight(UINT8_MAX);
	display->setDefaultBacklight();
	delay(INTRO_TIME);

}

void loop() {
	storage->nextImage();
	loadImage();
	delay(DISPLAY_TIME);
}

uint32_t loadImage() {
	uint32_t time = millis();
	uint16_t buffer[BUFFER_SIZE];

	display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / BUFFER_SIZE; i++) {
		storage->readImagePortion(buffer, BUFFER_SIZE);
		display->writeBuffer(buffer, BUFFER_SIZE);
	}

	return millis() - time;
}
