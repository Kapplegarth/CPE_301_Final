#ifndef LEDS_H
#define LEDS_H

#define YELLOW 1
#define GREEN 2
#define BLUE 3
#define RED 4

void LEDs_setup(){
	pinMode(26,OUTPUT);//pin 26 is yellow
	pinMode(27,OUTPUT);//pin 27 is green
	pinMode(28,OUTPUT);//pin 28 is blue
	pinMode(29,OUTPUT);//Pin 29 is Red
}

void turnOnLED(int color){
	switch (color){
		case YELLOW:
			digitalWrite(26, HIGH);
			digitalWrite(27, LOW);
			digitalWrite(28, LOW);
			digitalWrite(29, LOW);
			break;
		case GREEN:
			digitalWrite(26, LOW);
			digitalWrite(27, HIGH);
			digitalWrite(28, LOW);
			digitalWrite(29, LOW);
			break;
		case BLUE:
			digitalWrite(26, LOW);
			digitalWrite(27, LOW);
			digitalWrite(28, HIGH);
			digitalWrite(29, LOW);
			break;
		case RED:
			digitalWrite(26, LOW);
			digitalWrite(27, LOW);
			digitalWrite(28, LOW);
			digitalWrite(29, HIGH);
			break;
	}
}

#endif