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

DigitalFrame::DigitalFrame(ILI9486 *display, XPT2046_Touchscreen *touch, Calibration *calibration, SDStorage *storage):
	currentState(IMAGE_DISPLAY),
	currentDispMode(RANDOM),
	display(display),
	touch(touch),
	calibration(calibration),
	storage(storage),
	lastImageDisTime(0),
	lastTouchTime(0),
	brightnessLevel(BRIGHTNESS_LEVELS - 1),
	dispTimeLevel(DEFAULT_DISP_TIME_LEVEL),
	forceImageDisplay(true),
	imageRandDisplayed({}),
	randDisplayedN(0)
{
	// Check if sd card initialized correctly
	if (!storage->isOk()) { 
		this->changeState(SD_ERROR);
	}

	// Display intro image
	storage->toImage(INTRO_BMP);
	this->loadImage();
	display->setBacklight(255);
	this->lastImageDisTime = millis();


	// Load settings while displaying image
	this->loadSettings();
	this->countImages();

	// Pin A0 is unconnected
	// Electric noise will cause to generate different seed values
	randomSeed(analogRead(A0));

	display->changeDefaultBacklight(brightnessLevels[brightnessLevel]);
	display->setDefaultBacklight();

	// Wait to the end of intro display time
	delay(INTRO_DISPLAY_TIME - (millis() - this->lastImageDisTime) );
}

void DigitalFrame::loop() {
	// Check if touch occurred
	this->checkTouch();

	// Only check touch if not loading new images
	if (this->currentState != IMAGE_DISPLAY) { 
		return; 
	}

	// Display new image only if display time for old image passed
	if ( (!this->forceImageDisplay) && (millis() - this->lastImageDisTime < dispTimeLevels[this->dispTimeLevel]) ) {
		return;   
	}

	// Do not change image in ONLY_CURRENT mode (unless force display)
	if ( (!this->forceImageDisplay) && (this->currentDispMode == ONLY_CURRENT)) {
		return;
	}

	this->forceImageDisplay = false;

	// Choose new image based on current state
	switch(this->currentDispMode) {
		case IN_ORDER:
			storage->nextImage();
			break;

		case RANDOM: {
			// Pick random image from those not displayed recently
			uint16_t imageN = random(this->imageNumberInDir - this->randDisplayedN);
			// Find image not displayed recently
			uint16_t notDisp = imageN;
			uint16_t index = 0;
			for (uint16_t i = 0; i < imageN + 1; i++) {
				if (this->imageRandDisplayed[index++]) { 
					notDisp++;
					i--;
				}
			}

			// Mark image as recently displayed
			this->imageRandDisplayed[notDisp] = true;
			this->randDisplayedN++;

			// If all recently displayed, reset 
			if (this->randDisplayedN >= this->imageNumberInDir) {
				this->randDisplayedN = 0;
				for (uint32_t i = 0; i < MAX_IMG_N; i++) { this->imageRandDisplayed[i] = false; }
			}

			storage->toImage(notDisp);
			break;
		}
 
		case ONLY_CURRENT:
			storage->toImage( storage->getImageNumber() );
			break;
	}
			
	display->openWindow(0, 0, display->getWidth(), display->getHeight());

	// Load image by portions and check for touch in the meantime
	for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
		this->loadImagePortion();
		if (checkTouch()) { break; }
	}

	//  If image fully loaded
	if (this->currentState == IMAGE_DISPLAY) {
		this->lastImageDisTime = millis();
	}
}

void DigitalFrame::loadImage() {
	uint16_t buffer[IMG_BUFFER];

	// Load image into display
	display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
		bool ok = storage->readImagePortion(buffer, IMG_BUFFER);
		
		// If error while reading from sd
		if (!ok) {
			this->changeState(SD_ERROR);
		}
		
		display->writeBuffer(buffer, IMG_BUFFER);
	}
}

void DigitalFrame::loadImagePortion() {
	uint16_t buffer[IMG_BUFFER];

	// Load only one portion
	bool ok = storage->readImagePortion(buffer, IMG_BUFFER);

	// If error while reading from sd
	if (!ok) {
		this->changeState(SD_ERROR);
	}

	display->writeBuffer(buffer, IMG_BUFFER);
}

