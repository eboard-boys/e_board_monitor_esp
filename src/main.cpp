#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>  // Widget library
#include <SD.h>
#include <SoftwareSerial.h>

// UART serial (used instead of LoRa for now)
#define TX 32
#define RX 33

// ADC
#define POT 34

// Color defines
#define DARKER_GREY 0x18E0

// The display size is known
#define SIZE_Y 240
#define SIZE_X 320
#define CENTER_Y (SIZE_Y / 2)
#define CENTER_X (SIZE_X / 2)

// Communication Status UI constants
#define LORA_ICON_RADIUS 10
#define LORA_ICON_DIAMETER (2 * LORA_ICON_RADIUS)
#define LORA_ICON_X (SIZE_X - LORA_ICON_RADIUS)
#define LORA_ICON_Y LORA_ICON_RADIUS
#define LORA_ICON_LEFT (SIZE_X - LORA_ICON_DIAMETER)
#define LORA_ICON_TOP (SIZE_X - LORA_ICON_RADIUS)

// Spedometer UI constants
#define SPEDOMETER_RADIUS 90
#define METER_ARC_OUTSIDE (SPEDOMETER_RADIUS - 3)
#define METER_ARC_INSIDE METER_ARC_OUTSIDE - (METER_ARC_OUTSIDE / 5)
#define METER_ARC_START_ANGLE 30
#define METER_ARC_END_ANGLE 330

// Maximum **REPORTED** speed
#define MAX_SPEED 100

// Function declarations
void update_spedometer();
void update_lora_status();

// Values are fetched from LoRa transceiver
// Having them as globals allow them to be accessed easily across tasks
byte speed = 0;
byte battery_temp = 0;
unsigned long trip_odometer = 0;
bool lora_communicating = false;

// For accessing display methods
TFT_eSPI tft = TFT_eSPI();

// For communication over UART
SoftwareSerial controller_com(RX, TX);

// Do this at start of run
void setup() {
  // Initialize display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  // Initial drawing of spedometer
  tft.fillCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, DARKER_GREY);
  tft.drawSmoothCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, TFT_SILVER, DARKER_GREY);
  tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, METER_ARC_START_ANGLE, METER_ARC_END_ANGLE, TFT_BLACK, DARKER_GREY);

  tft.drawCentreString("Sup *DUDE*", CENTER_X, 0, 1);

  // Shared text drawing parameters
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(8);
}

// Do this task indefinitely
void loop() {
  static int skip_display_update = 1000;

  // Read and process the throttle value
  uint16_t cur_throttle = analogRead(POT);
  speed = map(cur_throttle, 0, 4096, 0, 100);
  Serial.println(cur_throttle);
  // Check for serial communication; this variable is a misnomer for now
  lora_communicating = controller_com;
  // Write throttle value between 0 and 100 over serial
  controller_com.println(cur_throttle);

  skip_display_update = skip_display_update - 1;

  if (!skip_display_update) {
    // Update displays
    update_spedometer();
    update_lora_status();
  }
  if (skip_display_update <= 0) {skip_display_update = 1000;}
}

// Update the spedometer UI display
void update_spedometer() {
  static unsigned short last_angle = METER_ARC_START_ANGLE;

  // Calculate position on meter for a given speed
  unsigned short cur_speed_angle = map(speed, 0, MAX_SPEED, METER_ARC_START_ANGLE, METER_ARC_END_ANGLE);

  // Only update the display on changes
  if (cur_speed_angle != last_angle) {
    // Hide previous number
    tft.fillCircle(CENTER_X, CENTER_Y, METER_ARC_INSIDE, DARKER_GREY);
    // Ensure the colors are correct
    tft.setTextColor(TFT_WHITE, DARKER_GREY);
    tft.drawNumber(speed, CENTER_X, CENTER_Y);
    // Draw only part of the arc based on how much the speed changed
    if (cur_speed_angle > last_angle)
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, last_angle, cur_speed_angle, TFT_GREEN, TFT_BLACK);
    else
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, cur_speed_angle, last_angle, TFT_BLACK, DARKER_GREY);
    
    last_angle = cur_speed_angle;
  }
}

// Update LoRa communication UI status
void update_lora_status() {
  static bool last_status = !lora_communicating;

  if (!(last_status && lora_communicating)) {
    // Draw over last icon
    tft.fillRect(LORA_ICON_LEFT, 0, LORA_ICON_DIAMETER + 1, LORA_ICON_DIAMETER + 1, TFT_BLACK);

    // Show communication indicators
    if (lora_communicating)
      tft.fillSmoothCircle(LORA_ICON_X, LORA_ICON_Y, LORA_ICON_RADIUS, TFT_GREEN, TFT_BLACK);
    else
      tft.fillTriangle(LORA_ICON_LEFT, LORA_ICON_DIAMETER, LORA_ICON_TOP, 0, SIZE_X, LORA_ICON_DIAMETER, TFT_RED);
    last_status = lora_communicating;
  }
}
