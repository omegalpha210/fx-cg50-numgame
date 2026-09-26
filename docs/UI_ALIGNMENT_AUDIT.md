# UI alignment audit — beta.5 actual renderer

This audit uses the common 396×224 RGB565 renderer, not a mockup or a
calculator photograph. The 36-game [contact sheet](captures/contact-native.png),
354 individual native captures, 108 RULES top/middle/bottom captures, and the
host bounds test are the evidence. The game viewport stays y25–203, with
header y0–23 and F-key strip y204–223. Game-specific board and clue regions
are deliberately not forced into one global coordinate system. Text and
rectangles are integer-pixel rasterized; the host drawing backend asserts
framebuffer bounds. The contact sheet and the focused sheets were visually
reviewed; this is not a claim that every possible gameplay state or the physical
LCD has been inspected.

Four gameplay alignment findings were corrected: Sequence's two rows and
reading route; Make Target and Countdown's shared card row; and Prime Factor's
target numeral. The shared RULES finding affects all 36 games. Other game
boards, timers, status panels, dialogs, icons, selectors and softkey positions
showed no additional clipping or obvious alignment defect in the captured
representative states. Their gameplay geometry is unchanged.

- **RULES:** old text x10/width376 had no position display and accepted up to
  100 DOWN presses even past the final line. Now 13 short documents retain
  width376 with no rail and max offset0. The other 23 use width350, a 5px
  rail at x375–379/y49–178, arrows at y34–37 and y187–190, and a thumb
  proportional to the 10-line viewport. UP at top and DOWN at bottom are
  light gray; available directions are blue. Actual wrapped lines determine
  the maximum offset. Two 11-line documents (IDs8 and12) exercise the
  one-line-over case; Sequence is longest at 23 lines/max offset13.
- **Sequence:** cards were x12/138/264 (116×32), leaving margins12/16. They
  are now x14/140/266, margins14/14, at y51 and y90. Small horizontal arrows
  mark 1→2→3 and 4→5→6; the 3→4 route stays outside the cards at x389,
  crosses the 7px row gap at y86, and points into card4 from x7–12.
- **Make Target / Countdown:** with six cards the old x14–377 row used 53px
  cards and 9px gaps (margins14/19). The shared renderer now uses 55px cards,
  7px gaps and x15–380 (margins15/16). The normal font's three-digit width at
  scale2 is 46px, so `999` stays centered inside a 55px card without touching
  its border. Four- and five-card rows are centered from their actual total
  width too. [Six card-width fixtures](captures/cards-three-digit-native.png)
  show 1, 9, 10, 99, 100 and 999.
- **Prime Factor:** the old target started at fixed x135/y66/scale3 regardless
  of numeral width. The new target area is x88–307/y55–102. The renderer
  measures actual glyph width, chooses scale4 when it fits (otherwise scale3),
  and centers x/y by the measured width and 11×scale height. The six-digit
  `488808` is 188px at scale4 and fits with 16px on each side of its 220px
  target area. [Focused captures](captures/factor-centering-native.png) cover
  9, 360, 43956, 157626 and 488808.

In the table, “shared RULES” means the conditional width, rail and clamped
offset above. “Gameplay retained” means the beta.4 per-game board geometry,
not an unexamined redesign. Each row links to its current gameplay and RULES
renderer frames.

