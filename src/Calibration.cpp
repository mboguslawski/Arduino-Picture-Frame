/*
Calibration.cpp
Calibration class implementation.

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

#include "Calibration.h"

Calibration::Calibration(bool swapxy, ILI9486 *display, XPT2046_Touchscreen *touch):
    swapxy(swapxy),
    display(display),
    touch(touch),
    xEq(1, 0),
    yEq(1, 0)
{}

void Calibration::calibrate() {
    display->clear(ILI9486_BLACK);
    
    // Coordinates of points to display
    constexpr uint16_t p1x = 40;
    constexpr uint16_t p1y = 40;
    const uint16_t p2x = this->display->getWidth() - 40;
    const uint16_t p2y = this->display->getHeight() - 40;
    constexpr uint16_t size = 20;
    
    // Draw first point
    delay(200);
    display->drawCircle(p1x, p1y, size, ILI9486_WHITE, true);
    
    // Wait for touch
	while (!touch->tirqTouched() || !touch->touched()) {}

    // Get raw touch panel position 
	TS_Point t1 = touch->getPoint();
    
    // Display second point 
	delay(200);
	display->drawCircle(p2x, p2y, size, ILI9486_WHITE, true);
	
    // Wait for touch
	while (!touch->tirqTouched() || !touch->touched()) {}

    // Get raw touch panel position
	TS_Point t2 = touch->getPoint();

    // Swap x nad y coordinates if needed
    if (this->swapxy) {
        swapXY(t1);
        swapXY(t2);
    }

    // Calculate values used for translation
	this->xEq.m = (double)(p2x - p1x) / (double)(t2.x - t1.x);
	this->xEq.c = (double)p1x - this->xEq.m * (double)t1.x;
	this->yEq.m = (double)(p2y - p1y) / (double)(t2.y - t1.y);
	this->yEq.c = (double)p1y - this->yEq.m * (double)t1.y;

    // Print calibration values
    if (Serial) {
        Serial.print("mx = ");
        Serial.println(xEq.m);
        Serial.print("cx = ");
        Serial.println(xEq.c);
        Serial.print("my = ");
        Serial.println(yEq.m);
        Serial.print("cy = ");
        Serial.println(yEq.c);
    }

    display->clear();
}

void Calibration::translate(TS_Point &point) {
    if (this->swapxy) {
        swapXY(point);
    }

    point.x = xEq.calculate(point.x);
    point.y = yEq.calculate(point.y);
}

void Calibration::swapXY(TS_Point &point) {
    int16_t tmp = point.x;
    point.x = point.y;
    point.y = tmp;
}
