#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>  // Widget library
#include <SD.h>

#define DARKER_GREY 0x18E0

#define SPEDOMETER_RADIUS 90
#define METER_ARC_OUTSIDE (SPEDOMETER_RADIUS - 3)
#define METER_ARC_INSIDE METER_ARC_OUTSIDE - (METER_ARC_OUTSIDE / 5)
#define METER_ARC_START_ANGLE 30
#define METER_ARC_END_ANGLE 330

#define MAX_SPEED 30

#define CENTER_Y 120
#define CENTER_X 160

void update_spedometer();

byte speed = 0;
byte battery_temp = 0;
unsigned long trip_odometer = 0;
bool lora_communicating = false;

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  // Initial drawing of spedometer
  tft.fillCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, DARKER_GREY);
  tft.drawSmoothCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, TFT_SILVER, DARKER_GREY);
  tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, METER_ARC_START_ANGLE, METER_ARC_END_ANGLE, TFT_BLACK, DARKER_GREY);
  tft.setTextColor(TFT_WHITE, DARKER_GREY);
  // tft.setTextSize(0.6);
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
    tft.drawNumber(speed, CENTER_X, CENTER_Y);
    if (cur_speed_angle > last_angle)
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, last_angle, cur_speed_angle, TFT_GREEN, TFT_BLACK);
    else
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, cur_speed_angle, last_angle, TFT_BLACK, DARKER_GREY);
    last_angle = cur_speed_angle;
  }
}