| ID / game | Screen | Finding | Before geometry | After geometry | Change | Reason |
|---|---|---|---|---|---|---|
| 01 NUMBER BASEBALL | [play](captures/01-play.png), [RULES](captures/69-rules-01-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 10 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 02 EQUATION GUESS | [play](captures/02-play.png), [RULES](captures/69-rules-02-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 15 lines / max5 | Shared RULES clamp/indicator | Long text position |
| 03 NUMBER MIND | [play](captures/03-play.png), [RULES](captures/69-rules-03-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 9 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 04 CLUE LOCK | [play](captures/04-play.png), [RULES](captures/69-rules-04-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 10 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 05 SEQUENCE DETECTIVE | [play](captures/05-play.png), [RULES](captures/69-rules-05-top.png) | Card row 2px left; 3→4 unclear; RULES scroll beyond end / no position | Cards x12/138/264; RULES x10/w376; max input cap100 | Cards x14/140/266 + outer route; RULES w350 + rail; 23 lines / max13 | Centered rows, arrow route, shared RULES | Read six terms in order without crossing cards |
| 06 MAKE TARGET | [play](captures/06-play.png), [RULES](captures/69-rules-06-top.png) | Six-card row 2.5px left; RULES scroll beyond end / no position | Six cards x14–377, w53/gap9; RULES x10/w376; max input cap100 | Six cards x15–380, w55/gap7; RULES w350 + rail; 19 lines / max9 | Centered card row, shared RULES | Equal gaps and 3-digit scale2 fit |
| 07 COUNTDOWN | [play](captures/07-play.png), [RULES](captures/69-rules-07-top.png) | Six-card row 2.5px left; RULES scroll beyond end / no position | Six cards x14–377, w53/gap9; RULES x10/w376; max input cap100 | Six cards x15–380, w55/gap7; RULES w350 + rail; 15 lines / max5 | Centered card row, shared RULES | Equal gaps and symmetric margins |
| 08 MISSING OPERATORS | [play](captures/08-play.png), [RULES](captures/69-rules-08-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 11 lines / max1 | Shared RULES clamp/indicator | Long text position |
| 09 CROSS MATH | [play](captures/09-play.png), [RULES](captures/69-rules-09-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 12 lines / max2 | Shared RULES clamp/indicator | Long text position |
| 10 PRIME FACTOR | [play](captures/10-play.png), [RULES](captures/69-rules-10-top.png) | Target fixed-x off-center; RULES scroll beyond end / no position | Target x135/y66/scale3; RULES x10/w376; max input cap100 | Measured target area x88–307/y55–102; RULES w350 + rail; 13 lines / max3 | Measured numeral center, shared RULES | Large target remains within panel |
| 11 SUDOKU | [play](captures/11-play.png), [RULES](captures/69-rules-11-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 10 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 12 CALCUDOKU | [play](captures/12-play.png), [RULES](captures/69-rules-12-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 11 lines / max1 | Shared RULES clamp/indicator | Long text position |
| 13 KAKURO | [play](captures/13-play.png), [RULES](captures/69-rules-13-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 9 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 14 FUTOSHIKI | [play](captures/14-play.png), [RULES](captures/69-rules-14-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 9 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 15 SKYSCRAPERS | [play](captures/15-play.png), [RULES](captures/69-rules-15-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 9 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 16 HITORI | [play](captures/16-play.png), [RULES](captures/69-rules-16-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 7 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 17 BINARY PUZZLE | [play](captures/17-play.png), [RULES](captures/69-rules-17-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 6 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 18 NUMBRIX | [play](captures/18-play.png), [RULES](captures/69-rules-18-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 7 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 19 MAGIC SQUARE | [play](captures/19-play.png), [RULES](captures/69-rules-19-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 9 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 20 SUM GRID | [play](captures/20-play.png), [RULES](captures/69-rules-20-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 7 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 21 NIM | [play](captures/21-play.png), [RULES](captures/69-rules-21-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 22 WYTHOFF | [play](captures/22-play.png), [RULES](captures/69-rules-22-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 23 EUCLID | [play](captures/23-play.png), [RULES](captures/69-rules-23-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 24 MAKE FIFTEEN | [play](captures/24-play.png), [RULES](captures/69-rules-24-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 25 RACE TO TARGET | [play](captures/25-play.png), [RULES](captures/69-rules-25-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 26 2048 | [play](captures/26-play.png), [RULES](captures/69-rules-26-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 14 lines / max4 | Shared RULES clamp/indicator | Long text position |
| 27 SLIDING PUZZLE | [play](captures/27-play.png), [RULES](captures/69-rules-27-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 28 LIGHTS OUT | [play](captures/28-play.png), [RULES](captures/69-rules-28-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 15 lines / max5 | Shared RULES clamp/indicator | Long text position |
| 31 SHIKAKU | [play](captures/31-play.png), [RULES](captures/69-rules-31-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 13 lines / max3 | Shared RULES clamp/indicator | Long text position |
| 32 SLITHERLINK | [play](captures/32-play.png), [RULES](captures/69-rules-32-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 14 lines / max4 | Shared RULES clamp/indicator | Long text position |
| 33 BLACK BOX | [play](captures/33-play.png), [RULES](captures/69-rules-33-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 18 lines / max8 | Shared RULES clamp/indicator | Long text position |
| 34 CRYPTARITHM | [play](captures/34-play.png), [RULES](captures/69-rules-34-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 17 lines / max7 | Shared RULES clamp/indicator | Long text position |
| 35 HASHI | [play](captures/35-play.png), [RULES](captures/69-rules-35-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 16 lines / max6 | Shared RULES clamp/indicator | Long text position |
| 36 NONOGRAM | [play](captures/36-play.png), [RULES](captures/69-rules-36-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w376, no rail; 8 lines / max0 | Shared RULES clamp/indicator | No unnecessary rail |
| 37 REVERSI | [play](captures/37-play.png), [RULES](captures/69-rules-37-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 14 lines / max4 | Shared RULES clamp/indicator | Long text position |
| 38 NET | [play](captures/38-play.png), [RULES](captures/69-rules-38-top.png) | RULES scroll beyond end / no position | RULES x10/w376; max input cap100 | RULES w350 + rail; 16 lines / max6 | Shared RULES clamp/indicator | Long text position |

The host test checks all 36 actual wrapped texts, top/middle/bottom arrow
colors, top-UP/bottom-DOWN clamps, held-key steps, offset reset on new RULES,
six equal card boxes, and Prime target ink center. The gameplay renderer
capture test covers entry, action, RULES and EXIT flows across the registry.
Physical LCD legibility, save latency, dim/APO and the previously reported
MENU flashing remain **HARDWARE TEST REQUIRED**; see
[HARDWARE_RETEST.md](HARDWARE_RETEST.md).
