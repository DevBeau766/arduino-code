#ifndef WATERTEMPERATUUR_SENSOR_H
#define WATERTEMPERATUUR_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class WatertemperatuurSensor {
private:
    OneWire oneWire;
    DallasTemperature sensoren;

public:
    WatertemperatuurSensor(int pin);
    void begin();
    float read();
};

#endif
