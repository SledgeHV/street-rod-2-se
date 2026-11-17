Street Rod 2 SE (Sane Edition) - v1.0.0 - Windows 64-bit
https://github.com/SledgeHV/street-rod-2-se

CESKY - INSTALACE A SPUSTENI

1. Rozbal celý street-rod-2-se-v1.0.0-win64.zip do vlastní zapisovatelné
   složky, například Dokumenty\Street Rod 2 SE. Nespouštěj hru přímo ze ZIPu.
2. Do přiložené složky Game zkopíruj soubory ze své nainstalované DOSové
   verze Street Rod 2. Nejjednodušší je zkopírovat celý její obsah.
   Soubory musí být přímo v Game, ne v další podsložce:

   Street Rod 2 SE\
     Play SE.cmd
     Play Classic.cmd
     Street Rod 2 SE.exe
     SDL3.dll
     Game\
       SR.EXE nebo SR2.EXE
       LIB1
       LIB2
       LIB3
       SR2_DATA
       SR2_SHP
       SR2_END
       MSR.SND
       MSR2.MUS, MSR20.MUS, MSR21.MUS, MSR24.MUS, MSRBEG.MUS, PAUSE.MUS
       (a případné další původní soubory)

3. Spusť Play SE.cmd. Zapne SE vylepšení, 60 FPS, AdLib zvuk a snazší
   Mulholland. Ovládání i nastavení řízení zůstává výchozí jako ve hře.
   Nespouštěj původní SR.EXE / SR2.EXE; SE z něj pouze načítá data.

SDL3.dll musí zůstat vedle Street Rod 2 SE.exe. DOSBox ani instalace dalších
knihoven nejsou potřeba. Herní data nejsou součástí tohoto balíčku.
Uložené hry HOTROD*.SAV a tabulka rekordů hall_dat se zapisují do Game.
Pro přenesení uložených pozic je zkopíruj také do Game.

Play Classic.cmd vypne SE vykreslování a úpravy Mulhollandu. Opravy zvuku
a snížená četnost náhodných poruch převodovky zůstávají aktivní.
Alt+Enter přepíná celé okno / celou obrazovku; M vypíná a zapíná hudbu.
Volitelné asistované klávesové ovládání: Play SE.cmd --controls easy

Hlavní změny: plynulejší jízda a škálování spritů, delší dohlednost, opravy
dekorací, horizontu a mostu, opravy přehrávání hudby, profil Mulhollandu
snížený o 10 %. V lehkém režimu kameny jen zpomalují, lehké kontakty se
svodidly se odpouštějí, pád z útesu vrací auto se zpožděním 3 sekund,
šedý pruh sám brzdí a soupeř má 85 % původní cílové rychlosti a akcelerace
bez úmyslného vytlačování. Grafika zůstává původní, bez hi-res náhrad.

ENGLISH - INSTALLATION AND STARTUP

1. Extract the entire ZIP into a writable folder. Do not run inside the ZIP.
2. Copy your installed original DOS Street Rod 2 files directly into Game
   next to Street Rod 2 SE.exe. See the folder layout and required files above.
   SR.EXE and SR2.EXE are both accepted. Include all original .MUS music files.
3. Run Play SE.cmd for SE enhancements, 60 FPS, AdLib sound and easier
   Mulholland. Original controls and steering settings remain the default.

Keep SDL3.dll beside the EXE. No DOSBox or separate runtime installation is
needed. Original game data is not included. The original DOS executable is
read as data; run Play SE.cmd, not the DOS executable.
Saves (HOTROD*.SAV) and hall of fame (hall_dat) are stored in Game.
Copy existing saves into Game to continue them. Keep this folder writable.

Play Classic.cmd selects original rendering and Mulholland rules, retaining
audio fixes and reduced random transmission failure rates.
Alt+Enter toggles fullscreen; M toggles music.
Optional assisted keyboard controls: Play SE.cmd --controls easy
Advanced: "Street Rod 2 SE.exe" --game-dir "D:\My Games\Street Rod 2" v adlib

SE adds smooth driving and sprite scaling, longer draw distance, scenery,
horizon and bridge fixes, corrected music playback, a 10% flatter Mulholland,
and more forgiving race rules. Rocks slow instead of crashing; cliff recovery
costs three seconds; the grey zone brakes automatically; the opponent uses
85% target speeds/acceleration and no deliberate ramming. Native graphics
are retained. Close opponent sprite-angle changes can still visibly jump;
the dashboard still updates at its original rate.

CREDITS, LICENSES AND SOURCE

Based on kylofon's SDL3 port: https://github.com/kylofon/street-rod-2-sdl3
Port/SE code: MIT (LICENSE). Nuked-OPL3: LGPL 2.1 or later.
SDL3: zlib license. See licenses/THIRD-PARTY.txt and accompanying licenses.
Original Street Rod 2: California Dreams / Logical Design Works.

Matching source, including Nuked-OPL3 and build scripts, is provided as
street-rod-2-se-v1.0.0-source.zip in the same GitHub release:
https://github.com/SledgeHV/street-rod-2-se/releases/tag/v1.0.0
See the source README.md and build-enhanced.ps1 to rebuild or relink with
a modified Nuked-OPL3. The source archive contains no original game data.
