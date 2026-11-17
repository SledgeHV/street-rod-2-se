# Street Rod 2 SE (Sane Edition) — plynulá jízda v původním rozlišení

Street Rod 2 SE vychází z `kylofon/street-rod-2-sdl3` a přidává plynulejší
zobrazení jízdy a volitelné jednodušší ovládání. Zůstává 320 × 200 a původní
šestnáctibarevná grafika, zobrazená v poměru 4:3. Původní herní data se nemění.

## Spuštění

- **Play Enhanced.cmd**: vylepšená jízda, limit 60 FPS, VSync, původní ovládání hry a lehčí Mulholland.
- **Play Classic.cmd**: původní vykreslování, ovládání a pravidla kontaktů/soupeře pro porovnání.

Oba soubory použijí `game_original`, případně `Game`. Program rozpozná `SR.EXE`
i `SR2.EXE`. Připravená Windows verze je `build/sr2-enhanced.exe`; vedle ní musí
zůstat `SDL3.dll`. Uložené hry vznikají v pracovním adresáři, nikoli v `game_original`.

Výchozí je původní ovládání hry (`--controls classic`), včetně původní odezvy a
rozsahu zatáčení. Platí i při přímém spuštění EXE. Režim easy je pouze volitelný,
zapíná se přepínačem `--controls easy`.

## Obtížnost Mulhollandu

Výchozí `--difficulty easy` je nezávislé na ovládání: zatáčení zůstává původní.
Nové úlevy se týkají pouze závodu Mulholland; drag, akvadukt a přejezdy městem
si ponechávají dosavadní pravidla. `--difficulty classic` vypne tyto úlevy při
zachování plynulého obrazu. `--classic` je vypne společně s obrazovými vylepšeními.
Dřívější desetinásobné snížení náhodných poruch převodovky zůstává v obou režimech.

- Lehké kontakty se svodidly a skalní stěnou ubírají rychlost a mírně
  odrazí auto zpět. Ztráta je nejméně 10 mph, případně čtvrtina rychlosti.
  Stěna od 100 mph a prudký náraz do stěny zůstávají havárií.
- Kameny na silnici při jakékoli rychlosti pouze zpomalují: odečtou 15 mph,
  nebo čtvrtinu rychlosti, pokud je větší; nejvýše však do zastavení.
  Nemění polohu, směr ani natočení volantu a samy nezpůsobují havárii.
  Opakované hlášení stejného kontaktu má krátkou ochrannou lhůtu.
- Pád z útesu nebo okraje úzkého mostu vrátí auto na poslední bezpečné místo,
  srovná ho se silnicí a zdrží ho na přibližně tři sekundy s viditelným odpočtem.
  Soupeř pokračuje.
  Poté auto pokračuje rychlostí 25 mph s odpovídajícím převodem. Zdržení se počítá
  do běžného času závodu; nepřičítá se podruhé. Bez předchozího bezpečného vzorku
  se auto vrátí do středu aktuálního úseku.
- Cílové rychlosti soupeře i jeho zrychlování jsou na 85 % původních hodnot.
  Platí i pro omezení v zatáčkách a pomalých úsecích. Náhodné úmyslné vytlačování
  je vypnuté; běžná kolize s autem se nadále vyhodnocuje.
- Šedý pomalý úsek automaticky přibrzdí na nejvýše 60 mph místo havárie za
  překročení rychlosti. Brzdění synchronizuje otáčky a nechává zvolený převod;
  v nízkém převodu omezí rychlost ještě víc, aby nepřetočilo motor. Držený plyn
  omezení nepřebije. Samotný pruh nepoškozuje auto; okolní překážky mají své kolize.

Parametry a obnova auta jsou v `sr2port/src/enhanced/difficulty.c`. Herní data
a uložené hry se kvůli obtížnosti nepřepisují.

## Volitelné ovládání

| Ovládání ve volitelném režimu easy | Klávesa |
|---|---|
| Plyn / brzda | W / S nebo šipky nahoru / dolů |
| Zatáčení | A / D nebo šipky doleva / doprava |
| Vyšší / nižší rychlostní stupeň | E / Q |
| Automatická převodovka: D / N | E / Q |
| Původní kontextové řazení | Mezerník |
| Celá obrazovka | Alt+Enter |

