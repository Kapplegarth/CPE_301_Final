#ifndef FANMOTOR_H
#define FANMOTOR_H

//direction 1 is port 30, PC7
//direction 2 is port 31, PC6

volatile unsigned char* fanMotor_portC = (unsigned char*) 0x28;
volatile unsigned char* fanMotor_ddrC = (unsigned char*) 0x27;
//volatile unsigned char* fanMotor_pinC = (unsigned char*) 0x26;

void fanMotor_setup(){
	//set PC7 and PC6 to output
	*fanMotor_ddrC |= 0b11000000;
}

void fanOn(){
	//keep direction 2 off; turn on direction 1
	*fanMotor_portC &= 0b10111111;
	*fanMotor_portC |= 0b10000000;
}

void fanOff(){
	//turn off both directions
	*fanMotor_portC &= 0b00111111;
}

#endif