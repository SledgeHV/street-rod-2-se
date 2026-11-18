**Street Rod 2 SE (Sane Edition) v1.1.0** pro **Windows 64-bit**.

### Novinky od v1.0.0

- **Plynulejší jízdní model:** řízení, boční pohyb, motor a brzdy se aktualizují 60× za sekundu nezávisle na zobrazovaných FPS. Odezva směru a boční polohy už nečeká na starý interpolovaný snímek.
- **Předvídatelné řízení:** původní náběh 2 → 4 → 6 → … → 24 (cca 2 s do maxima). Kontra nejprve srovná kola do středu (z maxima za 0,2 s), potom opačné zatočení nabíhá běžnou rychlostí.
- **Plynulejší přístroje:** ručičky a ukazatel volantu se obnovují při 60 Hz; volant ukazuje skutečný rejd. Původní grafika zůstává zachovaná.
- **Jemnější plyn a brzdy:** průběžný výpočet s desetinnou přesností a plynulejší přechody mezi výkonovými pásmy. Puštění brzdy už téměř stojící auto nezrychlí na rychlost odpovídající volnoběžným otáčkám.
- **Snazší Aqueduct:** lehké škrtnutí o stěnu ubere 25 % rychlosti, odraz je kratší a soupeř úmyslně nevytlačuje. **Rychlost ani zrychlení soupeře se nemění.** Silné nárazy, překážky a tvar tratě si zachovávají původní pravidla.
- **Plynulé auto soupeře na Aqueductu:** průběžné škálování a natáčení původního spritu ve všech náklonech, včetně blízkých pohledů a zpětného zrcátka.

Časování závodu, řazení, opotřebení a AI zůstává původní. Výchozí rozložení kláves se nemění; `--controls easy` nabízí WASD/šipky a Q/E se stejným rozsahem rejdu. `--classic` obnoví původní jízdní model.

Při aktualizaci lze rozbalit nový balíček do nové složky a přenést do něj původní složku **Game**, včetně uložených pozic.

### Stažení a spuštění

1. Stáhni **street-rod-2-se-v1.1.0-win64.zip** níže v Assets a celý ho rozbal do zapisovatelné složky.
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

### Zachovaná vylepšení z v1.0.0

- Výchozích 60 FPS, plynulejší škálování spritů a pohyb kamery, delší dohlednost.
- Opravené dekorace v kopcích a ve městě, plynulejší horizont a útesy, oprava vykreslování u závěrečného mostu.
- Opravy přehrávání AdLib hudby a přibližně desetkrát méně náhodných poruch převodovky.
- Výškový profil Mulhollandu snížený o 10 %.
- Lehké kontakty se svodidly se odpouštějí, kameny jen zpomalují, pád z útesu vrací auto s třísekundovým zdržením a šedý pruh automaticky brzdí.
- Soupeř na Mulhollandu má 85 % původní cílové rychlosti a akcelerace, bez úmyslného vytlačování. Na Aqueductu zůstává původní rychlost.

Původní grafika a výchozí rozložení kláves zůstávají zachované.
`Play Classic.cmd` obnoví původní vykreslování, jízdní model a obtížnost tratí; opravy zvuku a snížená četnost náhodných poruch zůstávají.
Přechod mezi zadním a bočním obrázkem velmi blízkého soupeře může stále viditelně poskočit. Podélný pohyb a okolní scéna nadále používají interpolaci s prodlevou přibližně jednoho původního snímku.

### English quick start

Download **street-rod-2-se-v1.1.0-win64.zip**, extract everything, copy your installed original DOS game files directly into **Game** beside **Street Rod 2 SE.exe**, then run **Play SE.cmd**. See the file layout above. Include `SR.EXE` or `SR2.EXE`, all listed resources and all original `.MUS` files. SDL3 is bundled; no DOSBox is needed. Original game files are not included. Saves are read/written in **Game**. Czech and English instructions are included in `README.txt`.

New since v1.0.0: 60 Hz player steering, lateral movement, engine, braking and dashboard; current-state steering presentation; smoother Aqueduct opponent scaling/rotation, including the mirror; forgiving Aqueduct wall scrapes and shorter bounce without reducing opponent speed. Steering keeps the original ~2 s ramp to full lock. Reversals centre quickly, then build the opposite turn at the normal rate. Original graphics, key bindings, race clocks, wear and shift timing are retained. See the [project README](https://github.com/SledgeHV/street-rod-2-se#readme) for the full feature list.

### Source and validation

**street-rod-2-se-v1.1.0-source.zip** contains matching source and build scripts, including Nuked-OPL3; it is for developers. **SHA256SUMS.txt** contains checksums of both ZIPs.

The Windows build passed interpolation/isolation tests (822 replay checks), scenery, bridge, opponent scaling/rotation, difficulty, height-profile and audio tests. Driving regressions cover steering reversals with throttle/brake, wheel resets, symmetry, braking, motor bands and unchanged race/wear/AI clocks. The extracted package was checked for DLL dependencies and SE/Classic startup with local original data and SDL dummy drivers. The final steering correction was confirmed in a manual playtest; automated startup checks alone do not validate appearance or sound.

Based on [kylofon's SDL3 port](https://github.com/kylofon/street-rod-2-sdl3). Port/SE source: MIT; Nuked-OPL3: LGPL 2.1 or later; SDL3: zlib. Licenses are included.
