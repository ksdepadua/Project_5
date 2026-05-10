/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define SDA 13
#define SCL 14

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

String messageStatic = "Static message";
String messageToScroll = "This is a scrolling message with more than 16 characters";

// Function to scroll text
// The function acepts the following arguments:
// row: row number where the text will be displayed
// message: message to scroll
// delayTime: delay between each character shifting
// lcdColumns: number of columns of your LCD
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void setup(){
  Wire.begin(SDA, SCL);
  if(!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }
  
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  lcd.clear();
}

void loop(){
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print static message
  lcd.print(messageStatic);
  delay(250);
  // print scrolling message
  scrollText(1, messageToScroll, 500, lcdColumns);
}

bool i2CAddrTest(uint8_t addr) {
  Wire.begin();
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    return true;
  }
  return false;
}