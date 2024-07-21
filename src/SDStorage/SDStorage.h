/*
SDStorage.h

SDStorage class contains all SD related functions.
This class is suited for digital picture display.
All images should be 24bit bmp with resolution that exactly matches display.

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

#include <SD.h>

#define SETTINGS_FILE "settings.txt"

class SDStorage {
public:

    SDStorage(uint8_t SD_CS_PIN, uint16_t disWidth, uint16_t disHeight, String imageDir);

    uint16_t nextImage(); // Switch to next image available in imageDir, return number of invalid images
    bool toImage(String imageFile); // Go to specific image

    void readImagePortion(uint16_t *buffer, uint16_t size); // Load portion of image into buffer

    File getCurrentImage(); // Get current image object

    void saveSettings(uint8_t *settings, uint16_t nBytes);
    void loadSettings(uint8_t *settings, uint16_t nBytes); 

private:
    File imageDir; // Directory with images
    File currentImage;
    bool initialized; // True if SD card was initialized without errors
    uint16_t disWidth; // Display width [px]
    uint16_t disHeight; // Display height [px]

    uint16_t RGB24ToRGB16(uint8_t r, uint8_t g, uint8_t b); // Convert RGB24 format to RGB 16
    bool validateImage(File &image);
    uint32_t readLittleIndian32(File f); // Read data and convert to big indian format
    uint16_t readLittleIndian16(File f); // Read data and convert to big indian format
};
