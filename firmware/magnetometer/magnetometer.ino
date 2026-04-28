/*
 * magnetometer.ino
 *
 * Multi-sensor magnetometer firmware for the MAG PCB FINAL board.
 * Reads all 8 MMC5983MA sensors (4 per board, 2 boards) via two
 * TCA9546A I2C multiplexers and streams CSV over USB-serial at 115200 baud.
 *
 * Hardware:
 *   Arduino Nano Every (ATmega4809)
 *   Board 0: TCA9546A @ 0x70, sensors 0-3
 *   Board 1: TCA9546A @ 0x71, sensors 4-7
 *
 * Board settings (Arduino IDE):
 *   Board:                Arduino Nano Every
 *   Registers Emulation:  None (ATMEGA4809)
 *   Baud:                 115200
 */

#include <Wire.h>
#include "mmc5983ma.h"

// ── Configuration ──────────────────────────────────────────────────────────
#define MUX0_ADDR   0x70        // Board 0: A0=GND
#define MUX1_ADDR   0x71        // Board 1: A0=VCC (requires A0 rework)
#define RST_PIN     4           // Arduino GPIO → RST net on FFC connector
#define I2C_SPEED   400000UL    // 400 kHz Fast Mode
#define LOOP_DELAY_MS 100       // ~10 Hz per full 8-sensor sweep

// Set to true to use SET/RESET offset cancellation on every reading.
// More accurate but takes ~2x as long per sensor.
#define USE_OFFSET_CANCEL  false

// ── Globals ────────────────────────────────────────────────────────────────
MMC5983MA sensor;

// ── Mux helpers ───────────────────────────────────────────────────────────

void selectChannel(uint8_t muxAddr, uint8_t channel) {
    Wire.beginTransmission(muxAddr);
    Wire.write((uint8_t)(1 << channel));
    Wire.endTransmission();
}

void deselectAll(uint8_t muxAddr) {
    Wire.beginTransmission(muxAddr);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission();
}

void resetMuxes() {
    digitalWrite(RST_PIN, LOW);
    delayMicroseconds(100);
    digitalWrite(RST_PIN, HIGH);
    delay(1);
}

// ── I2C scanner (for bring-up / debug) ────────────────────────────────────

void i2cScan() {
    Serial.println("# I2C scan:");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("#   0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("# Scan complete.");
}

void scanMuxChannels(uint8_t muxAddr) {
    Serial.print("# Scanning mux 0x");
    Serial.println(muxAddr, HEX);
    for (uint8_t ch = 0; ch < 4; ch++) {
        selectChannel(muxAddr, ch);
        Wire.beginTransmission(MMC5983MA_ADDR);
        uint8_t err = Wire.endTransmission();
        Serial.print("#   ch");
        Serial.print(ch);
        Serial.print(": MMC5983MA @ 0x30 -> ");
        Serial.println(err == 0 ? "OK" : "NOT FOUND");
        deselectAll(muxAddr);
    }
}

// ── Read one sensor ────────────────────────────────────────────────────────

bool readSensor(uint8_t muxAddr, uint8_t channel,
                float &Bx, float &By, float &Bz) {
    selectChannel(muxAddr, channel);
    bool ok;
    if (USE_OFFSET_CANCEL) {
        ok = sensor.readWithOffsetCancel(Bx, By, Bz);
    } else {
        uint32_t rx, ry, rz;
        ok = sensor.readRaw(rx, ry, rz);
        if (ok) {
            Bx = MMC5983MA::rawToGauss(rx);
            By = MMC5983MA::rawToGauss(ry);
            Bz = MMC5983MA::rawToGauss(rz);
        }
    }
    deselectAll(muxAddr);
    return ok;
}

// ── Setup ──────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, HIGH);

    Wire.begin();
    Wire.setClock(I2C_SPEED);

    resetMuxes();
    delay(10);

    // Bring-up diagnostics — comment out after validation
    i2cScan();
    scanMuxChannels(MUX0_ADDR);
    scanMuxChannels(MUX1_ADDR);

    // Issue SET on all 8 sensors at startup
    for (uint8_t ch = 0; ch < 4; ch++) {
        selectChannel(MUX0_ADDR, ch);
        sensor.doSet();
        deselectAll(MUX0_ADDR);

        selectChannel(MUX1_ADDR, ch);
        sensor.doSet();
        deselectAll(MUX1_ADDR);
    }

    // CSV header
    Serial.println("sensorID,Bx_G,By_G,Bz_G");
}

// ── Main loop ──────────────────────────────────────────────────────────────

void loop() {
    float Bx, By, Bz;

    // Board 0 — sensors 0-3
    for (uint8_t ch = 0; ch < 4; ch++) {
        if (readSensor(MUX0_ADDR, ch, Bx, By, Bz)) {
            Serial.print(ch);
            Serial.print(',');
            Serial.print(Bx, 4);
            Serial.print(',');
            Serial.print(By, 4);
            Serial.print(',');
            Serial.println(Bz, 4);
        } else {
            Serial.print(ch);
            Serial.println(",ERR,ERR,ERR");
        }
    }

    // Board 1 — sensors 4-7
    for (uint8_t ch = 0; ch < 4; ch++) {
        if (readSensor(MUX1_ADDR, ch, Bx, By, Bz)) {
            Serial.print(ch + 4);
            Serial.print(',');
            Serial.print(Bx, 4);
            Serial.print(',');
            Serial.print(By, 4);
            Serial.print(',');
            Serial.println(Bz, 4);
        } else {
            Serial.print(ch + 4);
            Serial.println(",ERR,ERR,ERR");
        }
    }

    delay(LOOP_DELAY_MS);
}
