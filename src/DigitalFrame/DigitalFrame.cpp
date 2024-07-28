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
	display(display),
	touch(touch),
	calibration(calibration),
	storage(storage),
	state(IMAGE_DISPLAY),
	dispMode(RANDOM),
	randDisplayedN(0),
	lastImageDisTime(0),
	lastTouchTime(0),
	turnOffTime(0),
	brightnessLvl(BRIGHTNESS_LEVELS_N - 1),
	dispTimeLvl(DEFAULT_DISP_TIME_LEVEL),
	turnOffTimeLvl(0),
	turnOffScheduled(false),
	forceImageDisplay(true),
	imageRandDisplayed({})
{
	// Check if sd card initialized correctly
	if (storage->error()) { 
		this->changeState(SD_ERROR);
		return;
	}

	// Load settings while displaying image
	this->loadSettings();
	this->countImages();

	// Pin A0 is unconnected
	// Electric noise will cause to generate different seed values
	randomSeed(analogRead(A0));

	storage->toImage(INTRO_BMP);
	this->loadImage();
	display->changeDefaultBacklight(brightnessLvls[brightnessLvl]);
	display->setDefaultBacklight();

	// Wait to the end of intro display time
	delay(INTRO_DISPLAY_TIME);
}

void DigitalFrame::loop() {
	// Check if touch occurred
	if (this->touched()) {
		this->handleTouch();
	}

	// Check of sd errors
	if ( (storage->error()) && (this->state != SD_ERROR) ) {
		this->changeState(SD_ERROR);
	}

	// Check for turn off time if scheduled
	if ( (this->turnOffScheduled) && (millis() >= this->turnOffTime) ) {
		this->changeState(SLEEP);
	}

	if (this->state == SLEEP) {
		delay(50);
	}

	// Only check touch if not loading new images
	if (this->state != IMAGE_DISPLAY) { 
		return; 
	}

	// Display new image only if display time for old image passed
	if ( (!this->forceImageDisplay) && (millis() - this->lastImageDisTime < dispTimeLvls[this->dispTimeLvl]) ) {
		return;   
	}

	// Do not change image in ONLY_CURRENT mode (unless force display)
	if ( (!this->forceImageDisplay) && (this->dispMode == ONLY_CURRENT)) {
		return;
	}

	this->forceImageDisplay = false;
	this->moveToNextImg();
}

void DigitalFrame::moveToNextImg() {
	// Choose new image based on current state
	switch(this->dispMode) {
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
		if (this->touched()) { 
			this->handleTouch();
			break; 
		}
	}

	//  If image fully loaded
	if (this->state == IMAGE_DISPLAY) {
		this->lastImageDisTime = millis();
	}
}

void DigitalFrame::loadImage() {
	uint16_t buffer[IMG_BUFFER];

	// Load image into display
	display->openWindow(0, 0, display->getWidth(), display->getHeight());
	for (uint32_t i = 0; i < display->getSize() / IMG_BUFFER; i++) {
		storage->readImagePortion(buffer, IMG_BUFFER);
		display->writeBuffer(buffer, IMG_BUFFER);
	}
}

