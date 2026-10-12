#!/usr/bin/env python3
"""Package the generated homebrew and its instructions for copying to a PSP."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).resolve().parent.parent
eboot = root / "dist/PSP/GAME/SkateLab/EBOOT.PBP"
with eboot.open("rb") as stream:
    if stream.read(4) != b"\x00PBP":
        raise SystemExit("Expected a built PSP PBP container; run make package.")

archive = root / "dist/SkateLab-PSP.zip"
with ZipFile(archive, "w", compression=ZIP_DEFLATED) as bundle:
    bundle.write(eboot, "PSP/GAME/SkateLab/EBOOT.PBP")
    bundle.write(root / "README.md", "README.md")
    bundle.write(root / "docs/PORTING.md", "docs/PORTING.md")
    picture = root / "docs/images/park.png"
    if picture.is_file():
        bundle.write(picture, "docs/images/park.png")
print(f"Packaged {archive}")
