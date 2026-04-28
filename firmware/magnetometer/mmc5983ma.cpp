#include "mmc5983ma.h"

// ── Public ─────────────────────────────────────────────────────────────────

bool MMC5983MA::begin() {
    uint8_t id = readReg(REG_PRODUCT_ID);
    return (id == 0x30);
}

void MMC5983MA::doSet() {
    writeReg(REG_CONTROL0, CTRL0_DO_SET);
    delayMicroseconds(1);   // SET pulse duration ~500 ns; wait ≥1 µs
}

void MMC5983MA::doReset() {
    writeReg(REG_CONTROL0, CTRL0_DO_RESET);
    delayMicroseconds(1);
}

bool MMC5983MA::readRaw(uint32_t &rawX, uint32_t &rawY, uint32_t &rawZ) {
    // Trigger measurement
    writeReg(REG_CONTROL0, CTRL0_TM_M);

    // Wait for completion
    if (!waitMeasDone()) return false;

    // Burst-read 7 registers (0x00–0x06)
    Wire.beginTransmission(MMC5983MA_ADDR);
    Wire.write(REG_XOUT0);
    Wire.endTransmission(false);   // repeated start
    Wire.requestFrom((uint8_t)MMC5983MA_ADDR, (uint8_t)7);

    uint8_t buf[7];
    for (int i = 0; i < 7 && Wire.available(); i++)
        buf[i] = Wire.read();

    // Assemble 18-bit values
    // buf[0] = Xout[17:10], buf[1] = Xout[9:2], buf[6] bits[7:6] = Xout[1:0]
    rawX = ((uint32_t)buf[0] << 10) |
           ((uint32_t)buf[1] << 2)  |
           ((buf[6] >> 6) & 0x03);

    rawY = ((uint32_t)buf[2] << 10) |
           ((uint32_t)buf[3] << 2)  |
           ((buf[6] >> 4) & 0x03);

    rawZ = ((uint32_t)buf[4] << 10) |
           ((uint32_t)buf[5] << 2)  |
           ((buf[6] >> 2) & 0x03);

    return true;
}

bool MMC5983MA::readWithOffsetCancel(float &Bx_G, float &By_G, float &Bz_G) {
    uint32_t rawXs, rawYs, rawZs;
    uint32_t rawXr, rawYr, rawZr;

    doSet();
    if (!readRaw(rawXs, rawYs, rawZs)) return false;

    doReset();
    if (!readRaw(rawXr, rawYr, rawZr)) return false;

    // B = (B_set - B_reset) / 2
    // Using signed arithmetic on the offset-removed values
    Bx_G = (rawToGauss(rawXs) - rawToGauss(rawXr)) / 2.0f;
    By_G = (rawToGauss(rawYs) - rawToGauss(rawYr)) / 2.0f;
    Bz_G = (rawToGauss(rawZs) - rawToGauss(rawZr)) / 2.0f;

    return true;
}

float MMC5983MA::rawToGauss(uint32_t raw) {
    return ((float)raw - (float)MMC_ZERO_OFFSET) / MMC_COUNTS_PER_G;
}

// ── Private ────────────────────────────────────────────────────────────────

void MMC5983MA::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MMC5983MA_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t MMC5983MA::readReg(uint8_t reg) {
    Wire.beginTransmission(MMC5983MA_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MMC5983MA_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

bool MMC5983MA::waitMeasDone() {
    uint32_t t0 = millis();
    while (!(readReg(REG_STATUS) & STATUS_MEAS_DONE)) {
        if (millis() - t0 > MMC_MEAS_TIMEOUT_MS) return false;
    }
    return true;
}
