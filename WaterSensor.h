#ifndef WATERSENSOR_H
#define WATERSENSOR_H

#define WATER_SIGNAL A1
#define WATER_POWER 36

void waterSensor_setup(){
	pinMode(WATER_POWER, OUTPUT);
	pinMode(18, INPUT);
	digitalWrite(WATER_POWER, LOW);
}

int getWaterValue(){
	int value;
	digitalWrite(WATER_POWER, HIGH);
	delay(10);
	value = analogRead(WATER_SIGNAL);
	digitalWrite(WATER_POWER, LOW);
	return value;
}

#endif