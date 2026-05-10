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

void setup() {
  Serial.begin(115200); // Initialize the serial port and set the baud rate to 115200
  delay(500);
  Serial.println("ESP32 is ready!");  // Print the string "UNO is ready!"

  Wire.begin(SDA, SCL); // attach the IIC pin
  if (!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }
  lcd.init(); // LCD driver initialization
  lcd.backlight(); // Open the backlight
  lcd.setCursor(0,0); // Move the cursor to row 0, column 0
  lcd.print("Hello, World!"); // The print content is displayed on the LCD
}

void loop() {
  // Get the character input
  char keyPressed = myKeypad.getKey();
  // If there is a character input, sent it to the serial port
  if (keyPressed) {
    lcd.setCursor(0,1); // Move the cursor to row 1, column 0
    lcd.print("Key: ");
    lcd.print(keyPressed);
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
