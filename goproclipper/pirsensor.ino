// Program to read PIR sensor and send data to serial port at 9600 baud
// also finters pir sensor data to remove false positives
// frame buffer should be at least as long as the detection window to catch initial detection event
#include <Arduino.h>

#define LED_PIN LED_BUILTIN // LED on the board
#define PIR_PIN 12 // PIR sensor pin

const unsigned long SERIAL_INTERVAL = 100; // how long to wait between sending serial data, in milliseconds
unsigned long last_time; // last time we sent serial data

// Filters out false positives from PIR sensor by requiring multiple detections in a short time window
// Returns true if PIR is triggered, false otherwise
// frame buffer should be at least as long as the detection window
bool is_pir_triggered(bool pir_state) 
{
    static const unsigned long MIN_TIME_WITHOUT_EVENT_MS = 10000; // minimum time without event in milliseconds
    static const unsigned long DETECTION_WINDOW_MS = 5000; // window of time to look for multiple detections in milliseconds
    static const unsigned long WAIT_TIME_MS = 500; // time to wait after detection before looking for another detection in milliseconds
    static unsigned long last_event_time = 0;  // time of last PIR event
    static unsigned long last_check_time = 0;  // time of last check
    static bool true_positive = false;  // number of detections in the detection window

    // check if it's time to check for a PIR event
    unsigned long now = millis();
    if (now - last_check_time >= WAIT_TIME_MS) 
    {
        // check if PIR is triggered
        last_check_time = now;
        if (pir_state) 
        {
            last_event_time = now;
            // check if we've detected multiple events in the detection window
            if (now - last_event_time <= DETECTION_WINDOW_MS) 
            {
                true_positive = true;
                return true;
            } 
        }
        // check if we've been without an event for a long time
        if (now - last_event_time > MIN_TIME_WITHOUT_EVENT_MS) 
        {
            true_positive = false;
        }
    }
    return true_positive;
}

// sends pir_state to serial port at a rate of SERIAL_INTERVAL
void write_serial(bool pir_state) 
{
    static unsigned long last_time = 0; // last time we sent serial data
    if (millis() - last_time < SERIAL_INTERVAL)
    {
        return; // if it hasn't been long enough, return
    } 
    char c = (pir_state ? '1' : '0'); // Convert to character '0' or '1'
    Serial.write(c);  // send character to serial port
}

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
    PIR_detect = is_pir_triggered(PIR_detect); // filter out false positives
    write_serial(PIR_detect); // send filtered data to serial port
}
