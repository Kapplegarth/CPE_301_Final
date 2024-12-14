#ifndef FANMOTOR_H
#define FANMOTOR_H

//Set UP DC Motor
#define dir1 30
#define dir2 31

void fanMotor_setup(){
	pinMode(dir1,OUTPUT);
	pinMode(dir2,OUTPUT);
}

void fanOn(){
	digitalWrite(dir1,HIGH);
	digitalWrite(dir2,LOW);
}

void fanOff(){
	digitalWrite(dir1,LOW);
	digitalWrite(dir2,LOW);
}

#endif