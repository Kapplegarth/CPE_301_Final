#ifndef LCD_H
#define LCD_H

#include <LiquidCrystal.h>
// LCD pins <--> Arduino pins
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 2, D7 = 8;
int printErrorOnce = 0;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void lcd_setup(){
  lcd.begin(16, 2);
}

void reset_lcd(){
  printErrorOnce = 0;
  lcd.clear();
}

void displayErrorMessage(){
  if(printErrorOnce==0){
    lcd.clear();
    lcd.print("Water level low");
    printErrorOnce = 1;
  }
}

#endif