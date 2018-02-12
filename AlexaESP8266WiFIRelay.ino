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
// - add spdiff settings store - DONE
// - add mqtt client - DONE
// - add webServer that controls relay - DONE
// - add alexa belkin simulator
// - add hue-bridge support
// - save error to log file and display it in the webserver

// includes
#include <FS.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>

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
char cfg_ap_name[14] = "ESPWiFiRelay";

// config
char configFilename[14] = "/config.json";

// mqtt
char cfg_mqtt_host[40] = "broker.mqttdashboard.com";
char cfg_mqtt_port[6] = "1883";
const char *inTopic = "controlRelay";
const char *outTopic = "relayStatus";
const int maxMqttRetries = 5;
int mqttRetry = 0;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// webServer
ESP8266WebServer webServer(80);
bool serverIsRunning = false;

// spdiff
bool shouldSaveConfig = false; // flag for saving data

// Methods
void ledBlinkTest();
void relayToggleTest();
void turnRelayOn();
void turnRelayOff();
void buttonLoop();
void mqttLoop();
void webServerLoop();
void reconnectMqttClient();
void toggleRelay();
void setupWifi();
void setupMqtt();
void setupWebServer();
void reset();
void wifiSaveConfigCallback();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void println();
void readConfig();
void saveConfig();
String getRelayState();
String restartEsp();

void setup()
{
    // serial settings
    Serial.begin(9600); // 9600 because the relay module communicates with this baud rate

    // FOR DEBUG: wait serial monitor to settle down
    delay(1000);

    // Setup pins
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT);

    // Turn off
    turnRelayOff();

    // do tests
    ledBlinkTest();
    relayToggleTest();

    // read config
    readConfig();

    // wifi
    setupWifi(false);

    // webServer
    setupWebServer();
}

void loop()
{
    buttonLoop();
    mqttLoop();
    webServerLoop();
}

void mqttLoop()
{

    if (!mqttClient.connected())
    {
        reconnectMqttClient();
    }
    mqttClient.loop();
}

void webServerLoop()
{
    if (serverIsRunning)
    {
        webServer.handleClient();
    }
}

void handleRoot()
{
    webServer.send(200, "text/html", "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{ margin: auto;width: calc(100% - 4em);text-align: center;font-family: Verdana;padding: 2em;} button{width: calc(100% - 0.8em);margin-bottom: 1em; margin: 0.4em;padding: 0.4em; }</style><script>function load(page){var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) { document.getElementById(\"result\").innerHTML = this.responseText;} else {} }; xhttp.open(\"GET\", page, true); xhttp.send();};function init(){load('relay/state');setInterval(function(){load('relay/state');}, 5000);}</script></head><body onload=\"init()\"><h2>WiFi Relay control</h2><div><button onclick=\"load('relay/on')\">Turn on</button> <button onclick=\"load('relay/off')\">Turn off</button> <button onclick=\"load('relay/toggle')\">Toggle</button> <hr><button onclick=\"load('relay/state')\">Refresh</button><button onclick=\"load('esp/restart')\">Restart</button><p>State: <span id=\"result\"></span></p></div>");
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webServer.uri();
    message += "\nMethod: ";
    message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += webServer.args();
    message += "\n";
    for (uint8_t i = 0; i < webServer.args(); i++)
    {
        message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
    }
    webServer.send(404, "text/plain", message);
}

void setupWebServer()
{
    webServer.on("/", handleRoot);

    webServer.on("/relay/on", []() {
        turnRelayOn();
        webServer.send(200, "text/plain", "relay is on");
    });
    webServer.on("/relay/off", []() {
        turnRelayOff();
        webServer.send(200, "text/plain", "relay is off");
    });
    webServer.on("/relay/toggle", []() {
        toggleRelay();
        webServer.send(200, "text/plain", getRelayState());
    });
    webServer.on("/relay/state", []() {
        webServer.send(200, "text/plain", getRelayState());
    });
    webServer.on("/esp/restart", []() {
        webServer.send(200, "text/plain", restartEsp());
    });

    webServer.onNotFound(handleNotFound);
}

