#pragma once
#include <Arduino.h>
#include <Wire.h>

// ── MMC5983MA I2C address (fixed) ─────────────────────────────────────────
#define MMC5983MA_ADDR     0x30

// ── Register map ──────────────────────────────────────────────────────────
#define REG_XOUT0          0x00   // X[17:10]
#define REG_XOUT1          0x01   // X[9:2]
#define REG_YOUT0          0x02   // Y[17:10]
#define REG_YOUT1          0x03   // Y[9:2]
#define REG_ZOUT0          0x04   // Z[17:10]
#define REG_ZOUT1          0x05   // Z[9:2]
#define REG_XYZOUT2        0x06   // X[1:0] Y[1:0] Z[1:0]
#define REG_STATUS         0x08   // bit 0: Meas_M_Done, bit 1: Meas_T_Done
#define REG_CONTROL0       0x09   // TM_M, TM_T, START, SET, RESET, ...
#define REG_CONTROL1       0x0A   // BW[1:0], X/Y/Z_inhibit
#define REG_CONTROL2       0x0B   // PRD_SET, EN_PRD_SET, CMM_EN, ...
#define REG_PRODUCT_ID     0x2F   // should read 0x30

// ── CONTROL0 bit masks ────────────────────────────────────────────────────
#define CTRL0_TM_M         0x01   // trigger single magnetic measurement
#define CTRL0_TM_T         0x02   // trigger single temperature measurement
#define CTRL0_START        0x04   // start continuous mode
#define CTRL0_DO_SET       0x08   // perform SET
#define CTRL0_DO_RESET     0x10   // perform RESET

// ── STATUS bit masks ──────────────────────────────────────────────────────
#define STATUS_MEAS_DONE   0x01

// ── Conversion constants ──────────────────────────────────────────────────
// 18-bit output, zero-field = 2^17 = 131072
// Full-scale ±8 G spans 2^18 = 262144 counts => 16 G / 262144 = 0.0000610 G/LSB
#define MMC_ZERO_OFFSET    131072UL
#define MMC_COUNTS_PER_G   16384.0f   // 262144 / 16

// ── Timeout ───────────────────────────────────────────────────────────────
#define MMC_MEAS_TIMEOUT_MS   10

class MMC5983MA {
public:
    // Verify device is present and responding (checks product ID register)
    bool begin();

    // Issue a SET pulse (re-magnetize Permalloy in positive direction)
    void doSet();

    // Issue a RESET pulse (re-magnetize Permalloy in negative direction)
    void doReset();

    // SET/RESET offset cancellation: returns (B_set - B_reset) / 2
    // Returns false if measurement times out
    bool readWithOffsetCancel(float &Bx_G, float &By_G, float &Bz_G);

    // Trigger single measurement and read raw 18-bit values
    // Returns false if measurement times out
    bool readRaw(uint32_t &rawX, uint32_t &rawY, uint32_t &rawZ);

    // Convert raw 18-bit counts to Gauss
    static float rawToGauss(uint32_t raw);

private:
    void     writeReg(uint8_t reg, uint8_t val);
    uint8_t  readReg(uint8_t reg);
    bool     waitMeasDone();
};
