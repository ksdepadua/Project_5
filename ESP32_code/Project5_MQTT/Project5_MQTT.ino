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

// Handling incoming MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String incomingMessage = "";

  // Convert incoming byte array to String
  for (unsigned int i = 0; i < length; i++) {
    incomingMessage += (char)payload[i];
  }

  incomingMessage.trim(); // Removes trailing whitespace/newline

  // ...
}

// Reconnect to MQTT Broker
void reconnectToMQTT() {
  while(!mqttClient.connected()) {
    Serial.print("Connecting to MQTT Client...");

    // Attempt to connect with a client ID
    if(mqttClient.connect("ESP32_S3_Client")) {
      Serial.println("Connected to MQTT broker!");
    }

    else {
      Serial.print("Failed with state ");
      Serial.print(mqttClient.state());
      Serial.println(", Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 is ready!");

  connectToWifi();
  mqttClient.setServer(mqtt_server_ip, mqtt_port);  // Set MQTT Broker
  //mqttClient.setCallback(mqttCallback); // Set callback function

  Wire.begin(SDA, SCL);
  if(!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Hello, World!");
}

// the loop function runs over and over again forever...
void loop() {
  if(!mqttClient.connected()) {
    reconnectToMQTT();
  }

  char keyPressed = myKeypad.getKey();
  if(keyPressed) {
    mqttClient.publish("keypadInput", (const char *) &keyPressed);
  }
}

bool i2CAddrTest(uint8_t addr) {
  Wire.begin();
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    return true;
  }
  return false;
}