String getRelayState()
{
    if (relayState == RELAY_ON)
    {
        return "relay is on";
    }
    else
    {
        return "relay is off";
    }
}

void stopWebServer()
{
    if (!serverIsRunning)
    {
        return;
    }

    webServer.stop();

    serverIsRunning = false;

    if (debug)
    {
        Serial.println("HTTP webServer stopped");
    }
}

void startWebServer()
{
    webServer.begin();

    serverIsRunning = true;

    if (debug)
    {
        Serial.println("HTTP webServer started");
    }
}

void reconnectMqttClient()
{
    // Loop until we're reconnected
    while (!mqttClient.connected() && mqttRetry < maxMqttRetries)
    {
        mqttRetry++;

        if (debug)
        {
            Serial.print("Attempting MQTT connection...");
        }

        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);

        // Attempt to connect
        if (mqttClient.connect(clientId.c_str()))
        {
            if (debug)
            {
                Serial.println("connected");
            }
            // Once connected, publish an announcement...
            mqttClient.publish(outTopic, "UNKNOWN");
            // ... and resubscribe
            mqttClient.subscribe(inTopic);

            mqttRetry = 0;
        }
        else
        {

            if (mqttRetry >= maxMqttRetries)
            {
                if (debug)
                {
                    Serial.println("mqtt connection failed. Check config.");
                    Serial.print(cfg_mqtt_host);
                    Serial.print(":");
                    Serial.println(cfg_mqtt_port);
                }
            }
            else
            {
                if (debug)
                {
                    Serial.print("failed, rc=");
                    Serial.print(mqttClient.state());
                    Serial.println(" try again in 5 seconds");
                }

                // Wait 5 seconds before retrying
                delay(5000);
            }
        }
    }
}

void setupMqtt()
{
    mqttRetry = 0;

    String port_str(cfg_mqtt_port);
    int port = port_str.toInt();
    if (debug)
    {

        Serial.print("mqtt server");
        Serial.println(cfg_mqtt_host);
        Serial.print("mqtt port");
        Serial.println(port);
    }
    mqttClient.setServer(cfg_mqtt_host, port);
    mqttClient.setCallback(mqttCallback);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (debug)
    {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");

        for (int i = 0; i < length; i++)
        {
            Serial.print((char)payload[i]);
        }

        Serial.println();
    }

    // TODO better conversion
    String payloadStr = "";
    for (int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }

    Serial.println(payloadStr);
    const char *p = payloadStr.c_str();

    if (strcmp(topic, inTopic) == 0)
    {
        Serial.println("Found in topic");
        if (strcmp(p, "ON") == 0 || strcmp(p, "1") == 0)
        {
            turnRelayOn();
        }
        else if (strcmp(p, "OFF") == 0 || strcmp(p, "0") == 0)
        {
            turnRelayOff();
        }
        else if (strcmp(p, "TOGGLE") == 0 || strcmp(p, "2") == 0)
        {
            toggleRelay();
        }
    }
}

void println(const char c[])
{
    if (debug)
    {
        Serial.println(c);
    }
}

void readConfig()
{
    if (SPIFFS.begin())
    {
        if (SPIFFS.exists(configFilename))
        {
            // file exists, reading and loading

            println("reading config file");
            File configFile = SPIFFS.open(configFilename, "r");

            if (configFile)
            {
                println("opened config file");
                size_t size = configFile.size();

                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject &json = jsonBuffer.parseObject(buf.get());

                if (debug)
                {
                    json.printTo(Serial);
                }

                if (json.success())
                {
                    println("\nparsed json");

                    strcpy(cfg_mqtt_host, json["mqtt_host"]);
                    strcpy(cfg_mqtt_port, json["mqtt_port"]);
                    strcpy(cfg_ap_name, json["ap_name"]);
                }
                else
                {
                    println("failed to load json config");
                }
            }
        }
    }
    else
    {
        println("Failed to mount FS. Did you flash with the correct flash size? E.g. 1M (128k SPIFFS)");
    }
}