Režim easy omezuje natočení volantu podle rychlosti a po uvolnění klávesy jej
vrací do středu. Pohyb myši v tomto režimu neotáčí volantem. Nabídky a jejich
klávesové zkratky zůstávají původní. Režim classic zachovává také řazení A/Z.

Parametry současného easy řízení jsou v `sr2port/src/enhanced/controls.c`, funkci
`enh_steering`: maximální natočení je 12 do rychlosti 50, 8 do rychlosti 90 a 6
nad 90 (interní jednotky volantu). Přiblížení k požadovanému natočení má krok 4;
návrat do středu nebo změna směru krok 6. Funkce běží jednou za fyzikální krok,
běžně přibližně šestkrát za sekundu, nikoli každý zobrazovaný snímek. Původní
řízení má limit 24 a vlastní návrat volantu. Tyto hodnoty se při opravě soupeřova
poskakování neměnily; nové přepínače citlivosti nepřibyly.

```powershell
# Plynulé zobrazení, původní ovládání (výchozí):
.\build\sr2-enhanced.exe --game-dir game_original v adlib

# Volitelné jednodušší ovládání:
.\build\sr2-enhanced.exe --game-dir game_original --controls easy v adlib

# 120 FPS bez VSync (rozlišení se nemění):
.\build\sr2-enhanced.exe --game-dir game_original --fps 120 --vsync off v adlib
```

`--fps` přijímá 30–240, výchozí hodnota je 60. VSync může skutečnou frekvenci
omezit podle monitoru. Přepínač `--classic` vypne obrazová vylepšení, lehčí obtížnost
i easy ovládání; následující `--controls easy` nebo `--difficulty easy` může
příslušnou volbu samostatně zapnout.

## Co se změnilo

Původní snímek hry stále končí `pace_ticks(8)` a fyzika běží v původním časovači.
Dodatečné snímky vznikají interpolací polohy a kamery mezi dvěma stavy, přibližně
jeden původní snímek za simulací (obvykle kolem 110 ms). Nejde o prolínání obrázků:
silnice a objekty se znovu promítají podle interpolované polohy. Ani hudební
časovač, soupeřova AI či pravidla závodů se kvůli FPS nezrychlují.

Poloha auta, soupeře a hodnoty kamery nyní používají omezenou kubickou interpolaci.
Při běžném zrychlování navazuje i rychlost obrazového pohybu mezi původními snímky,
nejen poloha. Křivka zůstává mezi zachycenými hodnotami; zastavení a změna směru
omezí její tečny, teleport nebo návrat po pádu historii resetuje. Nepřidává se další
snímek zpoždění. Při delším záseku vykreslování se pohyb zastaví na posledním známém
stavu, nepředpovídá další jízdu.

Výška kamery na běžné silnici se bere z výškové mapy v přesné zobrazené poloze,
včetně zlomku segmentu. Tím odpadá opožděný výškový skok při přejezdu hranice
segmentu. Pád, plochý drag a zvláštní kamera akvaduktu si ponechávají své chování.
Projekce silnice a zrcátka uchovává desetinné souřadnice; dekorace, cedule,
skupiny keřů a běžné pohledy na soupeře je používají až při rasterizaci spritů.
Původní celočíselná projekce zůstává pro simulaci a Classic.

Renderer dvojky zároveň zapisuje údaje o kolizích, náklonu akvaduktu a kontaktu
se soupeřem. Proto se každý dodatečný snímek kreslí v dočasné kopii celé emulované
paměti, čtyř grafických rovin a VGA registrů. Po vykreslení se obnoví živý stav.
Překreslení dialogu se eviduje i tehdy, když zapisuje stejnou barvu; obraz jízdy
tak nepřekrývá dialogy. Palubní deska zůstává na původní frekvenci.

Objekty ze `SR2_SHP` skutečně obsahují předpočítané velikosti. Enhanced používá
nejdetailnější variantu a mění měřítko podle vzdálenosti. Totéž platí pro cedule,
zadní/přední pohledy na soupeře a překážky v akvaduktu. Nejbližší boční pohledy
na auto, kontaktní situace a některé varianty jeho náklonu nadále používají
původní výběr spritu. Startér a startovní světla zachovávají původní animaci.
Škálování používá ostré pixely, nikoli rozmazání; v rozlišení 320 × 200 jsou
jednopixelové kroky stále přirozenou mezí.

