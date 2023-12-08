#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>  // Widget library
#include <SD.h>
#include <string.h>
// #include <iostream>

// According to: https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
#define TX_2 17        // UART2 TX2 Pin
#define RX_2 16        // UART2 RX2 Pin
#define STM_Addr 24    // STM LoRa chip address (the longboard)
#define ESP_Addr 25    // ESP LoRa chip address (this device)
#define POT 34         // ADC
#define LoRa_Delay 250 // Delay ms to wait for smooth LoRa comms

// Color defines
#define DARKER_GREY 0x18E0

// Display orientation
#define ORIENTATION_LEFT_HAND 1
#define ORIENTATION_RIGHT_HAND 3

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

// Speedometer UI constants
#define SPEEDOMETER_RADIUS 80  // Was 90
#define METER_ARC_OUTSIDE (SPEEDOMETER_RADIUS - 3)
#define METER_ARC_INSIDE METER_ARC_OUTSIDE - (METER_ARC_OUTSIDE / 5)
#define METER_ARC_START_ANGLE 30
#define METER_ARC_END_ANGLE 330

// Raw throttle ranges
#define MIN_RAW_THROTTLE 1590
#define MAX_RAW_THROTTLE 2160

// Maximum mapped throttle
#define UI_FULL_THROTTLE 80 // Maximum **REPORTED** throttle for UI
#define POT_FULL_THROTTLE 80 // Maxium value to send over LoRa

// Function declarations
void update_throttle_display();
void update_lora_icon();
void display_speed();
void display_trip();

// Values are fetched from LoRa transceiver
// Having them as globals allow them to be accessed easily across tasks
byte speed = 0;
byte throttle = 0;
byte battery_temp = 0;
char rawMsg[5];
unsigned long trip_odometer = 0;
bool lora_communicating = false;
String recv_buffer = "";
char recv_data[10];
char trip_read[10];
  
// For accessing display methods
TFT_eSPI tft = TFT_eSPI();

