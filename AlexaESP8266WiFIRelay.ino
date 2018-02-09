/***
 * by Andreas Taske (andreas@taske.de) 2018
 * 
 * used exp8266 with wifi relay: 
 * **/

// TODO:
// - add relay control - DONE
// - add control led - DONE
// - add button that controls relay - DONE
// - add wifimanager
// - add spdiff settings store
// - add mqtt client
// - add webserver that controls relay
// - add alexa belkin simulator

// LED
const int ledPin = 2;

// button
#define PUSH 0x1
#define SWITCH 0x0

const int btnPin = 0;               // also used to enable flashing mode
const int btnType = SWITCH;         // push or switch
int lastBtnState = HIGH;            // because the pins are pulled high
int btnState;                       // the current reading from the input pin
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers

// relay
byte relCmdON[] = {0xA0, 0x01, 0x01, 0xA2};  // Hex command to send to serial for open relay
byte relCmdOFF[] = {0xA0, 0x01, 0x00, 0xA1}; // Hex command to send to serial for close relay

#define RELAY_ON 0x1
#define RELAY_OFF 0x0
int relayState = RELAY_OFF;

// Methods
void ledBlinkTest();
void relayToggleTest();
void turnRelayOn();
void turnRelayOff();
void buttonLoop();
void toggleRelay();

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
}

void loop()
{
    buttonLoop();
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

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // whatever the currentBtnState is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:
        if (currentBtnState != btnState)
        {
            btnState = currentBtnState;

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
            else // PUSH
            {
                // only toggle the relay if the new button state is HIGH
                if (btnState == LOW)
                {
                    toggleRelay();
                }
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