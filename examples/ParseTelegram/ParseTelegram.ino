/**
 * This example contains an application to receive and parse a P1 telegram
 * Available for ESP32, ESP8266 (only one uart is available) and Arduino boards
 */

#include "P1MeterParser.h"

#define CTS_PIN 5
#define P1_BAUD 115200
#define P1_CONFIG SERIAL_8N1

// If using an ESP32 check which module you are using. The default pins of Serial1 and Serial2 might be in use by PSRAM or Flash
#if defined(ESP32)
#define ESP32_SERIAL1_TX 21
#define ESP32_SERIAL1_RX 22
#endif

// Use Serial1 as the hardware serial for the P1 connection
HardwareSerial *P1Serial = &Serial1;

// Create a meter object. pass the serial pointer and an optional CTS pin
P1Meter meter(P1Serial, CTS_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Serial logging

  // Start the P1 Serial
#if defined(__avr__)
  P1Serial->begin(P1_BAUD, SERIAL_8N1);
#elif defined(ESP8266)
  P1Serial->begin(P1_BAUD, (SerialConfig)SERIAL_8N1);
#elif defined(ESP32)
  P1Serial->begin(P1_BAUD, SERIAL_8N1, ESP32_SERIAL1_RX, ESP32_SERIAL1_TX);
#endif
}

void loop() {
  // put your main code here, to run repeatedly:
  meter.ReceiveTelegram(); // Only blocks when a telegram is avaiable untill it is fully received
  if (meter.DataReady) {
    Serial.println("Start of telegram");
    
    // Print out the complete telegram to Serial
    Serial.println(meter.GetBuffer());
    
    // Parse the telegram into an easily accesible object
    P1Data data = meter.ProcessTelegram();

    // Print data from the object to Serial. For all available data objects see the P1MeterParser.h file
    Serial.println(data.DateTime); // Print the datetime string to Serial 
    Serial.print("P1 Version: ");
    Serial.println(data.P1Version, HEX); // Print the P1 version as a hex. 50 is version 5.0

    Serial.print("Current consumption: ");
    Serial.print(data.ActualDelivered);
    Serial.println(" W");
    
    Serial.println("Valid CRC: " + String(data.ValidCRC == 1 ? "true" : "false"));
    Serial.println("End of telegram");
  }
}