void DigitalFrame::countImages() {
	this->imageNumberInDir = 0;

	// Count images until same image occurred after directory rewind
	String name = storage->getCurrentImage().name();
	do {
		imageNumberInDir++;
		storage->nextImage();
	} while (name != storage->getCurrentImage().name());
}

bool DigitalFrame::checkTouch() {
	// To prevent multi-touches 
	if (millis() - lastTouchTime < TOUCH_DELAY) { return false; }
	
	bool touched = (touch->tirqTouched() && touch->touched());

	if (!touched) { return false; }

	// If touch occurred
	uint16_t x, y;
	this->getTouch(x, y);
	lastTouchTime = millis();

	// Handle touch based on current state
	switch(this->currentState) {
		case IMAGE_DISPLAY:
			this->changeState(MENU_DISPLAY);
			break;
		case MENU_DISPLAY:
			this->handleMenuTouch(x, y);
			break;
		case SET_BRIGHTNESS:
			this->handleSetBrightnessTouch(x, y);
			break;
		case SET_DISP_TIME:
			this->handleSetDispTimeTouch(x, y);
			break;
		case SET_DISP_MODE:
			this->handleSetDispModeTouch(x, y);
			break;
	}

	return true;
}

void DigitalFrame::getTouch(uint16_t &x, uint16_t &y) {
	TS_Point p = touch->getPoint();
	calibration->translate(p);

	x = p.x;
	y = p.y;
}

void DigitalFrame::changeState(State newState) {
	// If current state need to save setting to sd
	if ( (this->currentState == SET_BRIGHTNESS) || (this->currentState == SET_DISP_TIME) || (this->currentState == SET_DISP_MODE) ) {
		this->saveSettings();
	}
	
	display->clear();
	this->currentState = newState;

	switch(newState) {
		case IMAGE_DISPLAY:
			this->forceImageDisplay = true;
			break;
		case MENU_DISPLAY:
			storage->toImage(MENU_BMP);
			this->loadImage();
			break;
		case SET_BRIGHTNESS:
			storage->toImage(BRIGHTNESS_BMP);
			this->loadImage();
			this->displayLevel(this->brightnessLevel, BRIGHTNESS_LEVELS);
			break;
		case SET_DISP_TIME:
			storage->toImage(DISP_TIME_BMP);
			this->loadImage();
			this->displayTime(dispTimeLevels[this->dispTimeLevel]);
			break;
		case SET_DISP_MODE:
			storage->toImage(DISP_MODE_BMP);
			this->loadImage();
			this->displaySelected((uint8_t)this->currentDispMode);
			break;
		case SD_ERROR:
			display->setBacklight(255);
			this->displayStorageError();
			while(1){} // Suspend on sd error
			break;
	}
}

void DigitalFrame::handleMenuTouch(uint16_t x, uint16_t y) {
	// Touch on set brightness option
	if (y > 360) {
		this->changeState(SET_BRIGHTNESS);
	}

	// Touch on set display time option
	else if (y > 240) {
		this->changeState(SET_DISP_TIME);
	}

	// Touch on set display mode option
	else if (y > 120) {
		this->changeState(SET_DISP_MODE);
	}

	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}
}

void DigitalFrame::displayLevel(uint8_t level, uint8_t max) {
	uint16_t x = 50;
	for (uint8_t i = 0; i < max - 1; i++) {
		ILI9486_COLOR c = (i < level) ? ILI9486_WHITE : ILI9486_BLACK;
		display->fill(x, 270, x + 15, 330, c);
		x += 20;
	}
}

void DigitalFrame::displaySelected(uint8_t selected) {
	uint16_t y = 420;
	for (uint8_t i = 0; i < 3; i++) {
		ILI9486_COLOR c = (i == selected) ? ILI9486_WHITE : ILI9486_BLACK;
		display->drawCircle(305, y, 10, c, true);
		y -= 120;
	}
}

