/***
 * by Andreas Taske (andreas@taske.de) 2018
 * 
 * used exp8266 with wifi relay: 
 * **/

// TODO:
// - add relay control
// - add control led
// - add button that controls relay
// - add wifimanager
// - add spdiff settings store
// - add mqtt client
// - add webserver that controls relay
// - add alexa belkin simulator

// LED
const int ledPin = 2;

// button
const int btnPin = 0; // also used to enable flashing mode

// relay
byte relCmdON[] = {0xA0, 0x01, 0x01, 0xA2};  // Hex command to send to serial for open relay
byte relCmdOFF[] = {0xA0, 0x01, 0x00, 0xA1}; // Hex command to send to serial for close relay

#define RELAY_ON 0x1
#define RELAY_OFF 0x0
int relayState = RELAY_OFF;

// Methods
void ledBlinkTest();
void turnRelayOn();
void turnRelayOff();

void setup()
{
    // serial settings
    Serial.begin(9600); // 9600 because the relay module communicates with this baud rate

    // FOR DEBUG: wait serial monitor to settle down
    delay(2000);

    // Setup pins
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT);

    // Turn off led
    digitalWrite(ledPin, LOW);

    // do tests
    ledBlinkTest();
    relayToggleTest();
}

void loop()
{
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
    relayState = RELAY_ON;
}

void turnRelayOff()
{
    Serial.write(relCmdOFF, sizeof(relCmdOFF)); // turns the relay OFF
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