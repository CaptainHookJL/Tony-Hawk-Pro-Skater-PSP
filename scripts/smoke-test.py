#!/usr/bin/env python3
"""Run a bounded PSP integration session and preserve the emulator outcome."""
import os
from pathlib import Path
import re
import struct
import subprocess

root = Path(__file__).resolve().parent.parent
emulator = Path(os.environ.get(
    "PPSSPP_HEADLESS", "/workspace/.tools/ppsspp-build/PPSSPPHeadless"))
if not emulator.is_file():
    raise SystemExit("Set PPSSPP_HEADLESS to a built PPSSPPHeadless executable.")

outputs = root / "build/emulator"
outputs.mkdir(parents=True, exist_ok=True)
picture = outputs / "park.png"
picture.unlink(missing_ok=True)
command = [
    str(emulator), "--graphics=software", "--log", "--timeout-wall=40",
    "--timeout-emulated=10", f"--memstick={outputs / 'memstick'}",
    f"--root={root}", f"--screenshot-save={picture}",
    str(root / "build/psp-smoke/sidewalk_sessions.prx"),
]
try:
    run = subprocess.run(command, cwd=emulator.parent, capture_output=True,
                         text=True, timeout=50)
except subprocess.TimeoutExpired as error:
    (outputs / "emulator.log").write_bytes(
        (error.stdout or b"") + (error.stderr or b""))
    raise SystemExit("Emulator smoke test timed out; see build/emulator/emulator.log")

log = run.stdout + run.stderr
(outputs / "emulator.log").write_text(log)
if run.returncode:
    print(log[-6000:])
    raise SystemExit(run.returncode)
marker = re.search(r"SKATE_SMOKE_OK frames=300 score=(\d+) best=(\d+)", log)
if marker is None or "TIMEOUT" in log or "SKATE_SMOKE_FAIL" in log:
    print(log[-6000:])
    raise SystemExit("PSP gameplay session did not complete successfully.")
if not picture.is_file():
    raise SystemExit("Emulator completed without the required screenshot.")
header = picture.read_bytes()[:24]
if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
    raise SystemExit("Invalid emulator screenshot.")
width, height = struct.unpack_from(">II", header, 16)
if width not in (480, 512) or height != 272:
    raise SystemExit(f"Unexpected framebuffer dimensions: {width} x {height}")
print(f"PSP smoke passed: 300 frames, score {marker[1]}, best {marker[2]}.")
print(f"Framebuffer captured at {picture}; inspect it after renderer changes.")