// See "How to Multitask with FreeRTOS - Simply Explained"
// https://www.youtube.com/watch?v=WQGAs9MwXno
// Here is a basic task creation
void task_UpdateUI(void * parameters)
{
  for (;;){
    // Stuff here that will run indefinitely
    // Update displays
    update_throttle_display();
    update_lora_icon();

    // If moving update odometer
    if (speed >= 0.5)
    {
      display_trip();  // Temporarily takes in a constant 0.01 miles
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void task_sendLora(void * parameters)
{
  for (;;)
  {
    // Create char message and send
    char msg[100] = "";
	  sprintf(msg, "AT+SEND=%i,%i,%s", STM_Addr, strlen(rawMsg), rawMsg);
	  Serial2.println(msg);
    Serial.println(msg); // Print back to USB
    vTaskDelay(LoRa_Delay / portTICK_PERIOD_MS); // Delay for other tasks to run
  }
}

void task_readLora(void * parameters)
{
  for (;;)
  {

    if (Serial2.available())
    {
      lora_communicating = true;
      
      recv_buffer = Serial2.readString();     // Save received message
      Serial.println(recv_buffer);            // Print received data to USB

      sprintf(recv_data, "%i", recv_buffer);  // Save string recv_buffer to char array

      Serial.println(recv_data[0]); // Print first character of received data

      // If first char of recv_data == S update speed
      if (recv_data[0] == 'S')
      {
        Serial.println("Speed received");
        speed = atoi(recv_data);
      }
      
      // If first char of recv_data == T update trip
      else if (recv_data[0] == 'D')
      {
        Serial.println("Trip received");
        sprintf(trip_read, "Trip: %i m", recv_buffer);  // Create text containing trip distance
      }
      else Serial.println("unknown received");
      
    }
    else
    {
      lora_communicating = false;
      Serial.println("Nothing received!");
    }
    vTaskDelay(LoRa_Delay / portTICK_PERIOD_MS); // Delay for other tasks to run
  }
  
}

// Do this at start of run
void setup() {
  
  // Initialize Serial2
  Serial2.begin(115200, SERIAL_8N1, RX_2, TX_2);

  // Initialize Serial (This one reads back to USB)
  Serial.begin(115200);

  delay(LoRa_Delay);
  // Set parameters and check parameters
  Serial2.println("AT+IPR=115200");
  delay(LoRa_Delay);
  Serial2.println("AT+PARAMETER=10,8,1,4");
  delay(LoRa_Delay);
  Serial2.println("AT+ADDRESS=25");
  delay(LoRa_Delay);
  Serial2.println("AT+NETWORKID=3");
  delay(LoRa_Delay);
  Serial2.println("AT+CPIN?");
  delay(LoRa_Delay);
  Serial2.println("AT+CRFOP?");
  delay(LoRa_Delay);
  Serial2.println("AT+ADDRESS?");
  delay(LoRa_Delay);
  Serial2.println("AT+NETWORKID?");
  delay(LoRa_Delay);
  Serial2.println("AT+BAND?");
  delay(LoRa_Delay);
  Serial2.println("AT+MODE=0");
  delay(LoRa_Delay);

  // Initialize display
  tft.init();
  tft.setRotation(ORIENTATION_RIGHT_HAND);
  tft.fillScreen(TFT_BLACK);
  
  // Initial drawing of speedometer
  tft.fillCircle(CENTER_X, CENTER_Y, SPEEDOMETER_RADIUS, DARKER_GREY);
  tft.drawSmoothCircle(CENTER_X, CENTER_Y, SPEEDOMETER_RADIUS, TFT_SILVER, DARKER_GREY);
  tft.drawArc(CENTER_X, CENTER_Y, METER_ARC_OUTSIDE, METER_ARC_INSIDE, METER_ARC_START_ANGLE, 
    METER_ARC_END_ANGLE, TFT_BLACK, DARKER_GREY);

  // Shared text drawing parameters
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(8);

  // Create the task as outlined above as "UI_Update"
  xTaskCreate(
        task_UpdateUI,  // task function name
        "UI_Update",    // A description name of the task
        1000,           // stack size
        NULL,           // task parameters
        2,              // task priority
        NULL            // task handle
        );

  // Create the task as outlined above as "send_LoRa"
  xTaskCreate(
        task_sendLora,  // task function name
        "send_LoRa",    // A description name of the task
        1000,           // stack size
        NULL,           // task parameters
        1,              // task priority
        NULL            // task handle
        );

  // Create the task as outlined above as "read_LoRa"
  xTaskCreate(
        task_readLora,  // task function name
        "read_LoRa",    // A description name of the task
        1000,           // stack size
        NULL,           // task parameters
        1,              // task priority
        NULL            // task handle
        );
}

// Do this task indefinitely
void loop() {

  // Read and process the throttle value
  uint16_t cur_throttle = analogRead(POT);
  // printf("Raw Throttle: %d\n", cur_throttle);
  // printf("%d\n", cur_throttle);
  if (cur_throttle < MIN_RAW_THROTTLE || cur_throttle > 4000) cur_throttle = MIN_RAW_THROTTLE; // Clamp values less than min to min (make sure throttle not negative)
  else if (cur_throttle > MAX_RAW_THROTTLE ) cur_throttle = MAX_RAW_THROTTLE; // Clamp values greater than max to max
  // printf("Clamped Throttle: %d\n", cur_throttle);
  throttle = map(cur_throttle, MIN_RAW_THROTTLE, MAX_RAW_THROTTLE, 0, POT_FULL_THROTTLE);
  // printf("Mapped Throttle: %d\n", throttle);

  sprintf(rawMsg, "T%i", throttle);  // Convert the throttle into a char array to be sent

}

// Update the speedometer UI display
void update_throttle_display() {
  static unsigned short last_angle = METER_ARC_START_ANGLE;

  // Calculate position on meter for a given speed
  unsigned short cur_throttle_angle = map(throttle, 0, UI_FULL_THROTTLE, METER_ARC_START_ANGLE, 
    METER_ARC_END_ANGLE);

  // Only update the display on changes
  if (cur_throttle_angle != last_angle) {
    // Hide previous number
    tft.fillCircle(CENTER_X, CENTER_Y, METER_ARC_INSIDE, DARKER_GREY);
    // Display speed on the LCD
    display_speed();
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
  // Ensure the colors are correct
  tft.setTextColor(TFT_WHITE, DARKER_GREY);
  // Reset Text Size
  tft.setTextSize(1);
  // Display the number_text on the LCD right below throttle
  tft.drawNumber(speed, CENTER_X, CENTER_Y);
  // Adjust text size for speed units
  tft.setTextSize(2);
  // Display the speed units
  tft.drawCentreString("m/s", CENTER_X, 220, 1);

}

// Keep track of total trip distance and display on the LCD
void display_trip()
{
  tft.setTextSize(2);                          // Adjust text size for trip
  tft.drawString(trip_read, 75, 10, 1);    // Display the odometer
  // tft.drawString("Trip: 1.3km", 75, 10, 1);    // Display the odometer
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