void DigitalFrame::loadImagePortion() {
	uint16_t buffer[IMG_BUFFER];

	// Load only one portion
	storage->readImagePortion(buffer, IMG_BUFFER);
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

bool DigitalFrame::touched() {
	// To prevent multi-touches 
	if (millis() - lastTouchTime < TOUCH_DELAY) { return false; }
	
	return (touch->tirqTouched() && touch->touched());
}

void DigitalFrame::handleTouch() {
	uint16_t x, y;
	this->getTouchPos(x, y);
	lastTouchTime = millis();

	// Handle touch based on current state
	switch(this->state) {
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

		case SET_TURN_OFF:
			this->handleSetTurnOffTimeTouch(x, y);
			break;
		
		case SLEEP:
			this->changeState(IMAGE_DISPLAY);
			break;

		case SD_ERROR:
			display->turnOffBacklight();
			this->reset();
			break;
	}
}

void DigitalFrame::getTouchPos(uint16_t &x, uint16_t &y) {
	TS_Point p = touch->getPoint();
	calibration->translate(p);

	x = p.x;
	y = p.y;
}

void DigitalFrame::changeState(State newState) {
	// If current state need to save setting to sd
	if ( (this->state == SET_BRIGHTNESS) || (this->state == SET_DISP_TIME) || (this->state == SET_DISP_MODE) ) {
		this->saveSettings();
	}

	// Prepare screen for state change
	if (state == SLEEP ) {
		for (uint8_t i = 0 ; i < display->getDefaultBacklight(); i++) {
			display->setBacklight(i);
			delay(10);
		}
	}

	if ( (newState != SLEEP) && (state != SLEEP) ) {
		display->clear();
	}


	this->state = newState;

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
			this->dispLevel(this->brightnessLvl, BRIGHTNESS_LEVELS_N);
			break;

		case SET_DISP_TIME:
			storage->toImage(DISP_TIME_BMP);
			this->loadImage();
			this->dispTime(dispTimeLvls[this->dispTimeLvl]);
			break;

		case SET_DISP_MODE:
			storage->toImage(DISP_MODE_BMP);
			this->loadImage();
			this->dispSelected((uint8_t)this->dispMode);
			break;

		case SET_TURN_OFF:
			storage->toImage(SET_TURN_OFF_BMP);
			this->loadImage();
			this->turnOffTimeLvl = 0;
			this->dispTime(turnOffTimes[this->turnOffTimeLvl]);
			break;

		case SLEEP:
			this->turnOffScheduled = false;
			// Dim screen and turn off backlight
			for (int i  = display->getDefaultBacklight(); i > -1; i--) {
				display->setBacklight(i);
				delay(10);
			}
			break;

		case SD_ERROR:
			display->setBacklight(255);
			this->dispStorageError();
			break;
	}
}

void DigitalFrame::handleMenuTouch(uint16_t x, uint16_t y) {
	// Touch on set brightness option
	if (y > 384) {
		this->changeState(SET_BRIGHTNESS);
	}

	// Touch on set display time option
	else if (y > 288) {
		this->changeState(SET_DISP_TIME);
	}

	// Touch on set display mode option
	else if (y > 192) {
		this->changeState(SET_DISP_MODE);
	}

	// Touch on schedule turn off option
	else if (y > 96) {
		this->changeState(SET_TURN_OFF);
	}

	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}
}

void DigitalFrame::dispLevel(uint8_t level, uint8_t max) {
	uint16_t x = 50;
	for (uint8_t i = 0; i < max - 1; i++) {
		ILI9486_COLOR c = (i < level) ? ILI9486_WHITE : ILI9486_BLACK;
		display->fill(x, 270, x + 15, 330, c);
		x += 20;
	}
}

void DigitalFrame::dispSelected(uint8_t selected) {
	uint16_t y = 420;
	for (uint8_t i = 0; i < 3; i++) {
		ILI9486_COLOR c = (i == selected) ? ILI9486_WHITE : ILI9486_BLACK;
		display->drawCircle(305, y, 10, c, true);
		y -= 120;
	}
}

