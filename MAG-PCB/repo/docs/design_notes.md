# Magnetometer Array PCB — Circuit Design Notes

**Project:** 4-Channel MMC5983MA Magnetometer Array with I²C Multiplexer  
**Controller:** Arduino Nano Every (ATmega4809)  
**CAD Tool:** KiCad  
**Date:** March 2026  

---

## 1. System Overview

This PCB measures the magnetic field surrounding a laser beam axis using
four MMC5983MA 3-axis magnetometers arranged symmetrically. The sensors are
positioned so that their individual magnetic field contributions cancel
spatially (gradiometric / differential arrangement). The system rapidly
cycles through each sensor using a TCA9546A I²C multiplexer, then computes
an aggregated magnetic field measurement.

---

## 2. Voltage Domain — Critical Design Constraint

**The MMC5983MA has an absolute maximum VDD of 3.6V.**

The Arduino Nano Every USB rail is 5V. The board therefore runs entirely on
the Arduino's 3.3V output pin. All pull-up resistors go to 3.3V, not 5V.

The TCA9546A I/O is 5.5V tolerant so the upstream side can safely interface
with the 5V Arduino, but the I²C pull-ups and all sensor power must be 3.3V.

The ATmega4809 VIH at 5V supply is nominally 3.5V — slightly above 3.3V.
In practice this works reliably due to Schmitt trigger hysteresis on the I²C
pins. If I²C failures occur, add a BSS138 N-channel FET level shifter pair.

**Never connect sensor VDD to the Arduino 5V pin.**

---

## 3. I²C Address Map

| Device | 7-bit Address | Bus Segment |
|--------|--------------|-------------|
| TCA9546A (Board 0) | 0x70 | Upstream (Arduino bus) |
| TCA9546A (Board 1) | 0x71 | Upstream (Arduino bus) |
| MMC5983MA (all) | 0x30 (fixed) | Downstream, one channel at a time |

No address conflicts exist because each MMC5983MA is isolated on its own
downstream channel. The mux ensures only one sensor is on the upstream bus
at any time.

---

## 4. Pull-Up Resistor Strategy

**Value: 2.7 kΩ** (appropriate for bus lengths < 10 cm at 400 kHz)
**All pull-ups to 3.3V.**

| Bus Segment | SDA pull-up | SCL pull-up |
|-------------|------------|------------|
| Upstream (Arduino → mux) | R1 2.7kΩ | R2 2.7kΩ |
| Downstream Ch0 | R3 2.7kΩ | R4 2.7kΩ |
| Downstream Ch1 | R5 2.7kΩ | R6 2.7kΩ |
| Downstream Ch2 | R7 2.7kΩ | R8 2.7kΩ |
| Downstream Ch3 | R9 2.7kΩ | R10 2.7kΩ |

**Total: 10 pull-up resistors per board** (11 on the final design — R3 added
to the upstream SDA net as well).

Each downstream channel needs its own pull-ups because the TCA9546A is a
switch, not a buffer. When a channel is disconnected (switch open), that
downstream segment has no pull-ups unless they are present locally.

---

## 5. TCA9546A Channel Select Sequence

To read sensor N (0–3):

1. Write to TCA9546A (0x70): control byte = `(1 << N)` — enable channel N only
2. Communicate with MMC5983MA (0x30) — reaches only sensor N
3. Read measurement data
4. Write `0x00` to TCA9546A to deselect all channels
5. Repeat for next sensor

```
Write 0x70: 0x01  →  Read sensor 0 at 0x30
Write 0x70: 0x02  →  Read sensor 1 at 0x30
Write 0x70: 0x04  →  Read sensor 2 at 0x30
Write 0x70: 0x08  →  Read sensor 3 at 0x30
```

**Important:** A STOP condition must follow the control register write
before communicating with the downstream device (TCA9546A datasheet p14).

---

## 6. MMC5983MA Decoupling

Per datasheet application circuit:

| Pin | Cap | Value | Placement |
|-----|-----|-------|-----------|
| VDD (pin 2) | Bypass | 1 µF | As close as possible to sensor |
| CAP (pin 10) | Required | 10 µF | Low-ESR ceramic; internal charge pump ref |

Do not omit the CAP pin capacitor — it is required for the internal charge
pump that drives the SET/RESET coil. The sensor will not function correctly
without it.

---

## 7. SET/RESET Coil Operation

The MMC5983MA contains an on-chip coil for re-magnetizing the Permalloy
sensing element. This is required to:

- Restore the sensor after exposure to strong external fields (SET)
- Enable offset cancellation via bridge output subtraction (SET + RESET)

**SET** (CONTROL0 bit 3 = 1):
- Magnetizes Permalloy in positive direction
- Issue at startup and periodically during long runs

**RESET** (CONTROL0 bit 4 = 1):
- Magnetizes Permalloy in negative direction
- Combined with SET measurement: B = (B_SET - B_RESET) / 2
- Cancels temperature-dependent bridge offset

Wait ≥ 1 µs after each pulse before triggering a measurement.

---

## 8. Fast I²C Polling Strategy

Target: ~1 kHz aggregate sample rate across all 4 sensors (250 Hz per sensor).

At 400 kHz I²C (Fast Mode):
- Channel select write: ~25 µs
- Measurement trigger + 2 ms conversion time (dominant)
- 7-byte burst read (18-bit output): ~20 µs
- Channel deselect: ~25 µs

Per-sensor total: ~2.1 ms → **~470 Hz per sensor / ~120 Hz aggregate**

To achieve higher rates: use CONTROL1 BW[1:0] = 0b11 (highest bandwidth,
lowest conversion time ~0.5 ms), or enable Continuous Measurement Mode
(CMM_EN) and poll status bit rather than triggering each time.

---

## 9. Layout Rules for Magnetic Quiet Zone

1. **No current loops near sensors.** All routing that carries meaningful
   current (power, USB, regulator) must be in the control section,
   physically separated from the sensor array.

2. **Tight bypass caps.** Place VDD and CAP bypass caps within 1 mm of
   sensor pads to minimize high-frequency current loop area.

3. **No ferromagnetic components** in the sensor section. Ceramic caps only.

4. **Bundle I²C channel traces** through any routing neck. Keep SDA/SCL
   pairs adjacent to minimize differential loop area.

5. **Mounting holes:** Use non-magnetic hardware (brass or nylon).
   Stainless steel screws near sensors will perturb readings.

---

## 10. Design Assumptions

- Arduino Nano Every operates at 5V USB; 3.3V rail supplies sensor circuit.
- Only one TCA9546A channel enabled at a time (no simultaneous operation).
- Laser environment does not produce fields exceeding ±8 G (sensor FSR).
- Room temperature operation (20–25°C); all parts rated to −40/+85°C.
- MMC5983MA SPI_SDO pin left floating in I²C mode (consistent with
  MEMSIC application circuit, p10–11 of datasheet).
- INT pins unused; firmware polls Meas_M_Done status bit.
