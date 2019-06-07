/*
 ultrasonic.cpp

 Demo/test  program for the MaxBotix Ultrasonic Rangefinder sensor

 Created on 28-Apr-2019

 Copyright (c) 2019 Philippe Vanhaesendonck
 
 This program and the accompanying materials are made
 available under the terms of the Eclipse Public License 2.0
 which is available at https://www.eclipse.org/legal/epl-2.0/
 
 SPDX-License-Identifier: EPL-2.0
 */
 
#include <Arduino.h>
#include <SoftwareSerialRead.h>

#ifndef SERIAL_DEBUG
#define SERIAL_DEBUG 9600
#endif

// Hardware configuration
const uint8_t sensorReadPin = 4;	// Serial output from the sensor
const uint8_t sensorEnablePin = A3;	// Pin driving the MOSFET to control power

SoftwareSerialRead ultrasonicSensor(sensorReadPin);

void setup() {
	delay(1000);
	Serial.begin(SERIAL_DEBUG);
	Serial.println(F("Getting sensor data..."));
	Serial.println(F("Time\tDistance"));

	ultrasonicSensor.begin(9600);

	pinMode(sensorEnablePin, OUTPUT);
	digitalWrite(sensorEnablePin, LOW);
}

// In "stream" mode, data available every 1.83 seconds.
// Set a timeout to ensure we don't stay forever in read loop, but long enough
// to capture at least a complete frame.
const unsigned long timeout = 3000;

/*
 * get_sensor_data()
 *
 * Return one sensor reading or zero in case of timeout or frame error
 *
 */
uint16_t get_sensor_data() {
	unsigned long start_time = millis();
	bool start_frame = false;
	uint16_t dist = 0;
	char p1 = '0';
	char p2 = '0';

	// Enable sensor
	digitalWrite(sensorEnablePin, HIGH);

	// Sensor starts with:
	//	SCXL-MaxSonarWRMT
	//	PN:MBnnnn
	//	Copyright 2011-2017
	//	MaxBotix Inc.
	//	RoHSv23b 068  0517
	//	TempI
	//	R1599
	//	R1600
	//	...
	// Lines are '\r’ terminated

	// Wait for the start of frame marker
	// "millis() - start_time < timeout" is rollover-safe!
	while (!start_frame && (millis() - start_time < timeout)) {
		if (ultrasonicSensor.available()) {
			char c = ultrasonicSensor.read();
			if (c == 'R' && p1 == '\r' && p2 != '.') {
				start_frame = true;
			} else {
				p2 = p1;
				p1 = c;
			}
		}
	}

	if (start_frame) {
		// Next 4 characters are the distance
		uint8_t i = 0;
		while (i < 4 && (millis() - start_time < timeout)) {
			if (ultrasonicSensor.available()) {
				char c = ultrasonicSensor.read();
				if ('0' <= c && c <= '9') {
					dist = dist * 10 + (c - '0');
					i++;
				} else {
					// Corrupted data
					i = 4;
					dist = 0;
					Serial.print(F("Data corruption: "));
					Serial.println(c);
				}
			}
		}
		if (i < 4){
			Serial.println(F("Timeout while reading frame"));
			dist = 0;
		}
	} else {
		Serial.println(F("Timeout while waiting for start frame"));
	}

	// Disable sensor
	digitalWrite(sensorEnablePin, LOW);
	return dist;
}

void loop() {
	uint16_t distance;

	unsigned long start = millis();
	distance = get_sensor_data();
	Serial.print(millis() - start);

	Serial.print('\t');
	Serial.println(distance);
	delay(10000);
}
