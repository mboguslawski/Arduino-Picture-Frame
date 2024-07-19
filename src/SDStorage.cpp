/*
SDStorage.cpp

SDStorage class implementation.

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

#include "SDStorage.h"

SDStorage::SDStorage(uint8_t SD_CS_PIN, uint16_t disWidth, uint16_t disHeight, String imageDir):
    disHeight(disHeight),
    disWidth(disWidth),
    initialized(false)
{
    // Initialize SD card
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, 1);
    initialized = SD.begin(SD_CS_PIN);

    // Do not open directories if not initialized (SD card not inserted?)
    if (!initialized) {return; }

    this->imageDir = SD.open(imageDir);
    this->nextImage();
}

File SDStorage::getCurrentImage() {
    return this->currentImage;
}

void SDStorage::nextImage() {
    while (true) {
        this->currentImage.close();
        this->currentImage = this->imageDir.openNextFile();

        // Rewind directory if needed
        if (!this->currentImage) {
            this->imageDir.rewindDirectory();
            this->currentImage.close();
            this->currentImage = this->imageDir.openNextFile();
        }

        if (this->validateImage(this->currentImage)) { break; }
    }
}

bool SDStorage::toImage(String image) {
    this->currentImage.close();
    this->currentImage = SD.open(image);
    return this->validateImage(this->currentImage);
}

uint16_t SDStorage::RGB24ToRGB16(uint8_t r, uint8_t g, uint8_t b) {
    return (( (r) >> 3 ) << 11 ) | (( (g) >> 2 ) << 5) | ( (b) >> 3);
}

void SDStorage::readImagePortion(uint16_t *buffer, uint16_t size) {
    uint8_t pixels[size*3];
    this->currentImage.read(pixels, size*3);

    for (uint16_t i  = 0; i < size; i++) {
        buffer[i] = this->RGB24ToRGB16(pixels[i*3 + 2], pixels[i*3 + 1], pixels[i*3 + 0]);
    }
}


bool SDStorage::validateImage(File &image) {
    if (this->readLittleIndian16(image) != 0x4D42) {
        // Magic bytes missing
        return false;
    }
    
    // Read and ignore size
    this->readLittleIndian32(image);

    // Read and ignore creator bytes
    this->readLittleIndian32(image);

    // Offset between file head and image
    uint32_t offset = this->readLittleIndian32(image);

    // Ignore header size
    this->readLittleIndian32(image);

    uint32_t imageWidth = this->readLittleIndian32(image);
    uint32_t imageHeight = this->readLittleIndian32(image);

    // Check if size correct
    if ( max(imageWidth, imageWidth) != max(disWidth, disHeight)
        && min(imageWidth, imageHeight) != min(disHeight, disWidth) )
    {
        Serial.println(1);
        return false;
    }

    if (this->readLittleIndian16(image) != 1) {
        return false;
    }

    this->readLittleIndian16(image);
    
    if (this->readLittleIndian32(image) != 0) {
        return false;
    }

    // Move to data
    image.seek(offset);

    return true;
}

uint16_t SDStorage::readLittleIndian16(File f) {
    uint16_t d;
    uint8_t b;
    b = f.read();
    d = f.read();
    d <<= 8;
    d |= b;
    return d;
}

uint32_t SDStorage::readLittleIndian32(File f) {
    uint32_t d;
    uint16_t b;

    b = this->readLittleIndian16(f);
    d = this->readLittleIndian16(f);
    d <<= 16;
    d |= b;
    return d;
}
