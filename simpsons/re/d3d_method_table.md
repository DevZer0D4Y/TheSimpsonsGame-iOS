# The game's D3D command-writer method table

Extracted from live guest memory (image data at `0x821F4274`, read passively
from the running game). This is the game's dispatch table mapping D3D-level
rendering operations to the handler functions that encode them as PM4 GPU
command packets -- i.e. **the complete API boundary between "the game decides
what to draw" and "GPU command buffer bytes."** Every draw, state change,
texture bind, resolve and swap the game ever performs goes through exactly one
of these 73 handlers.

Why this matters: these are the hook points for draw batching, and the exact
seam where a native renderer replaces the emulated GPU path -- reimplement
these 73 functions natively and the PM4/EDRAM emulation layer below them is
no longer on the hot path. All 73 are verified present in the recompiled
function table (nothing the renderer calls was missed by analysis).

`meta` is the table's per-entry descriptor word (0x4000XXYY -- XX appears to
encode required command-buffer space, YY an argument/type field; the two known
draw entries share meta 0x40005602, consistent with same-signature variants).

Identified so far:
- entry with fn `0x82462810`: draw path A (writes PM4 DRAW_INDX; prim type in
  r4, index count in r5) -- instrumented by `src/draw_telemetry.cpp`
- entry with fn `0x824646E8`: draw path B (same encoding) -- instrumented

Remaining entries are unlabeled until their handler bodies are examined (each
handler's PM4 register writes identify the operation -- e.g. texture fetch
constant writes = SetTexture, render state register = SetRenderState).

