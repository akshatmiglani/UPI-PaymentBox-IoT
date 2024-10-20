#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
#define ROWS  4
#define COLS  4
 
char keyMap[ROWS][COLS] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};
 
uint8_t rowPins[ROWS] = {14, 27, 26, 25}; // GIOP14, GIOP27, GIOP26, GIOP25
uint8_t colPins[COLS] = {33, 32, 18, 19}; // GIOP33, GIOP32, GIOP18, GIOP19
uint8_t LCD_CursorPosition = 0;
 
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS );
LiquidCrystal_I2C I2C_LCD(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
 
void setup(){
  Serial.begin(115200);
  // Initialize The I2C LCD
  I2C_LCD.init();
  // Turn ON The Backlight
  I2C_LCD.backlight();
  // Clear The Display
  I2C_LCD.clear();
}
 
void loop(){
  
  char key = keypad.getKey();
  
  if (key) {
    Serial.print(key);
    I2C_LCD.setCursor(LCD_CursorPosition++, 0);
    if(LCD_CursorPosition == 16 || key == 'D')
    {
      I2C_LCD.clear();
      LCD_CursorPosition = 0;
    }
    else
    {
      I2C_LCD.print(key);
    }
  }
}