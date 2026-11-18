#!/usr/bin/env python3
"""Package a tested SE Windows build; no game data is read or included.

Run build-enhanced.ps1 -Test -Output build/sr2-se-release.exe first.
Then: python sr2port/tools/package_se.py
Pass --toolchain and --sdl to use different local SDK locations.
"""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[2]
VERSION = "1.1.0"
NAME = f"street-rod-2-se-v{VERSION}-win64"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / "build/sr2-se-release.exe")
    parser.add_argument("--toolchain", type=Path, default=ROOT / "work/toolchain/w64devkit/bin")
    parser.add_argument("--sdl", type=Path, default=ROOT / "work/toolchain/SDL3-3.4.20")
    args = parser.parse_args()
    dll = args.sdl / "x86_64-w64-mingw32/bin/SDL3.dll"
    system = Path(os.environ["SystemRoot"]) / "System32"
    # This small distribution intentionally uses only SDL3 and Windows DLLs.
    # Reject new runtime dependencies instead of silently shipping a broken ZIP.
    for binary in (args.exe, dll):
        report = subprocess.check_output([str(args.toolchain / "objdump.exe"), "-p", str(binary)], text=True)
        if "pei-x86-64" not in report:
            raise SystemExit(f"Not a Windows x64 binary: {binary}")
        for name in re.findall(r"DLL Name:\s*(\S+)", report):
            if name.lower() == "sdl3.dll" or (system / name).is_file():
                continue
            if name.lower().startswith(("api-ms-", "ext-ms-")):
                continue
            raise SystemExit(f"Unpackaged DLL dependency: {name}")

    release = ROOT / "release"
    release.mkdir(exist_ok=True)
    stripped = release / "sr2-se-stripped.exe"
    subprocess.run([str(args.toolchain / "strip.exe"), "-o", str(stripped), str(args.exe)], check=True)
    dist = ROOT / "sr2port/dist/se"
    files = {
        "Street Rod 2 SE.exe": stripped.read_bytes(),
        "SDL3.dll": dll.read_bytes(),
        "README.txt": (dist / "README.txt").read_bytes(),
        "LICENSE": (ROOT / "LICENSE").read_bytes(),
        "licenses/THIRD-PARTY.txt": (dist / "THIRD-PARTY.txt").read_bytes(),
        "licenses/SDL3-LICENSE.txt": (args.sdl / "LICENSE.txt").read_bytes(),
        "licenses/Nuked-OPL3-LICENSE.txt": (ROOT / "sr2port/third_party/nuked-opl3/LICENSE").read_bytes(),
        "Game/PUT_GAME_FILES_HERE.txt": (
            "Copy your installed original Street Rod 2 files directly into this folder.\r\n"
            "Sem zkopiruj soubory puvodni hry Street Rod 2.\r\n"
            "See ../README.txt for the file list and Czech/English instructions.\r\n"
        ).encode("ascii"),
    }
    for script in ("Play SE.cmd", "Play Classic.cmd"):
        files[script] = (dist / script).read_text(encoding="utf-8").replace("\n", "\r\n").encode("ascii")
    path = release / (NAME + ".zip")
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, contents in sorted(files.items()):
            archive.writestr(name, contents)
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == set(files)
    print(f"Package: {path.name} ({path.stat().st_size} bytes, {len(files)} files)")
    print("SHA256:", hashlib.sha256(path.read_bytes()).hexdigest())
    print("Verified x64 dependencies: SDL3.dll and Windows system DLLs only.")


if __name__ == "__main__":
    main()
