#!/usr/bin/env python3
import subprocess
import sys

# Path to your ELF file
ELF = "build/firmware.elf"

# Call nm to get symbols
try:
    output = subprocess.check_output(["arm-none-eabi-nm", ELF], text=True)
except subprocess.CalledProcessError:
    print("Error: Failed to run nm on ELF file")
    sys.exit(1)

# Find _SEGGER_RTT symbol
addr = None
for line in output.splitlines():
    if "_SEGGER_RTT" in line:
        addr = line.split()[0]
        break

if addr is None:
    print("Error: _SEGGER_RTT symbol not found")
    sys.exit(1)

print(f"Found _SEGGER_RTT at 0x{addr}")

# Call pyocd rtt
subprocess.run(["python", "-m", "pyocd", "rtt", "-a", f"0x{addr}"])