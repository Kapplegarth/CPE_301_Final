//Kyle Applegarth
//Ryan Zheng
//Ronald Icely
//Adam Veilleaux

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
//Complete the Real time Clock
//Does Github work at all

//Create Serial Registers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//Create the Analog Read stuff
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//Timer Variables
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

//water sensor
volatile unsigned char *PORTC =  (unsigned char *) 0x28;
volatile unsigned char *DDRC =  (unsigned char *) 0x27;
//start and stop buttons
volatile unsigned char *PORTD =  (unsigned char *) 0x2B;
volatile unsigned char *DDRD =  (unsigned char *) 0x2A;
//reset button
volatile unsigned char *PORTE =  (unsigned char *) 0x2E;
volatile unsigned char *DDRE =  (unsigned char *) 0x2D;

//Add the LED register
#include "LEDs.h"
//RTC
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday","Friday", "Saturday"};
// LCD pins <--> Arduino pins
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 2, D7 = 8;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
#include <dht.h> //install the DHTLib library
dht DHT;
#define DHT11_PIN 7
#define TEMP_THRESHOLD 25
#define VENT_LOWER_THRESHOLD 200
#define VENT_UPPER_THRESHOLD 900
#define WATER_SIGNAL 1
#define WATER_POWER 36
#define WATER_THRESHOLD 100
int printErrorOnce = 0;
//Create Stepper Motor
const int stepsPerRevolution = 2038;
//pin 32 IN1, Pin 34 IN2, IN3 33, Pin 35 IN4
Stepper myStepper = Stepper(stepsPerRevolution, 32,33,34,35);
//Create ISR for Start and Stop button and Reset
//Declare Variables
volatile char state;//d is for disabled, r for running, i for idle, and e for error
char previousState;//d is for disabled, r for running, i for idle, and e for error
double waterLevel = 0;
//Set UP DC Motor
#include "FanMotor.h"
unsigned long previousMillis = 0;
const long interval = 60000;
unsigned long currentMillis = 0;
void setup()
{
  U0init(9600);
  adc_init();//Start the ADC Read
  lcd.begin(16, 2);
  //Set the Water sensor
  *DDRC |= 0b00000010;
  *PORTC &= 0b11111101; //start with water sensor off
  rtc.begin();
  lcd.begin(16, 2); // set up number of columns and rows
  state = 'd';//state is disabled
  previousState = 'd';//Set the previous state
  //Set the ISR Stop Button pin 19
  *DDRD &= 0b11111011; //set to input with pullup
  *PORTD |= 0b00000100;
  attachInterrupt (digitalPinToInterrupt(19),setToDisabled,RISING);
  //Set the ISR start Button pin 18
  *DDRD &= 0b11110111; //set to input with pullup
  *PORTD |= 0b00001000;
  attachInterrupt (digitalPinToInterrupt(18),startCooler,RISING);
  //Set the ISR reset button to pin 3
  *DDRE &= 0b11011111; //set to input with pullup
  *PORTE |= 0b00100000;
  attachInterrupt (digitalPinToInterrupt(3),resetCooler,RISING);
  //Set up the lights
  LEDs_setup();
  //Set up fan
  fanMotor_setup();
  //Set up the vent potentiometer
}

void loop(){
  runStates();
  changeState();
  currentMillis = millis();
  
}

void changeState(){
  int time = millis();
  int chk = DHT.read11(DHT11_PIN);
  if(state != previousState){//Check for a state change
    displayStateChange();
    previousState = state;
  }
  if(((state =='i')||(state =='r')) && (getWaterValue()< WATER_THRESHOLD)){
    state = 'e';
  } 
  if((state == 'r') && (DHT.temperature<TEMP_THRESHOLD)){
    state = 'i';
  }
  if((state == 'i') && (DHT.temperature>TEMP_THRESHOLD)){
    state = 'r';
  }
}
void printEnvironment(){
  lcd.clear();
  int chk = DHT.read11(DHT11_PIN);
  float a = DHT.temperature;
  stringPrint("Temperature = ");
  printFloat(a);
  stringPrint("\n");
  stringPrint("Humidity = ");
  a = DHT.humidity;
  printFloat(a);
  stringPrint("\n");
  lcd.setCursor(0, 0); // move cursor to (0, 0)
  lcd.print("TEMP:"); // print message at (0, 0)
  lcd.setCursor(6, 0); // move cursor to (0, 0)
  lcd.print(DHT.temperature);
  lcd.setCursor(0, 1); // move cursor to (2, 1)
  lcd.print("HUMIDITY:"); // print message at (2, 1)
  lcd.setCursor(11, 1); // move cursor to (0, 0)
  lcd.print(DHT.humidity);

}

