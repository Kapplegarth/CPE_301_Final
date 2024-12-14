#ifndef LEDS_H
#define LEDS_H

#define YELLOW 1 //pin 26, PA4
#define GREEN 2 //pin 27, PA5
#define BLUE 3 //pin 28, PA6
#define RED 4 //pin 29, PA7

volatile unsigned char* leds_portA = (unsigned char*) 0x22;
volatile unsigned char* leds_ddrA = (unsigned char*) 0x21;
//volatile unsigned char* leds_pinA = (unsigned char*) 0x20;

void LEDs_setup(){
	//set PA4 through PA7 to output
	*leds_ddrA |= 0b11110000;
}

void turnOnLED(int color){
	switch (color){
		case YELLOW:
			*leds_portA |= 0b00010000; //first turn on the port
			*leds_portA &= 0b00011111; //then turn off the other 3
			break;
		case GREEN:
			*leds_portA |= 0b00100000;
			*leds_portA &= 0b00101111;
			break;
		case BLUE:
			*leds_portA |= 0b01000000;
			*leds_portA &= 0b01001111;
			break;
		case RED:
			*leds_portA |= 0b10000000;
			*leds_portA &= 0b10001111;
			break;
	}
}

#endif