# Gerbers

Generated from KiCad 10.0.1 on 2026-04-27.

## Files

| File | Layer | Function |
|------|-------|----------|
| `MAG PCB FINAL-F_Cu.gbr` | F_Cu (L1) | Top copper — all signal routing |
| `MAG PCB FINAL-B_Cu.gbr` | B_Cu (L2) | Bottom copper — ground/return paths |
| `MAG PCB FINAL-F_Paste.gbr` | F_Paste | Top solder paste (SMT stencil) |
| `MAG PCB FINAL-B_Paste.gbr` | B_Paste | Bottom solder paste |
| `MAG PCB FINAL-F_Silkscreen.gbr` | F_Silkscreen | Top component labels |
| `MAG PCB FINAL-B_Silkscreen.gbr` | B_Silkscreen | Bottom labels |
| `MAG PCB FINAL-F_Mask.gbr` | F_Mask | Top solder mask openings |
| `MAG PCB FINAL-B_Mask.gbr` | B_Mask | Bottom solder mask openings |
| `MAG PCB FINAL-Edge_Cuts.gbr` | Edge_Cuts | Board outline / routed profile |
| `MAG PCB FINAL-job.gbrjob` | — | Gerber job file (submit to fab) |

## Fab Specs (from job file)

- Board size: **50.05 mm × 120.05 mm**
- Layers: **2**
- Thickness: **1.6 mm**
- Min trace: **0.3 mm**
- Pad-to-pad: **0.2 mm**

## Ordering

Submit all `.gbr` files plus the `.gbrjob` to your PCB fabricator
(JLCPCB, PCBWay, OSH Park, etc.). The job file encodes all specifications
automatically.

For two-board assembly: order **2× quantity minimum**. Both boards use
identical Gerbers; the only hardware difference is the A0 address strap
on the TCA9546A (see `docs/magnetometer_report.pdf` Section 6).
