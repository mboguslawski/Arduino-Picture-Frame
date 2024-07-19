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

#define X_BEGIN 555
#define X_END 3551
#define Y_BEGIN 3783
#define Y_END 392

#define BUFFER_SIZE 100

#define DISPLAY_TIME 5000 // 5 seconds

#define TP_CS 4
#define TP_IRQ 3

ILI9486 *display;
XPT2046_Touchscreen *touch;
Calibration *calibration;
SDStorage *storage;

void setup() {
	display = new ILI9486(ILI9486::R2L_U2D);
	
	digitalWrite(ILI9486_CS, 1);
	digitalWrite(4, 1);
	storage = new SDStorage(5, display->getWidth(), display->getHeight(), "/");
	
	touch = new XPT2046_Touchscreen(TP_CS, TP_IRQ);
	touch->begin();
	
	calibration = new Calibration(true, display, touch);
	calibration->calibrate(X_BEGIN, X_END, Y_BEGIN, Y_END);
}

void loop() {
	uint16_t buffer[BUFFER_SIZE];
	storage->nextImage();

	display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / BUFFER_SIZE; i++) {
		storage->readImagePortion(buffer, BUFFER_SIZE);
		display->writeBuffer(buffer, BUFFER_SIZE);
	}

	display->drawString(10, 460, (uint8_t*)storage->getCurrentImage().name(), ILI9486::L, ILI9486_BLACK);

	delay(DISPLAY_TIME);
}
