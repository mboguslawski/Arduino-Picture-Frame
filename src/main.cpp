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
#include <SD.h>

#include "Calibration.h"

#define X_BEGIN 555
#define X_END 3551
#define Y_BEGIN 3783
#define Y_END 392

ILI9486 *display;
XPT2046_Touchscreen *touch;
Calibration *calibration;

void setup() {
	Serial.begin(9600);

	display = new ILI9486(ILI9486::R2L_U2D);
	touch = new XPT2046_Touchscreen(4, 3);
	touch->begin();
	calibration = new Calibration(true, display, touch);

	calibration->calibrate(X_BEGIN, X_END, Y_BEGIN, Y_END);
}

void loop() {
	if (touch->tirqTouched()) {
		if (touch->touched()) {
			TS_Point p = touch->getPoint();
			calibration->translate(p);
			display->drawCircle(p.x, p.y, 5, ILI9486_WHITE, true);
			Serial.print("Pressure = ");
			Serial.print(p.z);
			Serial.print(", x = ");
			Serial.print(p.x);
			Serial.print(", y = ");
			Serial.print(p.y);
			delay(100);
			Serial.println();
		}
	}
}