void DigitalFrame::displayTime(uint32_t time) {
	display->fill(0, 270, display->getWidth(), 330, ILI9486_BLACK);
	
	char text[128];
	bool seconds = true;

	if (time >= 60000) {
		time /= 60000;
		seconds = false;
	} else {
		time /= 1000;
	}
	
	uint16_t digitCount = 0;

	uint32_t tmp = time;
	while (tmp) {
		digitCount++;
		tmp /= 10;
	}

	uint16_t i = digitCount;
	while(time) {
		text[i-1] = (time % 10) + '0';
		time /= 10;
		i--;
	}

	text[digitCount] = ' ';

	if (seconds) {
		text[digitCount+1] = 's';
		text[digitCount+2] = 'e';
		text[digitCount+3] = 'c';
		text[digitCount+4] = 'o';
		text[digitCount+5] = 'n';
		text[digitCount+6] = 'd';
	} else {
		text[digitCount+1] = 'm';
		text[digitCount+2] = 'i';
		text[digitCount+3] = 'n';
		text[digitCount+4] = 'u';
		text[digitCount+5] = 't';
		text[digitCount+6] = 'e';
	}

	text[digitCount+7] = (text[0] != '1' || digitCount != 1) ? 's' : ' ';
	text[digitCount+8] = '\0';

	display->drawString(30, 300, (uint8_t*)text,ILI9486::L, ILI9486_WHITE );
}

void DigitalFrame::displayStorageError() {
	display->clear();
	display->drawLine(80, 120, display->getWidth()-80, display->getHeight()-120, ILI9486_RED);
	display->drawLine(80, display->getHeight()-120, display->getWidth()-80, 120, ILI9486_RED);
	display->drawString(70, 400, "SD card error", ILI9486::L, ILI9486_RED);
}

void DigitalFrame::handleSetBrightnessTouch(uint16_t x, uint16_t y) {
	// Touch on brightness up
	if (y > 360) {
		if (this->brightnessLevel == BRIGHTNESS_LEVELS - 1) { return; }
		this->brightnessLevel++;
		display->changeDefaultBacklight(brightnessLevels[this->brightnessLevel]);
		display->setDefaultBacklight();
	}

	// Touch on current brightness level
	else if (y > 240) {}
	
	// Touch on brightness down
	else if (y > 120) {
		if (this->brightnessLevel == 0) { return; }
		this->brightnessLevel--;
		display->changeDefaultBacklight(brightnessLevels[this->brightnessLevel]);
		display->setDefaultBacklight();
	}
	
	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If brightness level changed
	if ( (y > 360) || ( (y < 240) && (y > 120) ) ) {
		this->displayLevel(this->brightnessLevel, BRIGHTNESS_LEVELS);
	}
}

void DigitalFrame::handleSetDispTimeTouch(uint16_t x, uint16_t y) {
	// Touch on longer
	if (y > 360) {
		if (this->dispTimeLevel >= DISP_TIME_LEVELS - 1) { return; }
		this->dispTimeLevel++;
	}

	// Touch on current display time
	else if (y > 240) {}
	
	// Touch on shorter
	else if (y > 120) {
		if (this->dispTimeLevel <= 0) { return; }
		this->dispTimeLevel--;
	}
	
	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If disp time changed
	if ( (y > 360) || ( (y < 240) && (y > 120) ) ) {
		this->displayTime(dispTimeLevels[this->dispTimeLevel]);
	}
}

void DigitalFrame::handleSetDispModeTouch(uint16_t x, uint16_t y) {
	// Touch on random
	if (y > 360) {
		this->currentDispMode = RANDOM;
	}

	// Touch on in order 
	else if (y > 240) {
		this->currentDispMode = IN_ORDER;
	}

	// Touch on only current image
	else if (y > 120) {
		this->currentDispMode = ONLY_CURRENT;
	}

	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If mode changed
	if (y > 120) {
		this->displaySelected((uint8_t)this->currentDispMode);
	}
}

void DigitalFrame::saveSettings() {
	uint8_t s[5] = {
		this->brightnessLevel,
		this->dispTimeLevel,
		(uint8_t)this->currentDispMode,
		storage->getImageNumber() >> 8, // Image number is stored in 16 bit variable 
		storage->getImageNumber() & 0xFF
	};

	storage->saveSettings(s, 5);
}

void DigitalFrame::loadSettings() {
	uint8_t s[5];
	storage->loadSettings(s, 5);

	this->brightnessLevel = s[0];
	this->dispTimeLevel = s[1];
	this->currentDispMode = (DispMode)s[2];

	// Only ONLY_CURRENT mode uses image number
	if (currentDispMode == ONLY_CURRENT) {
		uint16_t imageN = ((uint16_t)s[3] << 8) | (uint16_t)s[4];
		storage->toImage(imageN);
	}
}
