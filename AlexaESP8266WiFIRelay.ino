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





// Methods
void ledBlinkTest();

void setup()
{
    // serial settings
    Serial.begin(9600);  // 9600 because the relay module communicates with this baud rate

    // Setup pins
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT);

    // Turn off led
    digitalWrite(ledPin, LOW);

    ledBlinkTest();
}

void loop()
{

}

void ledBlinkTest(){
    int blinkSpeed = 250;

    // blink 5 times
    for(int i=0; i<5;++i){
        delay(blinkSpeed);
        digitalWrite(ledPin, HIGH);
        delay(blinkSpeed);
        digitalWrite(ledPin, LOW);
    }
}