void DigitalFrame::dispTime(uint32_t time) {
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

void DigitalFrame::dispStorageError() {
	display->clear();
	display->drawLine(80, 120, display->getWidth()-80, display->getHeight()-120, ILI9486_RED);
	display->drawLine(80, display->getHeight()-120, display->getWidth()-80, 120, ILI9486_RED);
	display->drawString(70, 400, "SD card error", ILI9486::L, ILI9486_RED);
	display->drawString(70, 80, "Tap to reboot", ILI9486::L, ILI9486_RED);
}

void DigitalFrame::handleSetBrightnessTouch(uint16_t x, uint16_t y) {
	// Touch on brightness up
	if (y > 360) {
		if (this->brightnessLvl == BRIGHTNESS_LEVELS_N - 1) { return; }
		this->brightnessLvl++;
		display->changeDefaultBacklight(brightnessLvls[this->brightnessLvl]);
		display->setDefaultBacklight();
	}

	// Touch on current brightness level
	else if (y > 240) {}
	
	// Touch on brightness down
	else if (y > 120) {
		if (this->brightnessLvl == 0) { return; }
		this->brightnessLvl--;
		display->changeDefaultBacklight(brightnessLvls[this->brightnessLvl]);
		display->setDefaultBacklight();
	}
	
	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If brightness level changed
	if ( (y > 360) || ( (y < 240) && (y > 120) ) ) {
		this->dispLevel(this->brightnessLvl, BRIGHTNESS_LEVELS_N);
	}
}

void DigitalFrame::handleSetDispTimeTouch(uint16_t x, uint16_t y) {
	// Touch on longer
	if (y > 360) {
		if (this->dispTimeLvl >= DISP_TIME_LEVEL_N - 1) { return; }
		this->dispTimeLvl++;
	}

	// Touch on current display time
	else if (y > 240) {}
	
	// Touch on shorter
	else if (y > 120) {
		if (this->dispTimeLvl <= 0) { return; }
		this->dispTimeLvl--;
	}
	
	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If disp time changed
	if ( (y > 360) || ( (y < 240) && (y > 120) ) ) {
		this->dispTime(dispTimeLvls[this->dispTimeLvl]);
	}
}

void DigitalFrame::handleSetDispModeTouch(uint16_t x, uint16_t y) {
	// Touch on random
	if (y > 360) {
		this->dispMode = RANDOM;
	}

	// Touch on in order 
	else if (y > 240) {
		this->dispMode = IN_ORDER;
	}

	// Touch on only current image
	else if (y > 120) {
		this->dispMode = ONLY_CURRENT;
	}

	// Touch on go back option
	else {
		this->changeState(IMAGE_DISPLAY);
	}

	// If mode changed
	if (y > 120) {
		this->dispSelected((uint8_t)this->dispMode);
	}
}

void DigitalFrame::handleSetTurnOffTimeTouch(uint16_t x, uint16_t y) {
	// Touch on later option
	if (y > 360) {
		this->turnOffTimeLvl++;
		this->turnOffTimeLvl %= TURN_OFF_TIMES_N;
	}

	// Touch on displayed time
	else if (y > 240) {}

	// Touch on earlier option
	else if (y > 120) {
		this->turnOffTimeLvl--;
		this->turnOffTimeLvl %= TURN_OFF_TIMES_N;
	}

	// Touch on go back option
	else if (x < 160) {
		this->changeState(IMAGE_DISPLAY);
	}

	// Touch on schedule option
	else if (x >= 160) {
		this->turnOffScheduled = true;
		this->turnOffTime = millis() + turnOffTimes[this->turnOffTimeLvl];
		this->changeState(IMAGE_DISPLAY);
	}

	// If mode changed
	if (y > 120) {
		this->dispTime(turnOffTimes[this->turnOffTimeLvl]);
	}
}

void DigitalFrame::saveSettings() {
	uint8_t s[5] = {
		this->brightnessLvl,
		this->dispTimeLvl,
		(uint8_t)this->dispMode,
		(uint8_t)(storage->getImageNumber() >> 8), // Image number is stored in 16 bit variable 
		(uint8_t)(storage->getImageNumber() & 0xFF)
	};

	storage->saveSettings(s, 5);
}

void DigitalFrame::loadSettings() {
	uint8_t s[5];
	storage->loadSettings(s, 5);

	this->brightnessLvl = s[0];
	this->dispTimeLvl = s[1];
	this->dispMode = (DispMode)s[2];

	// Only ONLY_CURRENT mode uses image number
	if (dispMode == ONLY_CURRENT) {
		uint16_t imageN = ((uint16_t)s[3] << 8) | (uint16_t)s[4];
		storage->toImage(imageN);
	}
}
