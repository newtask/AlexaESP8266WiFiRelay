/***
 * by Andreas Taske (andreas@taske.de) 2018
 * **/

// TODO:
// - add relais control
// - add control led
// - add button that controls relais
// - add wifimanager
// - add spdiff settings store
// - add mqtt client
// - add webserver that controls relais
// - add alexa belkin simulator

// LED
const int ledPin = 2;

// button
const int btnPin = 0; // also used to enable flashing mode

// relais


// Methods
void ledBlinkTest();

void setup()
{
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