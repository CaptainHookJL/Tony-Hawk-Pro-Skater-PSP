# Toward Tony Hawk's Underground on PSP

## Starting point

The available material is a PS2 disc or game files; no buildable engine source
has been supplied. The prototype in this repository is a way to establish a
PSP build and explore mechanics. It does not load the PS2 game or reproduce it.

The target copy is the **USA PS2 edition**. The precise disc revision and file
listing have not yet been supplied; record them before researching formats.

A PS2 executable targets the Emotion Engine and PS2 hardware. PSP uses Allegrex,
a different vector unit, and a different graphics API. Copying the executable
into an EBOOT or reducing texture sizes cannot perform an engine port.

Tony Hawk's Underground 2 Remix is an existing PSP title and can serve as a
reference for how a related game fits PSP controls and resources. It is a
different game; its existence does not supply the source for Underground.

## First useful step with the disc files

Provide a directory listing, the game region/version, the names and sizes of
the large data files, and the contents of `SYSTEM.CNF`. Keep the original files
unchanged. A listing is enough to start identifying executable and archive
formats; there is no need to upload an entire ISO just to plan the conversion.

In an extracted disc directory, this produces a file list without modifying it:

```bash
python3 - /path/to/extracted-disc <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
for path in sorted(root.rglob('*')):
    if path.is_file():
        print(f'{path.stat().st_size:12d} {path.relative_to(root)}')
PY
```

Record format observations and version-specific differences before writing
converters. Game archives can contain mesh, texture, animation, collision,
script, and audio data with independent formats. Do not assume that identifying
one archive format identifies the engine or makes the game executable on PSP.

## Staged implementation

| Stage | Concrete result | How to validate it |
| --- | --- | --- |
| Native platform foundation | EBOOT, input, camera, renderer, and profiling | Boot in PPSSPP and on target PSP hardware; measure memory and frame time |
| Disc format research | Document archive layout, coordinate systems, texture and collision formats | Parse several known files with bounds checks and consistent record counts |
| Asset conversion | Host tools exporting a documented PSP runtime format | Inspect one small mesh and texture; validate dimensions, indices, and bounds |
| One environment slice | One small area with collision and streaming | Skate through the area without tunneling, missing geometry, or memory spikes |
| Movement reconstruction | Ollies, spins, flips, grinds, manuals, reverts, transitions, and bails | Compare measured behavior with PS2 gameplay and retain regression scenarios |
| Game systems | Script interpreter or reconstructed objectives, pedestrians, vehicles, walking, and menus | Complete one objective from start to finish |
| Content and optimization | Broader environments, animation, sound, saving, and loading | Play-test representative missions and profile on PSP hardware |

The immediate next milestone is a playable and measured prototype on a PSP or
PPSSPP. After that, inspect the supplied disc listing and prove a single asset
conversion before committing to a whole level or game.

## PSP constraints

Design for the PSP-1000's 32 MB main memory and 2 MB graphics memory first;
later PSP models have more main memory. Keep render buffers and command lists
explicit, budget vertices and texture residency, reduce detail using measured
frame times, and stream content instead of loading an entire PS2 level.

Use 480 x 272 rendering and choose a sustainable 30 or 60 FPS target after
hardware profiling. Controller mapping needs a deliberate camera strategy:
the PSP has one analog stick, unlike the PS2 controller.

## Current state

Available now: native 3D park rendering, a simple skater, movement and several
tricks, combos and bails, rail grinding, PSP input, and host gameplay checks.

Remaining: PS2 archive inspection, asset loaders, THUG-specific physics,
animation, scripting, missions, audio, walking, game content, and device
performance validation. A complete THUG port remains a substantial development
and reverse-engineering project.
