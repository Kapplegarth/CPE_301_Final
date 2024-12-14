//Kyle Applegarth
//Ryan Zheng
//Ronald Icely
//Adam Veilleaux

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>

#include "LEDs.h"
#include "WaterSensor.h"
#include "FanMotor.h"

//Complete the Real time Clock
//Does Github work at all
#define RDA 0x80
#define TBE 0x20
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

//Hopefully it does
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
#define WATER_THRESHOLD 100
int printErrorOnce = 0;
//Create Stepper Motorf
const int stepsPerRevolution = 2038;
//pin 32 IN1, Pin 34 IN2, IN3 33, Pin 35 IN4
Stepper myStepper = Stepper(stepsPerRevolution, 32,33,34,35);
//Create ISR for Start and Stop button and Reset
//Declare Variables
volatile char state;//d is for disabled, r for running, i for idle, and e for error
char previousState;//d is for disabled, r for running, i for idle, and e for error
double waterLevel = 0;

unsigned long previousMillis = 0;
const long interval = 60000;
unsigned long currentMillis = 0;
void setup()
{
  U0init(9600);
  lcd.begin(16, 2);
  //Set the Water sensor
  waterSensor_setup();
  rtc.begin();
  lcd.begin(16, 2); // set up number of columns and rows
  state = 'd';//state is disabled
  previousState = 'd';//Set the previous state
  //Set the ISR Stop Button pin 24
  pinMode(19, INPUT_PULLUP);
  attachInterrupt (digitalPinToInterrupt(19),setToDisabled,RISING);
  //Set the ISR start Button pin 23
  pinMode(18, INPUT_PULLUP);
  attachInterrupt (digitalPinToInterrupt(18),startCooler,RISING);
  //Set the ISR reset button to pin 25
  pinMode(3, INPUT_PULLUP);
  attachInterrupt (digitalPinToInterrupt(3),resetCooler,RISING);
  //Set up the lights
  LEDs_setup();
  //Set up DC Motor
  fanMotor_setup();
  //Set up the vent potentiometer
  pinMode(A0,INPUT);
  pinMode(23,INPUT);
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
  Serial.print("Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);
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
  if(analogRead(A0)<VENT_LOWER_THRESHOLD){
      myStepper.setSpeed(10);
      myStepper.step(-stepsPerRevolution);
      displayMotorMovement();
  }
  if(VENT_UPPER_THRESHOLD<analogRead(A0)){
      myStepper.setSpeed(10);
      myStepper.step(stepsPerRevolution);
      displayMotorMovement();
  }

}
void displayMotorMovement(){
  DateTime now = rtc.now();
  Serial.print("The vent has changed at the following time: ");
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println(" ");
}
void displayStateChange(){
  DateTime now = rtc.now();
  Serial.println(" ");
  Serial.print("The state has changed at the following time: ");
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println(" ");
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