void saveConfig()
{
    //save the custom parameters to FS
    println("saving config");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();

    json["mqtt_host"] = cfg_mqtt_host;
    json["mqtt_port"] = cfg_mqtt_port;
    json["ap_name"] = cfg_ap_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        println("failed to open config file for writing");
    }

    if (debug)
    {
        json.printTo(Serial);
    }

    json.printTo(configFile);
    configFile.close();
}

void setupWifi(boolean useConfigPortal)
{
    // stop services
    mqttClient.disconnect();
    stopWebServer();

    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //set config save notify callback
    wifiManager.setSaveConfigCallback(wifiSaveConfigCallback);
    wifiManager.setDebugOutput(debug);

    // add custom parameters
    WiFiManagerParameter custom_text("<p>Enter the WiFi ssid and password. Use 'scan' to display network around you.</p><p>You can also rename the ap ssid</p><p>MQTT settings are optional.</p>");
    WiFiManagerParameter back_link("<p><a href='/'>Back to home</a></p>");
    WiFiManagerParameter mqtt_host("mqtt_host", "mqtt host", cfg_mqtt_host, 40); // id, placeholder, defaultValue, length, customAttr
    WiFiManagerParameter mqtt_port("mqtt_port", "mqtt port", cfg_mqtt_port, 6);
    WiFiManagerParameter ap_name("ap_name", "ap_name port", cfg_ap_name, 14);

    wifiManager.addParameter(&mqtt_host);
    wifiManager.addParameter(&mqtt_port);
    wifiManager.addParameter(&ap_name);
    wifiManager.addParameter(&custom_text);
    wifiManager.addParameter(&back_link);

    // one minute timeout
    // wifiManager.setTimeout(120);
    if (useConfigPortal)
    {
        println("Entering config portal");

        wifiManager.startConfigPortal(cfg_ap_name); // TODO check why old ap name is used instead of cfg ap name
    }
    else
    {
        // try to autoconnect to last known configuration
        if (!wifiManager.autoConnect(cfg_ap_name))
        {
            delay(3000);

            //reset whole device and try again, or maybe put it to deep sleep
            restartEsp();
        }
    }

    println("Local ip");
    if (debug)
    {
        Serial.println(WiFi.localIP());
    }

    if (shouldSaveConfig)
    {
        strcpy(cfg_mqtt_host, mqtt_host.getValue());
        strcpy(cfg_mqtt_port, mqtt_port.getValue());
        strcpy(cfg_ap_name, ap_name.getValue());

        saveConfig();
    }

    // All done start services
    startWebServer();
    setupMqtt();
}

String restartEsp()
{
    // restart esp
    ESP.reset();

    // restart after x seconds
    delay(5000);

    return "ESP restarts";
}

void wifiSaveConfigCallback()
{
    // callback notifying us of the need to save config
    shouldSaveConfig = true;
}

void reset()
{
    /**
     * NOT TESTED YET
     **/

    // reset wifimanager
    WiFiManager wifiManager;
    wifiManager.resetSettings();

    // format storage
    SPIFFS.format();

    // clear EEPROM
    EEPROM.begin(512);           // size
    for (int i = 0; i < 96; ++i) // position of ssid and password
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();

    // restart esp
    ESP.reset();
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

    if ((currentMillis - lastDebounceTime) > debounceDelay)
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
    for (int i = 0; i < 1; ++i)
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

    if (debug)
    {
        Serial.println("RELAY_ON");
    }

    // send status via mqtt
    mqttClient.publish(outTopic, "ON");
}

void turnRelayOff()
{
    Serial.write(relCmdOFF, sizeof(relCmdOFF)); // turns the relay OFF
    digitalWrite(ledPin, LOW);
    relayState = RELAY_OFF;

    if (debug)
    {
        Serial.println("RELAY_OFF");
    }

    // send status via mqtt
    mqttClient.publish(outTopic, "OFF");
}

void relayToggleTest()
{
    int speed = 1000;

    // blink 5 times
    for (int i = 0; i < 1; ++i)
    {
        delay(speed);
        turnRelayOn();
        delay(speed);
        turnRelayOff();
    }
}
