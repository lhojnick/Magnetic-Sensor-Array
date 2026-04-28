# MAG PCB FINAL — Multi-Sensor Magnetometer Board

**Lab:** Coherent and Quantum Optics Lab, Purdue University  
**PI:** Dr. Elliott  
**Designer:** Luke Hojnicki  
**Primary User:** Chunya Wang  
**KiCad Version:** 10.0.1  
**Status:** Hardware validated, two-board system operational

---

## Overview

This repository contains everything needed to reproduce, operate, and extend the
multi-sensor magnetometer PCB developed for Dr. Elliott's quantum optics lab.

The system measures the 3-axis magnetic field vector (Bx, By, Bz) at **8 spatial
locations simultaneously** using two identical PCBs connected in series via FFC cable.
Each board carries four MMC5983MA AMR magnetometers arranged around a 30 mm radius
circular cutout, allowing the boards to be mounted concentrically around a cylindrical
experimental object (optical tube, glass cell, etc.).

```
Arduino Nano Every
       │
       ├── SDA / SCL / RST / 5V / 3.3V
       │
       ├── Board 0 ─ TCA9546A (0x70) ─┬── MMC5983MA #0  (channel 0)
       │              LM3940 LDO       ├── MMC5983MA #1  (channel 1)
       │                               ├── MMC5983MA #2  (channel 2)
       │                               └── MMC5983MA #3  (channel 3)
       │
       └── Board 1 ─ TCA9546A (0x71) ─┬── MMC5983MA #4  (channel 0)
                      LM3940 LDO       ├── MMC5983MA #5  (channel 1)
                                       ├── MMC5983MA #6  (channel 2)
                                       └── MMC5983MA #7  (channel 3)
```

---

## Repository Structure

```
MAG-PCB/
├── README.md                          ← this file
├── .gitignore
│
├── docs/
│   ├── magnetometer_report.tex        ← full lab report (LaTeX source)
│   ├── magnetometer_report.pdf        ← compiled report
│   └── design_notes.md               ← original circuit design document
│
├── firmware/
│   └── magnetometer/
│       ├── magnetometer.ino           ← main Arduino sketch
│       ├── mmc5983ma.h                ← MMC5983MA driver header
│       └── mmc5983ma.cpp              ← MMC5983MA driver implementation
│
├── python/
│   ├── logger.py                      ← USB-serial data logger (saves CSV)
│   └── requirements.txt
│
└── gerbers/
    ├── MAG PCB FINAL-F_Cu.gbr
    ├── MAG PCB FINAL-B_Cu.gbr
    ├── MAG PCB FINAL-F_Mask.gbr
    ├── MAG PCB FINAL-B_Mask.gbr
    ├── MAG PCB FINAL-F_Paste.gbr
    ├── MAG PCB FINAL-B_Paste.gbr
    ├── MAG PCB FINAL-F_Silkscreen.gbr
    ├── MAG PCB FINAL-B_Silkscreen.gbr
    ├── MAG PCB FINAL-Edge_Cuts.gbr
    └── MAG PCB FINAL-job.gbrjob       ← submit this to fab with all .gbr files
```

---

## Quick Start

### Hardware Setup

1. **Single board:** Connect the Arduino Nano Every to Board 0 via FFC cable.
   Confirm mux address `0x70` with the I2C scanner sketch.

2. **Two boards:** Connect Board 0 PIN_OUT → Board 1 PIN_IN with a
   **Type-B FFC cable** (Top-on-One-Side, Bottom-on-the-Other).
   Board 1 requires the A0 pin on its TCA9546A moved from GND → VCC
   to assign it address `0x71`. See the report (Section 6) for the
   rework procedure.

3. Power-on checklist:
   - Verify 3.3V at LDO output before connecting Arduino
   - Run I2C scanner — expect `0x70` (Board 0) and `0x71` (Board 1)
   - Scan each mux channel — expect `0x30` on all 8 channels

### Firmware

Open `firmware/magnetometer/magnetometer.ino` in Arduino IDE 2.x.

- Board: **Arduino Nano Every** (megaAVR Boards package)
- Registers Emulation: **None (ATMEGA4809)**
- Baud: **115200**

Upload and open Serial Monitor. You should see CSV output:
```
sensorID,Bx_G,By_G,Bz_G
0,0.2341,0.0123,-0.4412
...
7,0.2344,0.0125,-0.4415
```

### Data Logging (Python)

```bash
pip install -r python/requirements.txt
python python/logger.py --port COM3 --output data.csv
# Linux/Mac: --port /dev/ttyACM0
```

---

## Board Specifications

| Parameter | Value |
|---|---|
| Dimensions | 50.05 mm × 120.05 mm |
| Layer count | 2 (F_Cu / B_Cu) |
| Board thickness | 1.6 mm |
| Min trace width | 0.3 mm |
| Pad-to-pad clearance | 0.2 mm |
| KiCad version | 10.0.1 |

## I2C Address Map

| Device | Address | Notes |
|---|---|---|
| TCA9546A — Board 0 | `0x70` | A0 = GND (default) |
| TCA9546A — Board 1 | `0x71` | A0 = VCC (requires rework) |
| MMC5983MA (all) | `0x30` | Fixed; isolated per mux channel |

---

## Key Components

| Part | Function | Qty/board |
|---|---|---|
| MMC5983MA | 3-axis AMR magnetometer | 4 |
| TCA9546AD | 4-channel I2C mux | 1 |
| LM3940IMPX | 5V → 3.3V LDO | 1 |
| 2.7 kΩ resistors | I2C pull-ups | 11 |
| 1 µF ceramic caps | Sensor VDD bypass | 4 |
| 10 µF ceramic caps | Sensor CAP pin / mux VCC | 5 |
| 33 µF cap | LDO output | 1 |

---

## Known Issues / Future Work

- INT pin unused — firmware polls Meas_M_Done bit (adequate for ≤10 Hz)
- No on-chip temperature compensation — repeat SET/RESET for long runs
- FFC cable length sensitivity — reduce I2C to 100 kHz for cables > 30 cm
- Max 8 boards (32 sensors) before TCA9546A address space exhausted

---

## References

- MEMSIC MMC5983MA Datasheet, Rev. A
- Texas Instruments TCA9546A Datasheet, SCPS204
- Texas Instruments LM3940 Datasheet, SNOS386
- Arduino Nano Every: https://docs.arduino.cc/hardware/nano-every