| # | meta | handler | source |
|---|------|---------|--------|
| 0 | `0x4000BD05` | `sub_82461500` | `simpsons_recomp.17.cpp:29572` |
| 1 | `0x40007003` | `sub_824616C0` | `simpsons_recomp.17.cpp:29829` |
| 2 | `0x40004603` | `sub_824617D8` | `simpsons_recomp.17.cpp:29988` |
| 3 | `0x40002305` | `sub_82461868` | `simpsons_recomp.17.cpp:30075` |
| 4 | `0x40001304` | `sub_824618B8` | `simpsons_recomp.17.cpp:30123` |
| 5 | `0x40000F04` | `sub_824618F8` | `simpsons_recomp.17.cpp:30162` |
| 6 | `0x40003705` | `sub_824619D8` | `simpsons_recomp.17.cpp:30293` |
| 7 | `0x4000A903` | `sub_82461C88` | `simpsons_recomp.17.cpp:30681` |
| 8 | `0x40001B04` | `sub_82461CF8` | `simpsons_recomp.17.cpp:30746` |
| 9 | `0x40001404` | `sub_82461D48` | `simpsons_recomp.17.cpp:30795` |
| 10 | `0x40001804` | `sub_82461F28` | `simpsons_recomp.17.cpp:31117` |
| 11 | `0x40003905` | `sub_82462090` | `simpsons_recomp.17.cpp:31334` |
| 12 | `0x40004003` | `sub_82462190` | `simpsons_recomp.17.cpp:31478` |
| 13 | `0x40001803` | `sub_82462288` | `simpsons_recomp.17.cpp:31647` |
| 14 | `0x40001A05` | `sub_824623A0` | `simpsons_recomp.17.cpp:31808` |
| 15 | `0x40003B03` | `sub_82462490` | `simpsons_recomp.17.cpp:31944` |
| 16 | `0x40004A03` | `sub_824625B8` | `simpsons_recomp.17.cpp:32125` |
| 17 | `0x40004005` | `sub_824626B8` | `simpsons_recomp.17.cpp:32281` |
| 18 | `0x40005602` | `sub_82462810` **DRAW** | `simpsons_recomp.17.cpp:32459` |
| 19 | `0x40006D02` | `sub_824629C8` | `simpsons_recomp.17.cpp:32686` |
| 20 | `0x40006803` | `sub_82462B68` | `simpsons_recomp.17.cpp:32922` |
| 21 | `0x40001803` | `sub_82462BC8` | `simpsons_recomp.17.cpp:32980` |
| 22 | `0x40002103` | `sub_82462D50` | `simpsons_recomp.17.cpp:33204` |
| 23 | `0x40006802` | `sub_82462EF0` | `simpsons_recomp.17.cpp:33428` |
| 24 | `0x40005102` | `sub_82463038` | `simpsons_recomp.17.cpp:33600` |
| 25 | `0x4000CD03` | `sub_82463370` | `simpsons_recomp.17.cpp:34059` |
| 26 | `0x40004903` | `sub_82463498` | `simpsons_recomp.17.cpp:34229` |
| 27 | `0x4000C503` | `sub_824637B0` | `simpsons_recomp.17.cpp:34700` |
| 28 | `0x40003B03` | `sub_824638A0` | `simpsons_recomp.17.cpp:34843` |
| 29 | `0x40002204` | `sub_82463978` | `simpsons_recomp.17.cpp:34966` |
| 30 | `0x40005C05` | `sub_82463AE8` | `simpsons_recomp.17.cpp:35172` |
| 31 | `0x40006603` | `sub_82463C80` | `simpsons_recomp.17.cpp:35415` |
| 32 | `0x40005403` | `sub_82463DD0` | `simpsons_recomp.17.cpp:35608` |
| 33 | `0x40002303` | `sub_82463E60` | `simpsons_recomp.17.cpp:35699` |
| 34 | `0x4000A503` | `sub_824640F8` | `simpsons_recomp.17.cpp:36074` |
| 35 | `0x40003705` | `sub_824641D8` | `simpsons_recomp.17.cpp:36199` |
| 36 | `0x40002301` | `sub_82464268` | `simpsons_recomp.17.cpp:36292` |
| 37 | `0x40006B03` | `sub_824644A0` | `simpsons_recomp.17.cpp:36635` |
| 38 | `0x40003B03` | `sub_82464590` | `simpsons_recomp.17.cpp:36771` |
| 39 | `0x40005602` | `sub_824646E8` **DRAW** | `simpsons_recomp.17.cpp:36949` |
| 40 | `0x40006D02` | `sub_824648A0` | `simpsons_recomp.17.cpp:37176` |
| 41 | `0x40004005` | `sub_824649A0` | `simpsons_recomp.17.cpp:37332` |
| 42 | `0x40004A03` | `sub_82464AC8` | `simpsons_recomp.17.cpp:37513` |
| 43 | `0x40006C03` | `sub_82464C78` | `simpsons_recomp.17.cpp:37760` |
| 44 | `0x40003104` | `sub_82464D40` | `simpsons_recomp.17.cpp:37877` |
| 45 | `0x40001603` | `sub_82464D98` | `simpsons_recomp.17.cpp:37932` |
| 46 | `0x4000FB03` | `sub_82465188` | `simpsons_recomp.17.cpp:38497` |
| 47 | `0x4000BD03` | `sub_82465480` | `simpsons_recomp.17.cpp:38941` |
| 48 | `0x40009503` | `sub_824656D8` | `simpsons_recomp.17.cpp:39303` |
| 49 | `0x40002005` | `sub_82465758` | `simpsons_recomp.17.cpp:39379` |
| 50 | `0x40002803` | `sub_824657F8` | `simpsons_recomp.17.cpp:39475` |
| 51 | `0x40003303` | `sub_82465948` | `simpsons_recomp.17.cpp:39681` |
| 52 | `0x40007403` | `sub_82465C18` | `simpsons_recomp.17.cpp:40159` |
| 53 | `0x4000180A` | `sub_82465CB0` | `simpsons_recomp.17.cpp:40253` |
| 54 | `0x4000200A` | `sub_82465D30` | `simpsons_recomp.17.cpp:40329` |
| 55 | `0x40009A03` | `sub_82465FC0` | `simpsons_recomp.17.cpp:40709` |
| 56 | `0x40001D03` | `sub_82466038` | `simpsons_recomp.17.cpp:40781` |
| 57 | `0x40003303` | `sub_82466108` | `simpsons_recomp.17.cpp:40901` |
| 58 | `0x40009F03` | `sub_82466388` | `simpsons_recomp.17.cpp:41261` |
| 59 | `0x40002804` | `sub_82466450` | `simpsons_recomp.17.cpp:41381` |
| 60 | `0x40000A03` | `sub_824667C0` | `simpsons_recomp.17.cpp:41892` |
| 61 | `0x40000F03` | `sub_82466800` | `simpsons_recomp.17.cpp:41930` |
| 62 | `0x40009A03` | `sub_82466A68` | `simpsons_recomp.17.cpp:42276` |
| 63 | `0x40008F03` | `sub_82466CA8` | `simpsons_recomp.17.cpp:42607` |
| 64 | `0x40002703` | `sub_82466D48` | `simpsons_recomp.17.cpp:42706` |
| 65 | `0x40001604` | `sub_82466DA0` | `simpsons_recomp.17.cpp:42761` |
| 66 | `0x40002A04` | `sub_82466E48` | `simpsons_recomp.17.cpp:42859` |
| 67 | `0x40001E04` | `sub_82466EC0` | `simpsons_recomp.17.cpp:42929` |
| 68 | `0x40001B04` | `sub_82466F30` | `simpsons_recomp.17.cpp:42992` |
| 69 | `0x40008705` | `sub_82467150` | `simpsons_recomp.17.cpp:43306` |
| 70 | `0x40001804` | `sub_824671B0` | `simpsons_recomp.17.cpp:43366` |
| 71 | `0x40006F03` | `sub_82467450` | `simpsons_recomp.17.cpp:43782` |
| 72 | `0x40005B04` | `sub_824675C8` | `simpsons_recomp.17.cpp:44001` |
