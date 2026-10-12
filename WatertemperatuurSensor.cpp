#include "WatertemperatuurSensor.h"

WatertemperatuurSensor::WatertemperatuurSensor(int pin)
    : oneWire(pin), sensoren(&oneWire) {
}

void WatertemperatuurSensor::begin() {
    sensoren.begin();
}

float WatertemperatuurSensor::read() {
    sensoren.requestTemperatures();
    float tempC = sensoren.getTempCByIndex(0);

    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println(F("FOUT: DS18B20 niet bereikbaar!"));
        return -127.0;
    }

    Serial.print(F("Watertemperatuur: "));
    Serial.print(tempC, 2);
    Serial.println(F(" C"));

    return tempC;
}
