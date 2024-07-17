/*
Calibration.h
Translate touch screen coordinations to display coordinates.

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

#include <ILI9486.h>
#include <XPT2046_Touchscreen.h>

struct LinearEquation {
    LinearEquation(double m, double c): m(m), c(c) {}
    double m;
    double c;
    double calculate(double x) { return x * m + c; }
};

class Calibration {
public:
    Calibration(bool swapxy, ILI9486 *display, XPT2046_Touchscreen *touch);

    void calibrate(); // Display two points on screen to calibrate
    void calibrate(double mx, double cx, double my, double cy); // Used to pass consts to avoid calibration on each startup 

    void translate(TS_Point &point); // Translate x and y position to match calibration

private:
    bool swapxy; // Store need for swapping x and y coordinates in portrait orientations
    ILI9486 *display;
    XPT2046_Touchscreen *touch;
    LinearEquation xEq; // Used to translate x position
    LinearEquation yEq; // Used to translate y position

    void swapXY(TS_Point &point); // Swap x coordinate with y coordinate
};
