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
    err(false),
    imageNumber(UINT16_MAX),
    disWidth(disWidth),
    disHeight(disHeight)
{
    // Initialize SD card
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, 1);
    err = !SD.begin(SD_CS_PIN);

    // Do not open directories if not initialized (SD card not inserted?)
    if (err) {return; }

    this->imageDir = SD.open(imageDir);
    this->nextImage();

    this->countImages();
}

File SDStorage::getCurrentImage() {
    return this->currentImage;
}

uint16_t SDStorage::getImageNumber() {
    return this->imageNumber;
}

uint32_t SDStorage::imagesInDir() {
    return this->imagesInDirN;
}

void SDStorage::countImages() {
	this->imagesInDirN = 0;

	// Count images until same image occurred after directory rewind
	String name = this->getCurrentImage().name();
	do {
		imagesInDirN++;
		this->nextImage();
	} while (name != this->getCurrentImage().name());
}

bool SDStorage::error() {
    return this->err;
}

uint16_t SDStorage::nextImage() {
    uint16_t skipped = 0;

    while (true) {
        this->currentImage.close();
        this->currentImage = this->imageDir.openNextFile();

        // Rewind directory if needed
        if (!this->currentImage) {
            this->imageDir.rewindDirectory();
            this->currentImage.close();
            this->currentImage = this->imageDir.openNextFile();
            this->imageNumber = 0;
        }

        if (this->currentImage == NULL) {
            this->err = true;
            return;
        }

        if (this->validateImage(this->currentImage)) { break; }
        skipped++;
    }

    this->imageNumber++;
    return skipped;
}

bool SDStorage::toImage(String image) {
    this->currentImage.close();
    this->currentImage = SD.open(image);
    
    if (this->currentImage == NULL) {
        this->err = true;
    }
    
    return this->validateImage(this->currentImage);
}

bool SDStorage::toImage(uint16_t imagePos) {
    this->imageNumber = imagePos;

    String name = this->imageDir.name();
    name += String('/') + String(imagePos) + ".bmp";
    return this->toImage(name);
}

uint16_t SDStorage::RGB24ToRGB16(uint8_t r, uint8_t g, uint8_t b) {
    return (( (r) >> 3 ) << 11 ) | (( (g) >> 2 ) << 5) | ( (b) >> 3);
}

void SDStorage::readImagePortion(uint16_t *buffer, uint16_t size) {
    uint8_t pixels[size*3];
    int err = this->currentImage.read(pixels, size*3);

    if (err == -1) {
        this->err = true;
        return;
    }
    
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

void SDStorage::saveSettings(uint8_t *settings, uint16_t nBytes) {
    File file = SD.open(SETTINGS_FILE, O_READ | O_WRITE | O_CREAT);
    file.seek(0);

    for (uint16_t i = 0; i < nBytes; i++) {
        file.write(settings[i]);
    }

    file.close();
}

void SDStorage::loadSettings(uint8_t *settings, uint16_t nBytes) {
    File file = SD.open(SETTINGS_FILE);

    file.read(settings, nBytes);

    file.close();
}
