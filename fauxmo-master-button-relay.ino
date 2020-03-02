#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include "fauxmoESP.h"

// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "creds.h"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200
#define LED          5
#define BUTTON 4
#define ID           "kitchen light"

int switchState = 0; // actual read value from pin4
int oldSwitchState = 0; // last read value from pin4
int ledState = 0; // is the switch on = 1 or off = 0
int alexaState = 0;
int lightsOn = 0;


// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    // LEDs
    pinMode(LED, OUTPUT);
    pinMode(BUTTON, INPUT); // push button 

    
    digitalWrite(LED, HIGH);
   

    // Wifi
    wifiSetup();

    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    fauxmo.addDevice(ID);
    

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        if (strcmp(device_name, ID)==0) {
            //digitalWrite(LED, state ? HIGH : LOW);
            if (state) {
              alexaState = 1;
              } else {
                alexaState = 2;
                }
        }
    });

}

void loop() {

    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();
    
    //the physical button implementaiton
    switchState = digitalRead(BUTTON); // read the Button State
    ledState = !digitalRead(LED);

    if ( (oldSwitchState != switchState) || (( (alexaState == 1)&&(ledState == 0))||((alexaState == 2)&&(ledState == 1))     ) )
    {
      oldSwitchState = switchState;
      alexaState = 0;
      lightsOn = !lightsOn;
      
      }
     if(lightsOn)
     {
     digitalWrite(LED, LOW); // set the Relay on
     } else {
     digitalWrite(LED, HIGH); // set the Relay off
     }
    

    
    
    
     
    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID, true, 255);

}
