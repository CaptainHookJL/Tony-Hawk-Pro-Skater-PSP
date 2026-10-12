#!/usr/bin/env python3
"""Package the generated homebrew and its instructions for copying to a PSP."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).resolve().parent.parent
eboot = root / "dist/PSP/GAME/SidewalkSessions/EBOOT.PBP"
with eboot.open("rb") as stream:
    if stream.read(4) != b"\x00PBP":
        raise SystemExit("Expected a built PSP PBP container; run make package.")

archive = root / "dist/SidewalkSessions-PSP.zip"
with ZipFile(archive, "w", compression=ZIP_DEFLATED) as bundle:
    bundle.write(eboot, "PSP/GAME/SidewalkSessions/EBOOT.PBP")
    bundle.write(root / "README.md", "README.md")
    bundle.write(root / "docs/GAME_DESIGN.md", "docs/GAME_DESIGN.md")
    for name in ("menu", "park", "results"):
        picture = root / f"docs/images/{name}.png"
        if picture.is_file():
            bundle.write(picture, f"docs/images/{name}.png")
print(f"Packaged {archive}")