Oprava poskakování soupeře sjednocuje vzdálenost použitou pro velikost a polohu
spritu. U předních/zadních pohledů už poloha neskáče po celých jednotkách hloubky;
na běžné silnici se také plynule vzorkuje výška a směr tratě mezi segmenty.
Interpolovaná poloha soupeře uchovává i zlomek menší než původní 1/64 segmentu,
takže auta jedoucí stejnou rychlostí nemění vzájemný odstup kvůli zaokrouhlení.
V akvaduktu se stejná plynulá hloubka používá před otočením kotevního bodu spritu.
Boční pohledy a kolizní animace nadále používají původní zobrazení.

Regresní testy kontrolují stálý odstup při stejné rychlosti a plynulý průjezd
výškovými/směrovými segmenty před kamerou i ve zpětném zrcátku. Sestavení a běhy
Mulhollandu i akvaduktu na dodaných datech prošly; vizuální výsledek je ještě
potřeba posoudit při hraní.

### Dohlednost, horizont a útesy

Podélné převýšení Mulhollandu je v Enhanced sníženo o **10 %** kolem výšky
nájezdu na trať. Koeficient `ENH_MULHOLLAND_HEIGHT_PERCENT` je 90; mění se až
hotový nově vygenerovaný profil, takže opakované načítání nesnižuje kopce dál.
Silnice, horizont, dekorace i soupeř používají stejnou výškovou mapu. Původní
datové soubory a uložené hry se nemění; úprava platí i po načtení existující hry.
Boční útesy, zatáčky a výšky ostatních tratí zůstávají původní. `--classic`
používá 100% profil; samotné `--difficulty classic` koeficient výšky nemění.

Enhanced vykresluje běžnou silnici, Mulholland a drag až do **28 segmentů místo
14**. Stromy, keře, skály i městské lampy, cedule a další dekorace používají
stejná pravidla a plynulé měřítko po celém dosahu, bez přepnutí na omezenou
sadu vzdálených siluet. Banky v kopcích už nevypínají blízké dekorace pouhým
testem rozdílu výšek; bližší terén je nadále překrývá v pořadí vykreslování.
Skupiny keřů mají perspektivně promítané rozložení bez čtení za krátké původní
tabulky. Blízké sprity se ořezávají výhledem, jejich velikost už nezastaví strop
2,5×. U objektů s původním příznakem zachování nad palubní deskou platí stejná
spodní kotva i v Enhanced. Vzdálené objekty používají zmenšené původní sprity;
také soupeř před autem může být vidět dále. Na konci dat tratě se dosah bezpečně
zkracuje. Akvadukt zatím zachovává svých 14 segmentů a zrcátko 7. Classic používá
původní vzdálenost.

Horizont má posun po jednotlivých pixelech, počítaný z nezaokrouhleného směru
kamery. Původní posun po osmi pixelech je odstraněný včetně omezení původního
kopírování grafiky po bajtech. Výška horizontu plynule vzorkuje profil tratě;
odstraněn je také skok při dosažení horní hranice obrazu.

Profil útesů interpoluje původní tabulku velikostí podle skutečné vzdálenosti,
místo přepnutí podle celého řádku. Za původním dosahem se profil dále perspektivně
zmenšuje. Zobrazení útesu u mostu je navázané na sousední segmenty tratě;
vstup mostu do krátkého původního výhledu už nepřepíná všechny útesy najednou.
Útesy se také neořezávají skokovým testem směru kamery.

Rozšířené tabulky jsou oddělené od původních 15prvkových polí v herní paměti.
Fyzika a výchozí původní řízení se nemění. Testy ověřují omezení vzdálenosti podle
délky dat, posun přes šev panoramatu a spojitost profilu útesu při průjezdu hranicí
segmentu. Testovací běhy Mulhollandu a dragu držely přibližně 60 FPS; vykreslení
mezisnímku trvalo přibližně 1–1,6 ms. Vizuální doladění zbývá ověřit při hraní.