void runStates(){
  if(state == 'd'){
    fanOff();
    turnOnLED(YELLOW);
    ventMovement();
  }
  if(state == 'i'){
    fanOff();
    sendEnvInfo();
    turnOnLED(GREEN);
    ventMovement();
  }
  if(state == 'r'){
    fanOn();
    sendEnvInfo();
    turnOnLED(BLUE);
    ventMovement();
  }
  if(state == 'e'){
    turnOnLED(RED);
    displayErrorMessage();
    fanOff();
  }
}
void sendEnvInfo(){
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    printEnvironment();
  }
}
void ventMovement(){//Pin A0 will be the analog PIN
  if(adc_read(0)<VENT_LOWER_THRESHOLD){
      myStepper.setSpeed(10);
      myStepper.step(-stepsPerRevolution);
      displayMotorMovement();
  }
  if(VENT_UPPER_THRESHOLD<adc_read(0)){
      myStepper.setSpeed(10);
      myStepper.step(stepsPerRevolution);
      displayMotorMovement();
  }

}
void displayMotorMovement(){
  DateTime now = rtc.now();
  int data;
  stringPrint("The vent has changed at the following time: ");
  data = now.year(), DEC;
  printInt(data);
  stringPrint("/");
  data  = now.month(), DEC;
  printInt(data);
  stringPrint("/");
  data = now.day(), DEC;
  printInt(data);
  stringPrint(" (");
  stringPrint(daysOfTheWeek[now.dayOfTheWeek()]);
  stringPrint(") ");
  data = now.hour(), DEC;
  printInt(data);
  stringPrint(":");
  data =now.minute(), DEC;
  printInt(data);
  stringPrint(":");
  data = now.second(), DEC;
  printInt(data);
  stringPrint(" ");
  stringPrint("\n");
}
void displayStateChange(){
  int data;
  DateTime now = rtc.now();
  stringPrint(" ");
  stringPrint("\n");
  stringPrint("The state has changed at the following time: ");
  stringPrint("/");
  data = now.year(), DEC;
  printInt(data);
  stringPrint("/");
  data = now.month(), DEC;
  printInt(data);
  stringPrint("/");
  data = now.day(), DEC;
  printInt(data);
  stringPrint(" (");
  stringPrint(daysOfTheWeek[now.dayOfTheWeek()]);
  stringPrint(") ");
  data = now.hour(), DEC;
  printInt(data);
  stringPrint(":");
  data = now.minute(), DEC;
  printInt(data);
  stringPrint(":");
  data= now.second(), DEC;
  printInt(data);
  stringPrint(" ");
  stringPrint("\n");
}
void setToDisabled(){
  state = 'd';
  printErrorOnce = 0;
  lcd.clear();
}
void startCooler(){
  if(state == 'd'){
    state = 'i';
  }
}
void resetCooler(){
  if(state == 'e'){
    state = 'i';
    printErrorOnce = 0;
    lcd.clear();
  }
}
int getWaterValue(){
  int value = 0;
  *PORTC |= 0b00000010; //turn water sensor on
  my_delay(100);
  value = adc_read(WATER_SIGNAL);
  *PORTC &= 0b11111101; //turn water sensor off
  return value;
}
void displayErrorMessage(){
  if(printErrorOnce==0){
    lcd.clear();
    lcd.print("Water level low");
    printErrorOnce = 1;
  }
}
void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}
void my_delay(unsigned int freq)
{
  // calc period
  double period = 1.0/double(freq);
  // 50% duty cycle
  double half_period = period/ 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = (half_period/clk_period);
  // stop the timer
  *myTCCR1B &= ~0x07;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);

  * myTCCR1A = 0x0;
  // start the timer
  * myTCCR1B |= 0x01;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); 
  // stop the timer
  *myTCCR1B &= ~0x07;   
  // reset TOV           
  *myTIFR1 |= 0x01;
}
void stringPrint(String s){
  int  i =0;
  while(s[i] != '\0'){
    U0putchar(s[i]);
    i++;
  }
}
void stringPrintLn(String s){
  int  i = 0;
  while(s[i] != '\0'){
    U0putchar(s[i]);
    i++;
  }
  U0putchar('\n');
}
void printFloat(float a){
  int c = a;
  int d;
  d = c /1000;
  printChar(d);
  d = c /100;
  printChar(d);
  d = c / 10;
  printChar(d);
  d = c  % 1;
  printChar(d);
}
void printInt(int a){
  int d;
  d = a /1000;
  a = a - (d*1000);
  printChar(d);
  d = a /100;
  a = a - (d*100);
  printChar(d);
  d = a / 10;
  a = a - (d*10);
  printChar(d);
  d = a  / 1;
  a = a - (d);
  printChar(d);
}
void printChar(int a){
  if(a == 0){
    U0putchar('0');
  }
  if(a == 1){
    U0putchar('1');
  }
  if(a == 2){
    U0putchar('2');
  }
  if(a == 3){
    U0putchar('3');
  }
  if(a == 4){
    U0putchar('4');
  }
  if(a == 5){
    U0putchar('5');
  }
  if(a == 6){
    U0putchar('6');
  }
  if(a == 7){
    U0putchar('7');
  }
  if(a == 8){
    U0putchar('8');
  }
  if(a == 9){
    U0putchar('9');
  }
}
