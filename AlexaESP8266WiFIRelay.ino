/***
 * by Andreas Taske (andreas@taske.de) 2018
 * 
 * used exp8266 with wifi relay: 
 * **/

// TODO:
// - add relay control - DONE
// - add control led - DONE
// - add button that controls relay - DONE
// - add wifimanager - DONE
// - add spdiff settings store
// - add mqtt client
// - add webserver that controls relay
// - add alexa belkin simulator

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

// Debugging
boolean debug = true;

// LED
const int ledPin = 2;

// button
#define PUSH 0x1
#define SWITCH 0x0
#define TOGGLE_SWITCH 0x2

const int btnPin = 0;              // also used to enable flashing mode
const int btnType = TOGGLE_SWITCH; // PUSH, SWITCH or TOGGLE_SWITCH
int lastBtnState = HIGH;           // because the pins are pulled high
int btnState;                      // the current reading from the input pin
int btnStateChangeCount = 0;
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
unsigned long lastBtnChangedMillis = 0;

// relay
byte relCmdON[] = {0xA0, 0x01, 0x01, 0xA2};  // Hex command to send to serial for open relay
byte relCmdOFF[] = {0xA0, 0x01, 0x00, 0xA1}; // Hex command to send to serial for close relay

#define RELAY_ON 0x1
#define RELAY_OFF 0x0
int relayState = RELAY_OFF;

// wifi
char apName[14] = "ESPWiFiRelay";

// Methods
void ledBlinkTest();
void relayToggleTest();
void turnRelayOn();
void turnRelayOff();
void buttonLoop();
void toggleRelay();
void setupWifi();

void setup()
{
    // serial settings
    Serial.begin(9600); // 9600 because the relay module communicates with this baud rate

    // FOR DEBUG: wait serial monitor to settle down
    delay(2000);

    // Setup pins
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT);

    // Turn off
    turnRelayOff();

    // do tests
    ledBlinkTest();
    relayToggleTest();

    // wifi
    setupWifi(false);
}

void loop()
{
    buttonLoop();
}

void setupWifi(boolean useConfigPortal)
{
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    wifiManager.setDebugOutput(debug);
    WiFiManagerParameter custom_text("<p>Enter the WiFi ssid and password. Use 'scan' to display network around you.</p><p>MQTT settings are optional.");
    WiFiManagerParameter back_link("<p><a href='/'>Back to home</a></p>");
    WiFiManagerParameter mqtt_host("mqtt_host", "mqtt host", "broker.mqttdashboard.com", 40); // id, placeholder, defaultValue, length, customAttr
    WiFiManagerParameter mqtt_port("mqtt_port", "mqtt port", "1883", 6);
    wifiManager.addParameter(&mqtt_host);
    wifiManager.addParameter(&mqtt_port);
    wifiManager.addParameter(&custom_text);
    wifiManager.addParameter(&back_link);

    // one minute timeout
    // wifiManager.setTimeout(120);
    if (useConfigPortal)
    {
        wifiManager.startConfigPortal(apName);
    }
    else
    {
        // try to autoconnect to last known configuration
        if (!wifiManager.autoConnect(apName))
        {
            delay(3000);

            //reset whole device and try again, or maybe put it to deep sleep
            ESP.reset();

            // restart after 5 seconds
            delay(5000);
        }
    }
}

void buttonLoop()
{
    int currentBtnState = digitalRead(btnPin);

    // If the switch changed, due to noise or pressing:
    if (currentBtnState != lastBtnState)
    {
        // reset the debouncing timer
        lastDebounceTime = millis();
    }

    unsigned long currentMillis = millis();

    if (( currentMillis - lastDebounceTime) > debounceDelay)
    {
        // whatever the currentBtnState is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:
        if (currentBtnState != btnState)
        {
           
            btnState = currentBtnState;

            // check if button state changes rapidly 10 times, enter ap mode
            if (currentMillis - lastBtnChangedMillis < 500)
            {
                btnStateChangeCount++;

                if (btnStateChangeCount > 10)
                {
                    btnStateChangeCount = 0;
                    setupWifi(true);
                }
            }
            else
            {
                btnStateChangeCount = 0;
            }

            lastBtnChangedMillis = currentMillis;

            // relay change
            if (btnType == SWITCH)
            {
                if (btnState == LOW)
                {
                    turnRelayOn();
                }
                else
                {
                    turnRelayOff();
                }
            }
            else if (btnType == PUSH)
            {
                // only toggle the relay if the new button state is HIGH
                if (btnState == LOW)
                {
                    toggleRelay();
                }
            }
            else // TOGGLE_SWITCH
            {
                toggleRelay();
            }
        }
    }

    lastBtnState = currentBtnState;
}

void toggleRelay()
{
    if (relayState == RELAY_OFF)
    {
        turnRelayOn();
    }
    else
    {
        turnRelayOff();
    }
}

void ledBlinkTest()
{
    int speed = 250;

    // blink 5 times
    for (int i = 0; i < 5; ++i)
    {
        delay(speed);
        digitalWrite(ledPin, HIGH);
        delay(speed);
        digitalWrite(ledPin, LOW);
    }
}

void turnRelayOn()
{
    Serial.write(relCmdON, sizeof(relCmdON)); // turns the relay ON
    digitalWrite(ledPin, HIGH);
    relayState = RELAY_ON;
}

void turnRelayOff()
{
    Serial.write(relCmdOFF, sizeof(relCmdOFF)); // turns the relay OFF
    digitalWrite(ledPin, LOW);
    relayState = RELAY_OFF;
}

void relayToggleTest()
{
    int speed = 1000;

    // blink 5 times
    for (int i = 0; i < 5; ++i)
    {
        delay(speed);
        turnRelayOn();
        delay(speed);
        turnRelayOff();
    }
}