Street Rod 2 SE (Sane Edition) - v1.1.0 - Windows 64-bit
https://github.com/SledgeHV/street-rod-2-se

CESKY - INSTALACE A SPUSTENI

1. Rozbal celý street-rod-2-se-v1.1.0-win64.zip do vlastní zapisovatelné
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
   Mulholland i Aqueduct. Výchozí rozložení kláves zůstává původní.
   Nespouštěj původní SR.EXE / SR2.EXE; SE z něj pouze načítá data.

SDL3.dll musí zůstat vedle Street Rod 2 SE.exe. DOSBox ani instalace dalších
knihoven nejsou potřeba. Herní data nejsou součástí tohoto balíčku.
Uložené hry HOTROD*.SAV a tabulka rekordů hall_dat se zapisují do Game.
Pro přenesení uložených pozic je zkopíruj také do Game.

Play Classic.cmd obnoví původní vykreslování, jízdní model a obtížnost tratí. Opravy zvuku
a snížená četnost náhodných poruch převodovky zůstávají aktivní.
Alt+Enter přepíná celé okno / celou obrazovku; M vypíná a zapíná hudbu.
Volitelné asistované klávesové ovládání: Play SE.cmd --controls easy

Hlavní změny: plynulejší jízda a škálování spritů, delší dohlednost, opravy
dekorací, horizontu a mostu, opravy přehrávání hudby, profil Mulhollandu
snížený o 10 %. V lehkém režimu kameny jen zpomalují, lehké kontakty se
svodidly se odpouštějí, pád z útesu vrací auto se zpožděním 3 sekund,
šedý pruh sám brzdí a soupeř na Mulhollandu má 85 % původní cílové rychlosti
a akcelerace bez úmyslného vytlačování. Aqueduct odpouští lehké škrtnutí
o stěny a zkracuje odraz; soupeř nevytlačuje, ale jeho rychlost se nemění.
Grafika zůstává původní, bez hi-res náhrad.

Nově ve v1.1.0: řízení, boční pohyb, motor, brzdy a přístroje běží při 60 Hz.
Náběh řízení zachovává původní 2 -> 4 -> ... -> 24 (cca 2 s do maxima).
Kontra rychle srovná kola do středu (z maxima za 0,2 s), pak opačné zatočení
nabíhá stejně pomalu.
Soupeř se na Aqueductu plynule škáluje i natáčí, také ve zpětném zrcátku.

ENGLISH - INSTALLATION AND STARTUP

1. Extract the entire ZIP into a writable folder. Do not run inside the ZIP.
2. Copy your installed original DOS Street Rod 2 files directly into Game
   next to Street Rod 2 SE.exe. See the folder layout and required files above.
   SR.EXE and SR2.EXE are both accepted. Include all original .MUS music files.
3. Run Play SE.cmd for SE enhancements, 60 FPS, AdLib sound and easier
   Mulholland and Aqueduct. Original key bindings remain the default.

Keep SDL3.dll beside the EXE. No DOSBox or separate runtime installation is
needed. Original game data is not included. The original DOS executable is
read as data; run Play SE.cmd, not the DOS executable.
Saves (HOTROD*.SAV) and hall of fame (hall_dat) are stored in Game.
Copy existing saves into Game to continue them. Keep this folder writable.

Play Classic.cmd selects original rendering, driving and race rules, retaining
audio fixes and reduced random transmission failure rates.
Alt+Enter toggles fullscreen; M toggles music.
Optional assisted keyboard controls: Play SE.cmd --controls easy
Advanced: "Street Rod 2 SE.exe" --game-dir "D:\My Games\Street Rod 2" v adlib

SE adds smooth driving and sprite scaling, longer draw distance, scenery,
horizon and bridge fixes, corrected music playback, a 10% flatter Mulholland,
and more forgiving race rules. Rocks slow instead of crashing; cliff recovery
costs three seconds; the grey zone brakes automatically. On Mulholland, the
opponent uses 85% target speeds/acceleration. Aqueduct adds forgiving wall
scrapes and shorter bounce without reducing opponent pace. Deliberate ramming
is disabled on both courses. Native graphics are retained.

New in v1.1.0: 60 Hz steering, lateral movement, engine, brakes and dashboard.
Steering keeps the original ramp: 2 -> 4 -> ... -> 24, about 2 s to full lock.
Countersteering centres quickly (0.2 s from full lock), then builds the opposite
turn at the same normal rate.
Aqueduct opponent sprites scale and rotate continuously, including in the mirror.
Switching between rear and close side artwork can still visibly jump.

CREDITS, LICENSES AND SOURCE

Based on kylofon's SDL3 port: https://github.com/kylofon/street-rod-2-sdl3
Port/SE code: MIT (LICENSE). Nuked-OPL3: LGPL 2.1 or later.
SDL3: zlib license. See licenses/THIRD-PARTY.txt and accompanying licenses.
Original Street Rod 2: California Dreams / Logical Design Works.

Matching source, including Nuked-OPL3 and build scripts, is provided as
street-rod-2-se-v1.1.0-source.zip in the same GitHub release:
https://github.com/SledgeHV/street-rod-2-se/releases/tag/v1.1.0
See the source README.md and build-enhanced.ps1 to rebuild or relink with
a modified Nuked-OPL3. The source archive contains no original game data.
