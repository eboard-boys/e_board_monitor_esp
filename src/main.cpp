#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>  // Widget library
#include <SD.h>

// Color defines
#define DARKER_GREY 0x18E0

// Spedometer UI constants
#define SPEDOMETER_RADIUS 90
#define METER_ARC_OUTSIDE (SPEDOMETER_RADIUS - 3)
#define METER_ARC_INSIDE METER_ARC_OUTSIDE - (METER_ARC_OUTSIDE / 5)
#define METER_ARC_START_ANGLE 30
#define METER_ARC_END_ANGLE 330

// Maximum **REPORTED** speed
#define MAX_SPEED 30

// The LCD size is known
#define CENTER_Y 120
#define CENTER_X 160

// Function declarations
void update_spedometer();

// Values are fetched from LoRa transceiver
// Having them as globals allow them to be accessed easily across tasks
byte speed = 0;
byte battery_temp = 0;
unsigned long trip_odometer = 0;
bool lora_communicating = false;

// For accessing display methods
TFT_eSPI tft = TFT_eSPI();

void setup() {
  // Initialize LCD
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  // Initial drawing of spedometer
  tft.fillCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, DARKER_GREY);
  tft.drawSmoothCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, TFT_SILVER, DARKER_GREY);
  tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, METER_ARC_START_ANGLE, METER_ARC_END_ANGLE, TFT_BLACK, DARKER_GREY);

  // Shared text drawing parameters
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(8);
}

void loop() {
  update_spedometer();
  // speed = random(31);
  speed = (speed + 1) % 31;
  // delay(random(2000));
  delay(200);
}

void update_spedometer() {
  static unsigned short last_angle = METER_ARC_START_ANGLE;

  unsigned short cur_speed_angle = map(speed, 0, MAX_SPEED, METER_ARC_START_ANGLE, METER_ARC_END_ANGLE);
  if (cur_speed_angle != last_angle) {
    tft.fillCircle(CENTER_X, CENTER_Y, METER_ARC_INSIDE, DARKER_GREY);
    tft.setTextColor(TFT_WHITE, DARKER_GREY);
    tft.drawNumber(speed, CENTER_X, CENTER_Y);
    if (cur_speed_angle > last_angle)
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, last_angle, cur_speed_angle, TFT_GREEN, TFT_BLACK);
    else
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, cur_speed_angle, last_angle, TFT_BLACK, DARKER_GREY);
    last_angle = cur_speed_angle;
  }
}
