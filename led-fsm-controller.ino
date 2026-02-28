/*****************************************
* Project: Multi-Mode LED Controller Based on FSM
* Author: Ali Sbeity
* Version: 1.0
* Copyright (c) 2026 Ali Sbeity
* License: MIT License
********************************/

#include <EEPROM.h>

#define NUM_LEDS 6
#define NUM_MODES 6

// -------------------- Pins --------------------
const byte ledPins[NUM_LEDS] = {2, 3, 4, 5, 6, 7};
const byte modeButtonPin  = 8;
const byte powerButtonPin = 9;

// -------------------- EEPROM Addresses --------------------
#define ADDR_FLAG        0
#define ADDR_MODE        1
#define ADDR_SYSTEM      2
#define ADDR_RUN_INDEX   3
#define ADDR_DIRECTION   4
#define ADDR_BIN_L       5
#define ADDR_BIN_H       6

#define EEPROM_FLAG_VALUE 0x55

// -------------------- System Variables --------------------
bool systemOn = true;
byte mode = 0;

bool blinkState = false;
int runningIndex = 0;
int direction = 1;
unsigned int binaryCounter = 0;

// -------------------- Timing --------------------
unsigned long previousMillis = 0;
unsigned long interval = 500;

// -------------------- Debounce --------------------
const unsigned long debounceDelay = 30;

bool lastModeButtonState = HIGH;
bool lastPowerButtonState = HIGH;
bool modeButtonState = HIGH;
bool powerButtonState = HIGH;

unsigned long lastModeDebounceTime = 0;
unsigned long lastPowerDebounceTime = 0;

// Edge flags
bool modePressedEvent = false;
bool powerPressedEvent = false;

// --------------------------------------------------
// EEPROM Functions

void saveState() {
  EEPROM.update(ADDR_FLAG, EEPROM_FLAG_VALUE);
  EEPROM.update(ADDR_MODE, mode);
  EEPROM.update(ADDR_SYSTEM, systemOn);
  EEPROM.update(ADDR_RUN_INDEX, runningIndex);
  EEPROM.update(ADDR_DIRECTION, direction);
  EEPROM.update(ADDR_BIN_L, lowByte(binaryCounter));
  EEPROM.update(ADDR_BIN_H, highByte(binaryCounter));
}

void loadState() {
  if (EEPROM.read(ADDR_FLAG) == EEPROM_FLAG_VALUE) {
    mode = EEPROM.read(ADDR_MODE);
    systemOn = EEPROM.read(ADDR_SYSTEM);
    runningIndex = EEPROM.read(ADDR_RUN_INDEX);
    direction = EEPROM.read(ADDR_DIRECTION);

    byte lowB = EEPROM.read(ADDR_BIN_L);
    byte highB = EEPROM.read(ADDR_BIN_H);
    binaryCounter = word(highB, lowB);

    if (mode >= NUM_MODES) mode = 0;
    if (runningIndex >= NUM_LEDS) runningIndex = 0;
    if (direction != 1 && direction != -1) direction = 1;
  }
}

void resetSystem() {
  mode = 0;
  systemOn = true;
  runningIndex = 0;
  direction = 1;
  binaryCounter = 0;
  blinkState = false;
  previousMillis = millis();
  saveState();
}

// --------------------------------------------------

void setup() {
  for (byte i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  pinMode(modeButtonPin, INPUT_PULLUP);
  pinMode(powerButtonPin, INPUT_PULLUP);

  loadState();
}

// --------------------------------------------------

void loop() {
  readButtons();

  // --- Both buttons pressed -> Reset ---
  if (modePressedEvent && powerPressedEvent) {
    resetSystem();
    modePressedEvent = false;
    powerPressedEvent = false;
  }

  // --- Mode button event ---
  else if (modePressedEvent) {
    mode++;
    if (mode >= NUM_MODES) mode = 0;
    saveState();
    modePressedEvent = false;
  }

  // --- Power button event ---
  else if (powerPressedEvent) {
    systemOn = !systemOn;
    saveState();
    powerPressedEvent = false;
  }

  if (!systemOn) {
    turnAllOff();
    return;
  }

  unsigned long currentMillis = millis();

  switch (mode) {
    case 0:
      turnAllOff();
      break;

    case 1:
      interval = 1000;
      blinkAll(currentMillis);
      break;

    case 2:
      interval = 200;
      blinkAll(currentMillis);
      break;

    case 3:
      interval = 150;
      runningLight(currentMillis);
      break;

    case 4:
      interval = 120;
      pingPong(currentMillis);
      break;

    case 5:
      interval = 250;
      binaryMode(currentMillis);
      break;
  }
}

// --------------------------------------------------
// Button Handling (Debounce + Edge Detection)

void readButtons() {
  modePressedEvent = false;
  powerPressedEvent = false;

  // --- Mode ---
  bool readingMode = digitalRead(modeButtonPin);
  if (readingMode != lastModeButtonState)
    lastModeDebounceTime = millis();

  if ((millis() - lastModeDebounceTime) > debounceDelay) {
    if (readingMode != modeButtonState) {
      modeButtonState = readingMode;
      if (modeButtonState == LOW)
        modePressedEvent = true;
    }
  }
  lastModeButtonState = readingMode;

  // --- Power ---
  bool readingPower = digitalRead(powerButtonPin);
  if (readingPower != lastPowerButtonState)
    lastPowerDebounceTime = millis();

  if ((millis() - lastPowerDebounceTime) > debounceDelay) {
    if (readingPower != powerButtonState) {
      powerButtonState = readingPower;
      if (powerButtonState == LOW)
        powerPressedEvent = true;
    }
  }
  lastPowerButtonState = readingPower;
}

// --------------------------------------------------
// LED Functions

void turnAllOff() {
  for (byte i = 0; i < NUM_LEDS; i++)
    digitalWrite(ledPins[i], LOW);
}

// --------------------------------------------------

void blinkAll(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    blinkState = !blinkState;

    for (byte i = 0; i < NUM_LEDS; i++)
      digitalWrite(ledPins[i], blinkState);
  }
}

// --------------------------------------------------

void runningLight(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    turnAllOff();
    digitalWrite(ledPins[runningIndex], HIGH);

    runningIndex++;
    if (runningIndex >= NUM_LEDS) runningIndex = 0;

    saveState();
  }
}

// --------------------------------------------------

void pingPong(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    turnAllOff();
    digitalWrite(ledPins[runningIndex], HIGH);

    runningIndex += direction;

    if (runningIndex == NUM_LEDS - 1 || runningIndex == 0)
      direction = -direction;

    saveState();
  }
}

// --------------------------------------------------

void binaryMode(unsigned long currentMillis) {
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    for (byte i = 0; i < NUM_LEDS; i++)
      digitalWrite(ledPins[i], (binaryCounter >> i) & 0x01);

    binaryCounter++;
    saveState();
  }
}

