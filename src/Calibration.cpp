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
    xBegin(0),
    xEnd(0),
    yBegin(0),
    yEnd(0)
{}

void Calibration::calibrate() {
    // Coordinates of points to display
    const uint16_t left = 40;
    const uint16_t right = this->display->getWidth() - 40;
    const uint16_t bottom = 40;
    const uint16_t up = this->display->getHeight() - 40;
    const uint16_t size = 20;
    
    constexpr uint16_t touchDelay = 200;
    constexpr uint16_t repeat = 3;

    // 0-left bottom, 1-right bottom, 2-left up, 3-left up
    uint16_t positions[4][2] = {{left, bottom}, {right, bottom}, {left, up}, {right, up}};
    TS_Point points[4];


    // Draw points one by one
    display->clear(ILI9486_BLACK);
    for (uint8_t i = 0; i < 4; i++) {

        display->drawCircle(positions[i][0], positions[i][1], size, ILI9486_WHITE, true);
        
        // Repeat measure
        uint16_t x = 0, y = 0;
        for (uint8_t j = 0; j < repeat; j++) {
            delay(touchDelay);
            while (!touch->tirqTouched() || !touch->touched()) {} // Wait for touch

            TS_Point p = touch->getPoint();
            x += p.x;
            y += p.y;
        }

        display->drawCircle(positions[i][0], positions[i][1], size, ILI9486_BLACK, true);

        // Set point coordinates as average
        points[i].x = x / repeat;
        points[i].y = y / repeat;

        if (swapxy) { swapXY(points[i]); }
    }

    uint16_t xb = (points[0].x + points[2].x) / 2;
    uint16_t xe = (points[1].x + points[3].x) / 2;
    uint16_t yb = (points[0].y + points[1].y) / 2;
    uint16_t ye = (points[2].y + points[3].y) / 2;


    this->xBegin = map(0, positions[0][0], positions[1][0], xb, xe);
    this->xEnd = map(this->display->getWidth() - 1, positions[0][0], positions[1][0], xb, xe);

    this->yBegin = map(0, positions[0][1], positions[2][1], yb, ye);
    this->yEnd = map(this->display->getHeight() - 1, positions[0][1], positions[2][1], yb, ye);


    // Print calibration values
    if (Serial) {
        Serial.print("xBegin = ");
        Serial.println(xBegin);
        Serial.print("xEnd = ");
        Serial.println(xEnd);
        Serial.print("yBegin = ");
        Serial.println(yBegin);
        Serial.print("yEnd = ");
        Serial.println(yEnd);
    }

    display->clear();
}

void Calibration::calibrate(uint16_t xBegin, uint16_t xEnd, uint16_t yBegin, uint16_t yEnd) {
    this->xBegin = xBegin;
    this->xEnd = xEnd;
    this->yBegin = yBegin;
    this->yEnd = yEnd;
}

void Calibration::translate(TS_Point &point) {
    if (this->swapxy) {
        swapXY(point);
    }

    point.x = map(point.x, xBegin, xEnd, 0, this->display->getWidth() - 1);
    point.y = map(point.y, yBegin, yEnd, 0, this->display->getHeight() - 1);
}

void Calibration::swapXY(TS_Point &point) {
    int16_t tmp = point.x;
    point.x = point.y;
    point.y = tmp;
}