Implementace je v `sr2port/src/enhanced/`. Architektura byla inspirována
[Street Rod Enhanced](https://github.com/kylofon/streetrod-enhanced), ale používá
vlastní renderer SR2, aby zachovala jeho kopce, mosty a akvadukt bez hi-res přepisu.

## Sestavení a ověření

### Opravy hudby

Zápisy do AdLibu nyní procházejí časovanou frontou Nuked-OPL3. Původní port posílal
vypnutí a zapnutí tónu v jednom okamžiku; emulátor pak nezaznamenal uvolnění klávesy
a opakované tóny i údery bicích vypadávaly. Fronta zachová mezi zápisy přibližně
40 mikrosekund. Také je opraven přechod mezi skladbami: nulová úvodní prodleva
už nezpůsobí přetečení čítače a více než dvouminutové ticho. Opravy platí i pro
spouštěč Classic. Zvukový výstup se spouští až po naplnění úvodní zvukové rezervy.

Regresní test syntetizuje opakovaný tón i bicí přes skutečnou zvukovou cestu a
kontroluje energii obou úderů. Před opravou měl druhý tón přibližně 0,1 % energie
prvního, po opravě 100 %. Další test přehraje dvě krátké umělé skladby a ověří,
že druhá začne bez dlouhého výpadku. Herní data nejsou pro tyto testy potřeba.
Na dodaných datech proběhl také 45sekundový záznam úvodu bez ořezání signálu,
vyprázdnění vstupní zvukové fronty nebo zahazování vzorků. Subjektivní poslech
na uživatelově výstupu zůstává k ověření.

Pro diagnostiku lze nastavit `SR_AUDIO_STATS=1` (stav fronty) a
`SR_AUDIO_WAV=work/intro.wav` (záznam generovaného signálu).

### Současná pravidla poruch

Náhodná šance zničení převodovky je snížená přibližně na desetinu původní hodnoty,
včetně spuštění přes Classic. Easy samostatně mění ovládání. Na začátku závodu
se neprovádí zvláštní náhodný hod na poškození.

- **Převodovka:** při řazení manuálu se stisknutým plynem a otáčkami nad 5 000
  hrozí okamžité zničení. Pravděpodobnost je přibližně
  `trunc((100 + trunc(opotřebení / 100)) / 10) / 3277`, s opotřebením v rozsahu 0–10 000,
  vyhodnoceným po přičtení opotřebení za řazení. Příklady: nová asi **0,31 %**,
  z poloviny opotřebená **0,46 %**, z 90 % opotřebená **0,58 %** za každé takové
  řazení. Přesněji generátor používá 15bitovou hodnotu modulo 3277, takže pro
  uvedené prahy je pravděpodobnost `10 * práh / 32768`. Bez plynu, do 5 000 otáček
  nebo s automatem se tento náhodný test neprovádí.
- **Opotřebení převodovky** se při řazení zvyšuje o 15 nad 5 000 otáček, jinak o 3.
  Od hodnoty 9 900 se při řazení zničí jistě, i bez výše uvedeného náhodného testu.
- **Motor:** zničení není náhodné. Nastane při opotřebení alespoň 9 900 nebo při
  soustavném přetáčení na limitu 5 700 otáček. Limit je
  `15 - trunc(opotřebení / 1000)` fyzikálních kroků; překročení tohoto počtu motor
  zničí. U nového motoru jde o 16 kroků, přibližně 2,6 s na limitu. Platí to také
  při držení plynu na neutrál před startem. Automat omezuje otáčky na 5 690.
- **Startér jako součást auta** se samostatně nesimuluje. `starter` ve zdrojácích
  označuje člověka startujícího závod; jeho sražení závisí na poloze auta.

Zdroj pravidel: `sr2port/src/game/race_phys.c` (`race_phys_step`, `race_step`),
generátor náhody: `sr2port/src/game/flow_data.c` (`rnd`).

### Postup sestavení

Připravený přenosný GCC a SDL3 SDK jsou pouze v ignorované složce `work/toolchain`.
Do systému se nic neinstaluje:

```powershell
.\build-enhanced.ps1 -Test
```

Jiný MinGW GCC / SDL3 SDK lze předat parametry `-Compiler` a `-SDL`. Alternativně
lze použít původní CMake sestavení (CMake 3.24+, C11, SDL3, bez launcheru):

```text
cmake -S sr2port -B build -G Ninja -DSR2_LAUNCHER=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake pojmenovává hru `sr2port`; spouštěcí CMD soubory přijímají i toto jméno.
Automatické testy nevyžadují herní data. Ověřují obnovu celého stavu po 120
opakovaných mezisnímcích, interpolaci přes hranici segmentu, skok polohy,
překrytí dialogy, klasický režim, opakování a uvolnění kláves, návrat volantu,
průhlednost a ořezávání škálovaných spritů a výběr největší varianty.
Další testy kontrolují návaznost rychlosti při zrychlování, nepřestřelení při
zastavení a změně směru, přesnou výšku kamery přes vrchol kopce, desetinné
souřadnice před projekcí i v kotvách dekorací a zachování zvláštních kamer.
Po těchto třech úpravách prošlo 755 testovacích překreslení a všechny integrační
i audio testy. Krátké běhy na dodaných datech s dummy ovladači držely přibližně
60 FPS: Mulholland 1,4–1,7 ms na mezisnímek, město 0,9–1,1 ms, drag 0,9–1,1 ms
a akvadukt 0,6–0,8 ms. Tyto časy nejsou měřením skutečného zobrazení na monitoru.
Integrační test obtížnosti používá skutečný kód kolizí, fyziky a soupeře se
syntetickou tratí: ověřuje lehké/tvrdé kontakty, původní kolize, návrat z útesu
i mostu, odpočet, běh soupeře během penalizace, čas výsledku, rychlosti soupeře
na rovince/v zatáčce/v pomalém úseku a vypnuté vytlačování.
Pixelové testy skutečné rutiny dekorací ověřují oba svahy v řádcích 3–28,
vzdálené městské objekty, spodní ořezání, blízké vysoké sprity a bezpečné
indexování skupin keřů. Test zpomalovacího úseku ověřuje automatické brzdění
s drženým plynem, otáčky, nepoškozené díly a zachování původního režimu.

Lokálně ověřeno 2026-11-17: GCC sestavení bez varování; všechny uvedené testy;
na dodaných datech kontrola EXE a běhy města, dragu, Mulhollandu a akvaduktu
s dummy video/audio ovladačem. Samostatný běh akvaduktu dosahoval 59,9–60,1 FPS,
vykreslení mezisnímku přibližně 0,5–0,7 ms. Limity 30 a 120 FPS byly také ověřeny.
Skriptované W/E rozjelo auto a zařadilo první stupeň; po uvolnění plynu zpomalovalo.

Pro diagnostiku lze nastavit `SR_ENH_STATS=1` (FPS, čas vykreslení, rychlost a stupeň).
Herní pocit z řízení, vzhled přechodů spritů a chování na fyzickém monitoru ještě
vyžadují lidský playtest; nejde o dokončenou vizuální regresní kontrolu ani záruku
stejného výkonu na jiném počítači.

## Audit simulovaných závodů — 2026-11-17

Proběhlo deset běhů na dodaných datech s dummy video/audio ovladači. Pět závodů
skončilo výsledkem a následným návratem do města; ostatní byly kratší cílené
scénáře. Kontrola zahrnovala přehledy zachycených snímků a hustou sekvenci míjení
soupeře, nikoli pouze návratový kód programu. Běžné herní EXE se při auditu neměnilo.

| Scénář | Pozorovaný výsledek |
|---|---|
| Mulholland Enhanced / Classic | Oba závody dokončeny, výhra; přibližně 123 / 107 s od spuštění do výsledku. |
| Akvadukt Enhanced / Classic | Oba závody dokončeny, výhra; přibližně 65 / 64 s do výsledku. |
| Drag proti Kingovi | Prohra, korektní výsledkový dialog a návrat do města. |
| Přejezd městem | Dokončen bez pádu programu. |
| Mulholland s limitem 120 FPS | 28sekundový běh, 666 snímků pro hledání náhlých změn obrazu. |
| Skriptovaný plyn, pozdní řazení | Přetočení a zničení motoru (`result 0005`), odpovídá dosavadním pravidlům. |
| Skriptovaný plyn, včasné řazení | Náraz do soupeře a dialog opravy; plánované vyjetí z vozovky už nenastalo. |
| Řízené vyjetí z útesu | Samostatná diagnostická kopie posunula auto za skutečný záporný svah v segmentu 385. Návrat bez havárie, odpočet 3–2–1, rozjezd 25 mph. |

Při testu útesu proběhla penalizace od fyzikálního kroku 61 do 79, tedy přesně
18 kroků. Soupeř pokračoval z polohy 384 + 59/64 na 394 + 27/64. Snímky potvrdily
zobrazení a odstranění odpočtu i pokračování obrazu po návratu. Diagnostická
úprava je pouze v ignorované složce `work`, není součástí produkčních zdrojů.

**Potvrzený zbývající artefakt:** při míjení soupeř viditelně cukne v okamžiku
přepnutí zadního pohledu na boční. Doklad je dvojice `snap0256.bmp` /
`snap0257.bmp` v `work/race-audit/mulholland-passing`, přehled
`work/race-audit/opponent-switch.png`. `enh_opponent()` v `enhanced/sprites.c`
předává boční pohledy původnímu rendereru; `opponent_draw()` v `game/race_opp.c`
pak používá jiné kotevní body a jiný způsob ořezu. Plynulé přepočítávání zadního
spritu proto samo o sobě tento přechod nevyřeší. Další oprava má sjednotit
polohu a měřítko při změně pohledu; samotné přepnutí kresby auta zůstane dáno
počtem původních úhlových variant. Jde o grafický nedostatek, nikoli prokázaný
posun soupeře ve fyzice. V tomto auditu se jeho vykreslování neupravovalo.

Na zkontrolovaných snímcích se neprojevil rozpad palety ani překreslení kokpitu
nebo výsledkových dialogů vozovkou. Při dokončených jízdách nenastal pád procesu.
Regresní sady vykreslování (755 překreslení), obtížnosti i zvuku prošly; první
spuštění audio testu ze složky `build` nemohlo vytvořit WAV, opakování v izolované
pracovní složce prošlo. Běžné běhy držely zhruba 60 FPS. Husté ukládání BMP při
120 FPS způsobovalo měřitelné propady, takže tento diagnostický běh není
vhodný pro posuzování výkonu samotného rendereru.

Spouštěcí skripty, logy a obrazové podklady jsou v `work/race-audit*`.
Audit nepokrývá všechny modely aut, uložené hry ani všechny možné kontakty.
Poslech na fyzickém zvukovém zařízení a pocit z ručního řízení se zde neověřovaly.

## Oprava útesu před posledním mostem — 2026-11-17

Na variantě Mulhollandu 6 (most začíná segmentem 949) se při přiblížení přes výhled
krátce roztáhla hnědá plocha. Enhanced renderer použil efekt siluety vzdáleného
výjezdu z mostu také u blízkého vjezdu. Jeho původní tabulka obsahuje záporné šířky
`-20` a `-2000`; interpolace tak obrátila vrcholy útesu přes vozovku.

Enhanced nyní v těchto místech kreslí skutečně promítnuté plochy svahu v celé
dohlednosti a zachovává uzavření opěry na výjezdu z mostu. Classic ponechává původní
větev. Pixelový regresní test ověřuje, že svah se zápornými hodnotami původní
tabulky zůstane vlevo a nezasáhne vozovku ani prostor mimo výhled.

Cílený diagnostický průjezd od segmentu 914 přes most a cíl byl porovnán před
a po opravě v husté sekvenci snímků; záblesk po opravě zmizel. Podklady jsou
v `work/bridge-audit`, srovnání v `bridge-before-after.png`. Diagnostický přesun
auta je pouze v dočasné testovací kopii, běžné EXE ho neobsahuje.

Následoval celý simulovaný Mulholland (`SR_SEED=1`), včetně dalšího mostu a jeho
výjezdu: dokončen výhrou, návrat do města bez pádu. Kontrola zachycených snímků
neukázala opakování opraveného záblesku. Prošly i regresní sady vykreslování
(755 překreslení), obtížnosti včetně nového pixelového testu a zvuku.
