#!/usr/bin/env python3
"""
logger.py — USB-serial data logger for the MAG PCB magnetometer system.

Usage:
    python logger.py --port COM3 --output data.csv
    python logger.py --port /dev/ttyACM0 --output data.csv --duration 60

Output CSV columns:
    time_s, sensorID, Bx_G, By_G, Bz_G

Requirements:
    pip install pyserial pandas
"""

import argparse
import csv
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)


def find_arduino_port():
    """Try to auto-detect the Arduino Nano Every serial port."""
    import serial.tools.list_ports
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "Arduino" in (p.manufacturer or "") or "ttyACM" in p.device:
            return p.device
    return None


def parse_args():
    parser = argparse.ArgumentParser(
        description="Log magnetometer CSV data from the MAG PCB board."
    )
    parser.add_argument(
        "--port", "-p",
        help="Serial port (e.g. COM3 or /dev/ttyACM0). Auto-detected if omitted."
    )
    parser.add_argument(
        "--baud", "-b",
        type=int, default=115200,
        help="Baud rate (default: 115200)"
    )
    parser.add_argument(
        "--output", "-o",
        default=f"data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
        help="Output CSV filename (default: timestamped)"
    )
    parser.add_argument(
        "--duration", "-d",
        type=float, default=None,
        help="Recording duration in seconds (default: run until Ctrl-C)"
    )
    parser.add_argument(
        "--sensors", "-s",
        type=int, default=8,
        help="Expected number of sensors (default: 8). Used for progress display."
    )
    parser.add_argument(
        "--no-warmup",
        action="store_true",
        help="Skip the 5-second warmup delay"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Resolve port
    port = args.port or find_arduino_port()
    if port is None:
        print("ERROR: Could not auto-detect Arduino port. Specify with --port.")
        sys.exit(1)
    print(f"Connecting to {port} at {args.baud} baud...")

    try:
        ser = serial.Serial(port, args.baud, timeout=2)
    except serial.SerialException as e:
        print(f"ERROR: Could not open port: {e}")
        sys.exit(1)

    # Flush startup messages
    time.sleep(2)
    ser.reset_input_buffer()

    if not args.no_warmup:
        print("Warming up for 5 seconds...")
        time.sleep(5)
        ser.reset_input_buffer()

    output_path = Path(args.output)
    print(f"Logging to {output_path}")
    print("Press Ctrl-C to stop.\n")

    t_start = time.time()
    row_count = 0
    error_count = 0

    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["time_s", "sensorID", "Bx_G", "By_G", "Bz_G"])

        try:
            while True:
                # Check duration limit
                elapsed = time.time() - t_start
                if args.duration and elapsed >= args.duration:
                    print(f"\nDuration limit reached ({args.duration} s).")
                    break

                raw = ser.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8", errors="replace").strip()

                # Skip comments and header
                if not line or line.startswith("#") or line.startswith("sensor"):
                    print(f"  [{line}]")
                    continue

                parts = line.split(",")
                if len(parts) != 4:
                    continue

                sensor_id = parts[0]

                # Error rows from firmware
                if "ERR" in line:
                    error_count += 1
                    print(f"  SENSOR {sensor_id} ERROR (total errors: {error_count})")
                    continue

                try:
                    Bx = float(parts[1])
                    By = float(parts[2])
                    Bz = float(parts[3])
                except ValueError:
                    continue

                t_rel = round(time.time() - t_start, 4)
                writer.writerow([t_rel, sensor_id, Bx, By, Bz])
                f.flush()
                row_count += 1

                B_mag = (Bx**2 + By**2 + Bz**2) ** 0.5
                print(
                    f"  t={t_rel:8.3f}s  id={sensor_id}  "
                    f"Bx={Bx:+.4f}  By={By:+.4f}  Bz={Bz:+.4f}  "
                    f"|B|={B_mag:.4f} G"
                )

        except KeyboardInterrupt:
            print("\nStopped by user.")

    ser.close()
    elapsed = time.time() - t_start
    print(f"\nSaved {row_count} rows to {output_path}")
    print(f"Elapsed: {elapsed:.1f} s  |  Errors: {error_count}")

    # Quick sanity check
    if row_count > 0:
        _check_sensors(output_path, args.sensors)


def _check_sensors(csv_path: Path, expected_sensors: int):
    """Print a quick per-sensor summary after logging."""
    try:
        import pandas as pd
        df = pd.read_csv(csv_path)
        print("\nPer-sensor summary:")
        for sid in sorted(df["sensorID"].unique()):
            sub = df[df["sensorID"] == sid]
            B_mag = (sub["Bx_G"]**2 + sub["By_G"]**2 + sub["Bz_G"]**2) ** 0.5
            print(
                f"  Sensor {sid}: n={len(sub):4d}  "
                f"|B| mean={B_mag.mean():.4f} G  std={B_mag.std():.5f} G"
            )
        found = df["sensorID"].nunique()
        if found < expected_sensors:
            print(f"\nWARNING: only {found}/{expected_sensors} sensors in data.")
    except ImportError:
        pass   # pandas optional — skip summary


if __name__ == "__main__":
    main()
