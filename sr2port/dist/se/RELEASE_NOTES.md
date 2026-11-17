První vydání **Street Rod 2 SE (Sane Edition)** pro **Windows 64-bit**.

### Stažení a spuštění

1. Stáhni **street-rod-2-se-v1.0.0-win64.zip** níže v Assets a celý ho rozbal do zapisovatelné složky.
2. Do přiložené složky **Game**, vedle **Street Rod 2 SE.exe**, zkopíruj obsah své nainstalované původní DOSové hry Street Rod 2.
3. Spusť **Play SE.cmd**. SDL3 je přibalené; DOSBox ani další instalace nejsou potřeba.

Soubory musí být **přímo v Game**, ne v další podsložce:

```text
Street Rod 2 SE/
  Play SE.cmd
  Play Classic.cmd
  Street Rod 2 SE.exe
  SDL3.dll
  Game/
    SR.EXE nebo SR2.EXE
    LIB1
    LIB2
    LIB3
    SR2_DATA
    SR2_SHP
    SR2_END
    MSR.SND
    *.MUS (všechny původní hudební soubory)
```

Hudební soubory zahrnují `MSR2.MUS`, `MSR20.MUS`, `MSR21.MUS`, `MSR24.MUS`, `MSRBEG.MUS` a `PAUSE.MUS`.
Původní `SR.EXE` / `SR2.EXE` slouží jako data; spouštěj **Play SE.cmd**.
Uložené pozice `HOTROD*.SAV` a tabulka rekordů `hall_dat` patří také do **Game**.
Herní data nejsou součástí vydání. V balíčku je český i anglický návod `README.txt`.

### Co SE mění

- Výchozích 60 FPS, plynulejší škálování spritů a pohyb kamery, delší dohlednost.
- Opravené dekorace v kopcích a ve městě, plynulejší horizont a útesy, oprava vykreslování u závěrečného mostu.
- Opravy přehrávání AdLib hudby a přibližně desetkrát méně náhodných poruch převodovky.
- Výškový profil Mulhollandu snížený o 10 %.
- Lehké kontakty se svodidly se odpouštějí, kameny jen zpomalují, pád z útesu vrací auto s třísekundovým zdržením a šedý pruh automaticky brzdí.
- Soupeř má 85 % původní cílové rychlosti a akcelerace, bez úmyslného vytlačování.

Původní grafika a výchozí ovládání i nastavení řízení zůstávají zachované.
`Play Classic.cmd` vypne SE vykreslování a úpravy Mulhollandu; opravy zvuku a snížená četnost náhodných poruch zůstávají.
Změna úhlu spritu blízkého soupeře může stále viditelně poskočit; přístrojová deska používá původní frekvenci aktualizace.

### English quick start

Download **street-rod-2-se-v1.0.0-win64.zip**, extract everything, copy your installed original DOS game files directly into **Game** beside **Street Rod 2 SE.exe**, then run **Play SE.cmd**. See the file layout above. Include `SR.EXE` or `SR2.EXE`, all listed resources and all original `.MUS` files. SDL3 is bundled; no DOSBox is needed. Original game files are not included. Saves are read/written in **Game**. Czech and English instructions are included in `README.txt`.

Default: smoother 60 FPS driving, native graphics, original steering/controls and easier Mulholland. See the [project README](https://github.com/SledgeHV/street-rod-2-se#readme) for the full feature list.

### Source and validation

**street-rod-2-se-v1.0.0-source.zip** contains matching source and build scripts, including Nuked-OPL3; it is for developers. **SHA256SUMS.txt** contains checksums of both ZIPs.

The Windows build passed interpolation/isolation tests (755 replay checks), scenery and bridge rendering, difficulty and height-profile tests, and audio regression tests. The extracted package was checked for DLL dependencies and tested for SE/Classic startup with original game data using SDL dummy video/audio drivers. These startup checks do not replace listening or visual playtesting.

Based on [kylofon's SDL3 port](https://github.com/kylofon/street-rod-2-sdl3). Port/SE source: MIT; Nuked-OPL3: LGPL 2.1 or later; SDL3: zlib. Licenses are included.
