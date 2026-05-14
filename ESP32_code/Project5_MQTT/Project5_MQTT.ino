#include <WiFi.h> // WiFi library
#include <PubSubClient.h> // MQTT library
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define SDA 13 //Define SDA pins
#define SCL 14 //Define SCL pins

LiquidCrystal_I2C lcd(0x27,16,2);

// define the symbols on the buttons of the keypad
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[4] = {27, 26, 25, 33}; // connect to the row pinouts of the keypad
byte colPins[4] = {15, 21, 22, 23};   // connect to the column pinouts of the keypad

// initialize an instance of class NewKeypad
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

// WiFi Network Credentials
const char* ssid = "keanaaa"; // Replace with WiFi SSID (USE HOTSPOT)
const char* password = "password123!"; // Replace w/ WiFi password

// MQTT Broker Settings
const char* mqtt_server_ip = "cs2600.duckdns.org"; // Replace w/ broker IP (GCP Duck DNS)
const int mqtt_port = 1883;

// Object Declarations
WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server_ip, mqtt_port, wifiClient); // Creates an MQTT client using WiFi // PubSubClient MQTTclient(mqtt_server, mqtt_port, wifiClient);

// Global state for scrolling
unsigned long lastScroll = 0;
int scrollPos = 0;
char currentLine0[128] = {0};
char currentLine1[128] = {0};
bool isScrolling = false;

#define LCD_COLS 16
#define SCROLL_PAD 16

unsigned long lastReconnectAttempt = 0;

char lastStatus[128] = {0};
char lastAvailSpaces[128] = {0};

bool gameStarted = false;

void connectToWifi() {
  Serial.println("\nConnecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setLine(int row, const char* message) {
    char* target = (row == 0) ? currentLine0 : currentLine1;
    // Build padded string: 16 spaces + message + 16 spaces
    snprintf(target, 128, "                %s                ", message);
    scrollPos = 0;
}

void printStatus(const char* message) {
    char buf[96];
    snprintf(buf, sizeof(buf), "Status: %s", message);
    setLine(0, buf);
}

void printAvailSpaces(const char* message) {
    char buf[96];
    snprintf(buf, sizeof(buf), "Avail: %s", message);
    setLine(1, buf);
}

// Handling incoming MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char incomingMessage[128] = {0};
    if (length >= sizeof(incomingMessage)) length = sizeof(incomingMessage) - 1;
    memcpy(incomingMessage, payload, length);

    if (strcmp(topic, "statusOutput") == 0) {
        if (strcmp(incomingMessage, lastStatus) == 0) return;
        strncpy(lastStatus, incomingMessage, sizeof(lastStatus) - 1);
        gameStarted = true;

        Serial.print("Topic: statusOutput, Message: ");
        Serial.println(incomingMessage);

        printStatus(incomingMessage);
    }
    else if (strcmp(topic, "availSpacesOutput") == 0) {
        if (strcmp(incomingMessage, lastAvailSpaces) == 0) return;
        strncpy(lastAvailSpaces, incomingMessage, sizeof(lastAvailSpaces) - 1);
        gameStarted = true;

        Serial.print("Topic: availSpacesOutput, Message: ");
        Serial.println(incomingMessage);

        printAvailSpaces(incomingMessage);
    }
}

void lcdPrint(int row, const char* text, int start) {
    char slice[17] = {0};
    int len = strlen(text);
    for (int i = 0; i < 16; i++) {
        slice[i] = (start + i < len) ? text[start + i] : ' ';
    }
    slice[16] = '\0';
    lcd.setCursor(0, row);
    lcd.print(slice);
}

void scrollText() {
    if (!gameStarted) return;
    if (isScrolling) return;

    if (millis() - lastScroll >= 500) {
        isScrolling = true;
        lastScroll = millis();

        int len = strlen(currentLine0);
        if (scrollPos >= len) scrollPos = 0;

        lcdPrint(0, currentLine0, scrollPos);
        lcdPrint(1, currentLine1, scrollPos);

        scrollPos++;
        isScrolling = false;
    }
}

// Reconnect to MQTT Broker
void reconnectToMQTT() {
    if (mqttClient.connected()) return;
    
    unsigned long now = millis();
    if (now - lastReconnectAttempt < 5000) return; // don't retry too fast
    lastReconnectAttempt = now;

    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP32_S3_Client")) {
        Serial.println("Connected!");
        mqttClient.subscribe("statusOutput");
        mqttClient.subscribe("availSpacesOutput");
    } else {
        Serial.print("Failed, state: ");
        Serial.println(mqttClient.state());
    }
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 is ready!");

  connectToWifi();
  mqttClient.setServer(mqtt_server_ip, mqtt_port);  // Set MQTT Broker
  mqttClient.setKeepAlive(60); // Keeps connection alive for 60 seconds
  mqttClient.setCallback(mqttCallback); // Set callback function

  Wire.begin(SDA, SCL);
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  //if(!i2CAddrTest(0x27)) {
  //  lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  //}
  //lcd.init();
  //lcd.backlight();
  //lcd.clear();
  //delay(100);
}

// the loop function runs over and over again forever...
void loop() {
    reconnectToMQTT(); // handles connection check internally

    mqttClient.loop();
    //scrollText();

    char keyPressed = myKeypad.getKey();
    if (keyPressed) {
        mqttClient.publish("keypadInput", (const byte *) &keyPressed, 1);
    }
}

bool i2CAddrTest(uint8_t addr) {
  Wire.begin(SDA, SCL);
  Wire.setClock(50000); // 100kHz instead of default 400kHz
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    return true;
  }
  return false;
}