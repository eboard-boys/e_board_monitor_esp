#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>  // Widget library
#include <SD.h>
#include <string.h>

// According to: https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
#define TX_2 17 // UART2 TX2 Pin
#define RX_2 16 // UART2 RX2 Pin

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
#define SPEDOMETER_RADIUS 80  // Was 90
#define METER_ARC_OUTSIDE (SPEDOMETER_RADIUS - 3)
#define METER_ARC_INSIDE METER_ARC_OUTSIDE - (METER_ARC_OUTSIDE / 5)
#define METER_ARC_START_ANGLE 30
#define METER_ARC_END_ANGLE 330

// Maximum **REPORTED** speed
#define MAX_SPEED 100

// Function declarations
void update_throttle_display();
void update_lora_icon();
void update_trip(float distance);
void display_speed();

// Values are fetched from LoRa transceiver
// Having them as globals allow them to be accessed easily across tasks
byte speed = 0;
byte throttle = 0;
byte battery_temp = 0;
unsigned long trip_odometer = 0;
bool lora_communicating = false;

// For accessing display methods
TFT_eSPI tft = TFT_eSPI();

// Do this at start of run
void setup() {
  
  // Initialize Serial2

  // Set the network ID 3 of Reyax LoRa
  Serial.println("AT+NETWORKID=3");

  // Set the address of Reyax LoRa
  Serial2.println("AT+ADDRESS=25");

  // Send AT command to verify things are working
  Serial2.println("AT");


  // Initialize display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  // Initial drawing of spedometer
  tft.fillCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, DARKER_GREY);
  tft.drawSmoothCircle(CENTER_X, CENTER_Y, SPEDOMETER_RADIUS, TFT_SILVER, DARKER_GREY);
  tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, METER_ARC_START_ANGLE, 
    METER_ARC_END_ANGLE, TFT_BLACK, DARKER_GREY);

  tft.drawCentreString("Trip: ", CENTER_X-15, 230, 1);

  // Shared text drawing parameters
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(8);
}

// Do this task indefinitely
void loop() {
  static int skip_display_update = 1000;

  // Read and process the throttle value
  uint16_t cur_throttle = analogRead(POT);
  speed = map(cur_throttle, 0, 4096, 0, 100); // Map throttle value between [0, 100]

  char data[25] = "";
  sprintf(data, "AT+SEND, 25, 4, %i", cur_throttle);  // Convert the throttle into a char array to be sent
  Serial2.println(data);                              // Send throttle value via AT+ command

  // Decrement the skip timer
  skip_display_update -= 1;

  // Check for serial communication
  if (Serial2.available()) lora_communicating = true;
  else lora_communicating = false;

  // Is this to give an artificial timer for the screen update?
  if (!skip_display_update) {
    // Update displays
    update_throttle_display();
    update_lora_icon();
    update_trip(0.01);  // Temporarily takes in a constant 0.01 miles
    display_speed();
  }
  if (skip_display_update <= 0) skip_display_update = 1000;

}

// Update the spedometer UI display
void update_throttle_display() {
  static unsigned short last_angle = METER_ARC_START_ANGLE;

  // Calculate position on meter for a given speed
  unsigned short cur_throttle_angle = map(throttle, 0, MAX_SPEED, METER_ARC_START_ANGLE, 
    METER_ARC_END_ANGLE);

  // Only update the display on changes
  if (cur_throttle_angle != last_angle) {
    // Hide previous number
    tft.fillCircle(CENTER_X, CENTER_Y, METER_ARC_INSIDE, DARKER_GREY);
    // Ensure the colors are correct
    tft.setTextColor(TFT_WHITE, DARKER_GREY);
    tft.drawNumber(throttle, CENTER_X, CENTER_Y);
    // Draw only part of the arc based on how much the speed changed
    if (cur_throttle_angle > last_angle)
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, last_angle, 
        cur_throttle_angle, TFT_GREEN, TFT_BLACK);
    else
      tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, cur_throttle_angle, 
        last_angle, TFT_BLACK, DARKER_GREY);
    
    last_angle = cur_throttle_angle;
  }
}

// Read the received speed value and display on the LCD
void display_speed()
{
  // Check if LoRa recieved speed
  // Take the message and save only "S<number>"

  // Convert the number to a string

  // Display the number_text on the LCD right below throttle
  tft.drawString("speed!", CENTER_X, 180);
}

// Keep track of total trip distance and display on the LCD
void update_trip(float distance)
{

  // compound distance to the odometer
  trip_odometer += distance;

  // convert odometer value to string
  String text_odometer = String(trip_odometer, 3);

  // Display the odometer
  tft.drawCentreString(text_odometer, CENTER_X+5, 230, 1);
}

// Update LoRa communication UI status
void update_lora_icon() {
  
  static bool last_status = !lora_communicating;

  // if one or both are not HIGH
  if (!(last_status && lora_communicating)) {
    // Draw over last icon
    tft.fillRect(LORA_ICON_LEFT, 0, LORA_ICON_DIAMETER + 1, LORA_ICON_DIAMETER + 1, TFT_BLACK);

    // Show communication indicators
    if (lora_communicating)
    {
      // Draw comm good icon
      tft.fillSmoothCircle(LORA_ICON_X, LORA_ICON_Y, LORA_ICON_RADIUS, TFT_GREEN, TFT_BLACK);
    }
    else
    {
      // Draw comm failure icon
      tft.fillTriangle(LORA_ICON_LEFT, LORA_ICON_DIAMETER, LORA_ICON_TOP, 0, SIZE_X, 
      LORA_ICON_DIAMETER, TFT_RED);
    }
      
    last_status = lora_communicating;
  }
}
