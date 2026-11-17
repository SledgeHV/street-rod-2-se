# Street Rod 2 SE (Sane Edition)

A smoother, more forgiving edition of *Street Rod 2: The Next Generation* (1991),
based on [kylofon's native C/SDL3 port](https://github.com/kylofon/street-rod-2-sdl3).
SE keeps the original **320 × 200, 16-colour graphics**, displayed at 4:3, and
focuses on smoother driving, continuous sprite scaling and a less punishing
Mulholland race. There are no high-resolution replacement graphics.

**You need your own original game files.** They are not included in this repository.
Podrobný český popis změn a nastavení: **[ENHANCED.md](ENHANCED.md)**.

## Download and play (Windows 64-bit)

Download **[Street Rod 2 SE v1.0.0](https://github.com/SledgeHV/street-rod-2-se/releases/tag/v1.0.0)**
and extract `street-rod-2-se-v1.0.0-win64.zip` to a writable folder.
Copy your installed original Street Rod 2 files into the included **`Game`**
folder, next to `Street Rod 2 SE.exe`, then run **`Play SE.cmd`**.
SDL3 is included; no separate installation or DOSBox is needed.

The files must be directly inside `Game`, not inside another subfolder:
`SR.EXE` (or `SR2.EXE`), `LIB1`, `LIB2`, `LIB3`, `SR2_DATA`, `SR2_SHP`,
`SR2_END`, `MSR.SND` and all original `*.MUS` music files.
See the included `README.txt` for Czech and English instructions.
The original DOS executable is read as game data; start the SE program instead.

## SE improvements

- **60 FPS driving by default**, configurable from 30 to 240 FPS. Additional
  frames reproject the road, camera and objects between simulation states;
  the original physics and game timers do not run faster.
- **Continuous sprite scaling** from the most detailed original sprites,
  smoother opponent positioning, fractional projection and camera height.
- **Longer draw distance:** up to 28 road segments instead of 14 on ordinary
  roads, Mulholland and the drag strip. Aqueduct retains its original range.
- **Smoother scenery and horizon:** pixel-by-pixel panorama scrolling, fewer
  decoration pop-ins, improved hillside visibility and a fix for the cliff
  polygon flashing across the road near the final bridge.
- **Mulholland's elevation profile reduced by 10%** in enhanced mode.
- **AdLib music fixes:** repeated notes and drum hits retrigger correctly,
  track changes no longer cause a long silence, and playback starts with a
  filled audio buffer.
- **Roughly ten times fewer random transmission failures.** Wear-out and
  sustained engine over-revving still matter.

### A more forgiving Mulholland

The default `--difficulty easy` changes Mulholland only:

- Light wall/guardrail contacts slow and deflect the car; severe wall impacts
  can still cause a crash.
- Road rocks only slow the car at any speed. They do not crash it or alter steering.
- Falling off a cliff or narrow bridge returns the car to a safe position,
  imposes a three-second delay while the opponent keeps moving, and restarts
  the car at 25 mph.
- Opponent target speeds and acceleration are reduced to 85%, including corner
  speed limits. Deliberate ramming is disabled; ordinary car collisions remain.
- The grey slow zone automatically brakes to at most 60 mph instead of causing
  a crash for entering too fast.

**Original steering and controls remain the default.** Optional `--controls easy`
enables WASD/arrow driving, Q/E shifting and steering assistance. Difficulty and
controls are independent settings.

## Build and run SE

Requirements: a C11 compiler, CMake 3.24+ and the SDL3 development package.
On Windows, MinGW-w64/MSYS2 is supported. Make SDL3 discoverable to CMake, using
`-DCMAKE_PREFIX_PATH=<SDL3-prefix>` if needed.

```sh
cmake -S sr2port -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSR2_LAUNCHER=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Put your installed DOS game files in `game_original/` or `Game/`. Both `SR.EXE`
and `SR2.EXE` are recognized. On Windows, run **Play Enhanced.cmd** or
**Play Classic.cmd**. Keep SDL3 and any required runtime DLLs beside the EXE.

Alternatively, supply your compiler and SDL3 SDK paths to the standalone build script:

```powershell
.\build-enhanced.ps1 -Compiler C:/path/to/gcc.exe -SDL C:/path/to/SDL3-prefix -Test
.\build\sr2-enhanced.exe --game-dir game_original v adlib
```

The script produces `build/sr2-enhanced.exe`; CMake produces `sr2port` (or
`sr2port.exe`). The Windows scripts recognize either name in `build/`.
To build the Windows release package, run
`./build-enhanced.ps1 -Test -Output build/sr2-se-release.exe`, then
`python sr2port/tools/package_se.py` (use `--toolchain` and `--sdl` for custom SDK
locations). The packager checks DLL dependencies and includes only the executable,
SDL3, launch scripts, instructions and licenses; it never packages original game data.
Examples after a CMake build:

```sh
# Enhanced display, original controls, easier Mulholland (defaults)
./build/sr2port --game-dir game_original v adlib
# Optional keyboard driving assistance
./build/sr2port --game-dir game_original --controls easy v adlib
# Smooth display with original Mulholland contact/opponent rules
./build/sr2port --game-dir game_original --difficulty classic v adlib
# Original rendering, controls, course height and Mulholland rules
./build/sr2port --game-dir game_original --classic v adlib
```

Classic still includes the audio fixes and reduced random transmission failure
rate; it is not a bit-for-bit restoration of upstream game logic.
Save games and the hall of fame are read and written in the selected game-data
directory (`Game` in the Windows release), so that directory must be writable.
The original program and resource files are not modified.

## Validation and current limits

Regression tests cover rendering-state isolation and interpolation (755 replay
checks), scenery pixels, bridge-bank rendering, difficulty rules, course height,
controls and audio retriggering. Simulated Mulholland, Aqueduct and drag races
have also been run using original game data locally.

The low native resolution still produces one-pixel movement steps. Very close
side views of the opponent and some collision/roll animations retain the original
sprite selection; switching from a rear to a side view can still visibly jump.
The dashboard retains the original update rate. Interpolation adds approximately
one original frame of visual delay. See [ENHANCED.md](ENHANCED.md) for details.
Local diagnostic captures and original game data are not published.

## Contributors and credits

- **[SledgeHV](https://github.com/SledgeHV)** — Sane Edition direction, gameplay
  feedback and playtesting.
- **Codex (OpenAI)** — AI coding assistance with SE implementation, debugging,
  automated testing, documentation and release packaging.

The original reverse engineering and SDL3 port are by
[kylofon](https://github.com/kylofon/street-rod-2-sdl3). This fork builds on that work;
the rendering approach was also inspired by
[Street Rod Enhanced](https://github.com/kylofon/streetrod-enhanced).
Nuked-OPL3 retains its [LGPL-2.1-or-later license](sr2port/third_party/nuked-opl3/LICENSE).

The original port's introduction and development notes follow below. The linked
upstream Windows release does **not** contain the Sane Edition changes.

## Upstream project

A faithful native port (C, SDL3) of California Dreams' *Street Rod 2: The Next Generation* (1991),
in the making. Not an emulator. It builds on the engine of the
[Street Rod port](https://github.com/kylofon/street-rod-sdl3), since both games share most of their
code. The original game files are not included: the port reads them from a `Game` folder at run
time.

Status and plan: [PLAN.md](PLAN.md) (as a page: `python tools/plan_site.py`, then open
`site/index.html`). Agent rules: [CLAUDE.md](CLAUDE.md).

## Running

Download: [v0.1.0 for Windows x64](https://github.com/kylofon/street-rod-2-sdl3/releases/tag/v0.1.0) (needs your own copy of the game).


Build (MSYS2 mingw64, see `sr2port/PORTING.md`), put the installed game in `Game\`, then either start
the launcher, **`Street Rod 2.exe`** (built beside `sr2port.exe` when wxWidgets 3.2 is installed: game folder,
graphics, sound AdLib / Normal, the game's switches, window size, full screen), or run the port directly:

```text
sr2port.exe --game-dir Game v adlib      (v = VGA, adlib or speaker = sound; no menus)
sr2port.exe --game-dir Game              (the original's text menus for video and sound)
```

## Layout

```text
Game\        the installed DOS game (not in git)
_tools\      junction to Ghidra + JDK (shared with the other ports, not in git)
tools\       reverse-engineering scripts (Python, Ghidra)
port\        RE guide, function indexes, SR1 matches, symbols, specs (port\spec), formats (port\formats)
docs\        manual PDFs/text (not in git)
work\        generated output and DOSBox reference captures (not in git)
site\        local HTML view of PLAN.md with token usage (not in git)
sr2port\     the port (from phase 4)
```

## License

MIT for the code in this repository (see [LICENSE](LICENSE)). Street Rod 2 is © California Dreams /
Logical Design Works.
