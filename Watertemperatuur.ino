/*******************************************************************************
 * DS18B20 Watertemperatuur via LoRaWAN (TTN - ABP)
 *
 * Sensor: DS18B20 waterproof temperatuursensor
 * Bedrading:
 *   Rood   -> 5V
 *   Zwart  -> GND
 *   Geel   -> Pin D3 (met 4.7k pull-up weerstand naar 5V)
 *
 * Payload (2 bytes, signed int16, temp * 100):
 *   TTN Decoder:
 *   function decodeUplink(input) {
 *     var temp = (input.bytes[0] << 8) | input.bytes[1];
 *     if (temp > 32767) temp -= 65536;
 *     return { data: { temperature: temp / 100.0 } };
 *   }
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_BUS = 3;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensoren(&oneWire);

// LoRaWAN ABP keys
static const PROGMEM u1_t NWKSKEY[16] = {0xA3, 0xDD, 0x08, 0xF9, 0xB6, 0xFA, 0x53, 0x78, 0x33, 0xC7, 0xA8, 0xF7, 0x82, 0xBF, 0xBD, 0xBE};
static const u1_t PROGMEM APPSKEY[16] = {0xEE, 0x19, 0x6A, 0x9C, 0x0B, 0x0D, 0x25, 0xA2, 0xCC, 0xC2, 0x76, 0x57, 0xD9, 0x34, 0xA4, 0xFA};
static const u4_t DEVADDR = 0x260B23B6;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t payload[2];
static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;

const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void leesTemperatuurEnVulPayload() {
    sensoren.requestTemperatures();
    float tempC = sensoren.getTempCByIndex(0);

    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println(F("FOUT: DS18B20 niet bereikbaar!"));
        tempC = -127.0;
    }

    Serial.print(F("Watertemperatuur: "));
    Serial.print(tempC, 2);
    Serial.println(F(" C"));

    int16_t tempInt = (int16_t)(tempC * 100);
    payload[0] = (uint8_t)(tempInt >> 8);
    payload[1] = (uint8_t)(tempInt & 0xFF);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            if (LMIC.txrxFlags & TXRX_ACK)
                Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
                Serial.print(F("Received "));
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes"));
            }
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        default:
            Serial.println(F("Event"));
            break;
    }
}

void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        leesTemperatuurEnVulPayload();
        LMIC_setTxData2(1, payload, sizeof(payload), 0);
        Serial.println(F("Packet queued"));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("DS18B20 Water Temp via LoRaWAN (ABP)"));

    sensoren.begin();
    Serial.print(F("Sensoren gevonden: "));
    Serial.println(sensoren.getDeviceCount());

    #ifdef VCC_ENABLE
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    os_init();
    LMIC_reset();

    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);
    #elif defined(CFG_us915)
    LMIC_selectSubBand(1);
    #endif

    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF7, 14);

    do_send(&sendjob);
}

unsigned long vorigeLezing = 0;
const unsigned long LEES_INTERVAL = 2000;

void loop() {
    os_runloop_once();

    if (millis() - vorigeLezing >= LEES_INTERVAL) {
        vorigeLezing = millis();
        sensoren.requestTemperatures();
        float tempC = sensoren.getTempCByIndex(0);
        Serial.print(F("Live temp: "));
        Serial.print(tempC, 2);
        Serial.println(F(" C"));
    }
}
