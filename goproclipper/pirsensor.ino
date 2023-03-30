// Program to read PIR sensor and send data to serial port at 9600 baud
#include <Arduino.h>

#define LED_PIN LED_BUILTIN // LED on the board
#define PIR_PIN 12 // PIR sensor pin

const unsigned long SERIAL_INTERVAL = 100; // how long to wait between sending serial data, in milliseconds
unsigned long last_time; // last time we sent serial data

void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT_PULLUP); // reverse logic
  Serial.begin(9600);
  last_time = millis();
}

void loop() 
{
  bool PIR_detect = digitalRead(PIR_PIN); // read PIR sensor
  digitalWrite(LED_PIN, PIR_detect); // turn LED on if PIR detects motion
  
  unsigned long now_time = millis();  // get current time
  if (now_time - last_time > SERIAL_INTERVAL)  // check if it's time to send serial data
  {
    char c = (PIR_detect ? '1' : '0'); // Convert to character '0' or '1'
    Serial.write(c);  // send character to serial port
    last_time = now_time; // update last time we sent serial data
  }
}
