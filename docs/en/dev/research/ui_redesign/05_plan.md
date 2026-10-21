# 05. Implementation plan

Phased, ordered milestones. Effort is for one experienced developer working on this alone
(S ≤ 1 week, M 2-3 weeks, L 4-6 weeks, XL > 6 weeks); parallel streams are noted where a
second person helps. "Files" are the likely touch points; new code lives under `src/ui/`.
Every milestone ends with the full build of `cataclysm-bn-tiles` and `cata_test-tiles`, the
`[r3d]` test run with `CATA_TEST_COMPUTE_ACCELERATION=cpu`, and the screenshot suite.

Sequencing constraints set by the user: the 3D sidebar removal (M3) happens only after the
HUD design in `03_design.md` §4.1 and §3.1 is approved, and with no stopgap HUD in between.
Decisions of 2026-10-21 folded in: curses is retired (`BACKLOG.md` UI-02; milestone M1.5 and
phase 3 inside M11; costs in `06_frontend_options.md` §4), the frontend stays SDL3 unless the
decision gate at the end of M1 fails (`06_frontend_options.md` §3), and the Tutorial and
Guided Play merge into one guided start (UI-01; milestone M9.5).

{{svg:milestones}}

## Smallest valuable slice

**M1 + M2:** the widget layer with the Settings screen and the Controls editor, and real
gamepad support. It ships remapping of every device (fixing a verified defect: gamepad
buttons do not work today), the command palette, UI scale and the narrator skeleton, without
touching a single gameplay screen. It is the foundation every later screen needs, and it is
testable on its own.

## M0. Decisions and spike (S, 1 week)

Scope: get answers to the open questions below; spike the `ui::node` tree with one screen
(Settings) on the SDL backend; measure text-engine glyph quality at 75/100/200 % and on a
1280x800 window; build the `accesskit-c` 0.23.1 `examples/sdl` program and attach a minimal
tree from the spike screen, then try it with Orca (here), NVDA and VoiceOver (testers).
Files: `src/ui/` (spike), `CMakeLists.txt` (pin SDL3_ttf tag; optional `accesskit-c` prebuilt).
→ verify: the spike screen renders; a scale change needs no restart; screenshot at 1280x800
has no glyph under 9 px; at least one screen reader reads the spike tree (gate input for M1).
Risks: SDL3_ttf text engine behaviour on the software renderer; `accesskit-c` adapters are
documented as best-effort.

## M1. Foundation (L, 4-6 weeks; two streams)

Scope: `ui::node`, layout (box/flex, wrapping text), focus manager with spatial navigation,
`menu` input context, theme tokens, SDL backend on `SDL_Renderer` + `TTF_TextEngine`,
widgets (panel, stack, tabs, list, text, button, toggle, choice, slider, text field,
progress, card), narrator core with a no-op speech backend, screenshot and transcript test
harness. Deliver the **Settings screen** (39 options, six tabs) and **Reference** skeleton.
Stream A: tree, layout, backend. Stream B: widgets, Settings screen, tests.
Files: `src/ui/*`, `src/ui_manager.*` (register new adaptors), `src/options.cpp` (expose a
curated list), `src/sdltiles.cpp` (present order), `tests/ui_*_test.cpp`, `tests/data/ui_snapshots/`.
Depends on: M0.
→ verify: Settings opens from the pause menu; keyboard, mouse (hit-tested) and D-pad navigate
it; snapshot tests pass at 100 % and 200 %; narrator transcript test speaks "Settings. 6
tabs." then each focused row.
→ **Frontend decision gate** (`06_frontend_options.md` §3): the Settings tree is exposed
through `accesskit-c` and read by at least two of NVDA, VoiceOver, Orca. Pass: continue on
SDL3. Fail after two weeks of effort: stop, re-cost option B (Godot) with the spike's
findings, and get a user decision before M2.
Risks: layout engine scope creep (keep to box/flex, adopt Yoga only on demonstrated need).

## M1.5. Curses retirement, phase 1 (S, 1 week; parallel with M2)

Scope (`06_frontend_options.md` §4): delete `src/ncurses_def.cpp/.h` and `src/wincurse.cpp`;
remove the `CURSES` option, `find_package(Curses)`, the macOS ncursesw bundling and the
`tests/CMakeLists.txt` branch; delete the `linux-curses`, `dist-curses`, `osx-curses-*-dist`
presets and the four curses CI jobs; drop the `cata_test` (curses) target; update the building
guides, `ui_overhaul.md` and README. Keep `cursesport.cpp` and `cursesdef.h` (legacy screens
still draw through them) and leave `#if defined(TILES)` guards in place in files this fork
has not otherwise changed.
Files: `CMakeLists.txt:33, 207-212, 646-660, 893-934`, `src/CMakeLists.txt:208-230`,
`tests/CMakeLists.txt:37`, `CMakePresets.json`, `.github/workflows/{build,matrix,release}.yml`,
`docs/en/dev/guides/building/*`, `docs/en/dev/guides/ui_overhaul.md`, `README.md`.
Depends on: M0 (user decision recorded).
→ verify: `cmake --preset linux-full` and the MSVC preset configure; `rg CURSES CMakeLists.txt
src/CMakeLists.txt tests/CMakeLists.txt CMakePresets.json .github/workflows` is empty; both
binaries build; the full test run is unchanged; `--check-mods` passes.
Risks: upstream merge friction in `CMakeLists.txt` and workflows (fork-only decision; see
open question 1); the Android build must still configure (it uses the tiles path).

## M2. Input: gamepad, controls editor, palette (M, 2-3 weeks)

Scope: `SDL_Gamepad` migration with hot-plug, sticks, rumble, labels; `input_event`
modifiers and device; context parent chain and the collapse to 12 contexts (legacy contexts
mapped); Controls editor (search, three columns, capture with modifiers and middle mouse,
conflicts, per-action hold/toggle, presets incl. Left-handed and One-handed); command
palette; glyphs for the last device; on-screen keyboard for text entry.
Files: `src/sdltiles.cpp` (event loop 1871-1935, 3277-3510), `src/input.h/.cpp`
(`display_menu` replaced), `src/action.cpp`, `data/raw/keybindings/*.json` (context
restructure, gamepad names), `src/ui/screens/controls.cpp`, `tests/input_test.cpp`.
Depends on: M1.
→ verify: a gamepad's face buttons trigger actions (today they do not); `Ctrl+Shift+K` and
middle mouse are capturable; rebinding movement keys does not break menu navigation (CDDA
#45927 scenario as a test); the palette lists and runs every action; Deck rule "default config
reaches all content" checked by a test that walks the play context.
Risks: legacy context names used by mods and Lua (`input_context` ids); keep aliases.

## M3. HUD and sidebar removal in 3D mode (L, 4-5 weeks; after HUD approval)

Scope: implement §4.1 of the design on the new layer: vitals strip with body figure, stamina
and move mode; badge row with icons, words and trend; message ticker; hotbar (display only
until M5 binds items); interaction prompt with primary verb and hold-list; safe-mode state;
clock on the compass; edge-arrow chevrons and class glyphs; vehicle cluster; HUD modes Full /
Minimal / Off; "Reduce motion" behaviour for fades and hit glow. Then remove the sidebar in
3D: `ui::sidebar_hidden()`, panel widths 0, `draw_panels` early return, terrain window at
full width, re-anchor the five sidebar-relative placements, drop the pixel minimap in 3D,
make `}` a no-op with a message, keep saved layouts loading for classic.
Files: `src/r3d_hud.cpp/.h` (badges, arrows, compass remain; texts move to the new layer),
`src/sdltiles.cpp:1611-1700`, `src/game.cpp:4690-4730, 4946-5000, 9576, 10082-10107,
10531-10539`, `src/panels.cpp/.h` (`get_width_*`, `show_adm`), `src/pixel_minimap.cpp`,
`src/status_report.cpp` (add vehicle and noise sentences), `src/options.cpp` (HUD mode,
Minimap), `src/ui/hud/*`, `tests/r3d_hud_test.cpp`, `tests/ui_hud_test.cpp`.
Depends on: M1 (layer), M2 (prompt glyphs). Parallel with M4.
→ verify: in 3D the picture spans the window and no panel is drawn; every sidebar item in the
§3.1 table is visible on the HUD, in `@`, or in F4 (a test walks the table: for each item,
assert the HUD text or the F4 sentence); classic 2D is pixel-identical to before (snapshot);
`[r3d]` passes; geometry snapshot unchanged; Xvfb smoke with a private save;
interrupt popup still names the creature; Lua sidebar widgets appear as badges.
Risks: hidden consumers of `TERRAIN_WINDOW_WIDTH` and sidebar width (grep lists 7 sites in
`game.cpp`; there may be more in `overmap_ui.cpp`, `pickup.cpp`, `advanced_inv.cpp` via
`PICKUP_POSITION`/`LOOKAROUND_POSITION`); runtime toggle of the renderer must re-run the
resize callback; message visibility regression if the ticker lands later than the removal
(they ship in the same milestone by rule).

## M4. Advanced config file and options migration (M, 2 weeks; parallel with M3)

Scope: `config/advanced.json` writer/reader with `"//key"` descriptions, validation and
warnings, reload points, `--dump-advanced-options` generating `docs/en/user/advanced_options.md`,
one-time migration from `options.json`, "Reset advanced settings" verb, world-option split
(8 in the New game screen, rest in `worldoptions.json`).
Files: `src/options.cpp/.h` (tag each option `ui | advanced | world_ui | world_file`),
`src/main.cpp` (CLI flag), `docs/en/user/advanced_options.md` (generated), `tests/options_test.cpp`.
Depends on: M1 (Settings screen shows the curated set).
→ verify: fresh install creates the file with every advanced key and description; a bad value
logs a warning and uses the default; editing the file and closing Settings applies non-restart
keys; the generated doc lists the same keys as the file; the Settings screen shows exactly 39
options (test counts them).

Where each current option goes (by rule; the per-option list is produced by the tagging
commit and checked by the count test):

| Current page | In-game (Settings tab) | Advanced file | Elsewhere |
|---|---|---|---|
| general (5) | Master/Music/Effects/Ambient (Audio) | `SOUNDPACKS` | |
| interface (57) | `USE_LANG`, Units (merges 5), Confirmations (merges 3), Safe mode, Auto-pickup | the other 45 (`MESSAGE_*`, `SIDEBAR_*`, `AIM_*`, `*_POSITION`, `MENU_SCROLL`, `EDGE_SCROLL`, `HIDE_CURSOR`, vehicle view tweaks, ...) | `ENABLE_JOYSTICK` → Controls: Gamepad; `CLOSE_ADV_INV`, `OPEN_DEFAULT_ADV_INV`, `NEW_PICKUP_MENU`, `MORALE_STYLE`, `HEALTH_STYLE`, `ITEM_SYMBOLS` removed with their screens |
| graphics (74) | Renderer (2D / 3D; `USE_TILES` is removed with curses), Tileset, Window, HUD, Minimap, Camera, FOV, Weather effects, Cutaway | `ANIMATION_*`, `PIXEL_MINIMAP_*`, `R3D_FP_*` tuning, `R3D_VIEW_DEPTH`, `R3D_LEVEL_HEIGHT`, `R3D_THIN_WALLS`, `R3D_WALL_TEXTURES`, `R3D_VOXEL_ITEMS`, `R3D_AVATAR_SILHOUETTE`, `TEXTURE_STREAMING`, `RENDER_BATCHING`, `FRAMEBUFFER_ACCEL`, `VSYNC`, `SCALING_FACTOR`, `COMPUTE_ACCELERATION`, `TERMINAL_X/Y`, night-vision colours, `MEMORY_MAP_MODE`, loading-screen options | `MODERN_MENUS` → theme file; `R3D_HUD` → HUD mode; `R3D_WASD` → movement preset; `R3D_FP_COMPASS/EDGE_ALERT/CROSSHAIR` → HUD mode |
| font page (3) | Readable font (Accessibility), UI scale (Accessibility) | `FONT_*`, `USE_DRAW_ASCII_LINES_ROUTINE`, `FONT_BLENDING` | |
| world_default (47) | New game > World: `CITY_SIZE`, `SPAWN_DENSITY`, `NPC_DENSITY`, `SEASON_LENGTH`, `MONSTER_UPGRADE_FACTOR`, `VEHICLE_SPAWNRATE`, `ITEM_SPAWNRATE`, `INITIAL_DAY` | | the other 39 in the world's `worldoptions.json`, documented |
| debug (24) | none | all | |
| android (43) | none | all (Android build only) | |

## M5. Inventory (XL, 6-8 weeks; two streams)

Scope: the unified Inventory screen in four modes; item card with summary, comparison,
full info; sort with secondary key; category tabs; search that looks inside containers;
chips (can use, needed, junk); marks and bulk move; worn order editing; quick-slot
assignment; replace `game_menus::inv::*` call sites with Pick mode; loot mode from the prompt;
trade mode from dialogue.
Stream A: view model (grouping, sort, search, comparison deltas) with pure tests. Stream B:
screen and call-site migration.
Files: `src/inventory_ui.*`, `src/game_inventory.*`, `src/advanced_inv*.cpp`, `src/pickup.cpp`,
`src/examine_item_menu.cpp`, `src/armor_layers.cpp`, `src/reload/reload_ui.cpp`,
`src/trade_win.*`, `src/item.cpp` (`info()` sectioning for the card), `src/output.cpp`
(`draw_item_info` consumers), `src/ui/screens/inventory*.cpp`, `tests/inventory_ui_test.cpp`.
Depends on: M1, M2, M3 (hotbar).
→ verify: every former call site opens the new screen in the right mode (a test enumerates
`game_menus::inv` entry points); sort is stable; search finds "apple juice" inside a bottle
(BN #7280 scenario); comparison deltas match `item::info` numbers; drag-and-drop and keyboard
produce the same result; transcript test for a loot sequence; snapshots at 200 %.
Risks: `inventory_selector` presets are used by many call sites with custom predicates; keep
the preset interface and feed it to the new model. Trade mode touches NPC economy code; keep
`trade_win` logic, replace only presentation.

## M6. Character, Journal, Map (L, 4-6 weeks; three parallel streams)

Scope: Character (5 tabs, body figure, morale reasons, effects), Journal (Log, Missions,
Lessons, Notes, Records), Map (layers, verbs, notes count and delete-in-view).
Files: `src/character_display.cpp`, `src/mutation_ui.cpp`, `src/bionics_ui.cpp`,
`src/perk_menu.cpp`, `src/morale.cpp`, `src/messages.cpp`, `src/mission_ui.cpp`,
`src/diary_ui.cpp`, `src/scores_ui.cpp`, `src/onboarding.cpp` (checklist into Journal),
`src/overmap/overmap_ui.cpp`, `src/ui/screens/{character,journal,map}*.cpp`.
Depends on: M1, M2.
→ verify: each legacy key (`@ [ p Ctrl+P v P M Home ) m`) lands in the right tab; the F4
report and the Status tab agree on every number (test); map notes count matches the Notes
tab; snapshots; transcripts.
Risks: overmap drawing is cell-based (`overmap_ui.cpp` 2312 lines); keep its map drawing and
replace only the chrome, or the milestone doubles.

## M7. Craft, Build, Cursor mode (L, 4-6 weeks; two streams)

Scope: Craft with filters, tabs, search, component alternatives, pin; Build with Construct
(placement stays open, copy-this, remembered selection) and Zones; Cursor mode merging look,
target, throw, list items/monsters with the tile card and narrator reading.
Files: `src/crafting_gui.cpp`, `src/construction.cpp`, `src/clzones.cpp`, `src/game.cpp`
(`look_around` 10061-10200, zones manager 9548), `src/ranged.cpp` (targeting UI chrome),
`src/salvage.cpp`, `src/ui/screens/{craft,build,cursor}*.cpp`.
Depends on: M5 (Pick mode for components).
→ verify: "can craft" filter matches `recipe::can_make`; unknown recipes are counted not named;
construction tab/selection survives reopen (BN #8299 scenario); cursor card transcript on a
scripted scene; targeting still fires through the same `target_handler` code path.
Risks: targeting UI is intertwined with aim mechanics in `ranged.cpp`; replace chrome only.

## M8. Accessibility delivery (M, 3 weeks; parallel from M3 onward)

Scope: speech backends (NVDA client + SAPI, AVSpeechSynthesizer, libspeechd), narrator modes
and keys, colour vision mode tables and 3D tints, high-contrast theme, WCAG/CVD tests, Reduce
motion wiring across r3d, sound-cues-as-text subtitles, presets (Vision / Hearing / Motor),
Steam Deck pass (glyph height, on-screen keyboard, default config walk).
Files: `src/ui/speech_{windows,macos,linux}.*`, `src/ui/narrator.cpp`,
`data/raw/colors_{deutan,protan,tritan}.json`, `data/raw/ui/themes/high_contrast.json`,
`src/color_loader.h`, `src/cata_tiles.cpp:3406`, `src/r3d_hud.cpp`, `src/r3d_sky.cpp`
(lightning), `src/sounds.cpp` (subtitle line), `tests/ui_a11y_test.cpp`.
Depends on: M1 (tree), M3 (HUD events).
→ verify: on each OS the narrator speaks the Settings screen (manual, logged); transcript
tests; CVD distance test passes for all tokens in all modes; lightning and hit glow measured
≤ 3 flashes/s; subtitle appears for a scripted sound; preset application changes exactly the
documented options.
Risks: macOS and Windows cannot be run here (unverified platforms per the renderer guide);
needs a tester per OS.

## M9. Reference, onboarding, dialogue, main menu, new game (L, 4-5 weeks)

Scope: Reference (actions, lessons, handbook pages, concepts, F2 integration); Dialogue
chrome on the new layer with attitude header and repeat key; Main menu (5), Pause (5), New
game single screen with five steps, mods picker, world options split; Load.
Files: `src/help.cpp`, `src/onboarding.cpp`, `src/npctalk.cpp` (chrome only),
`src/dialogue_win.*`, `src/main_menu.cpp`, `src/newcharacter.cpp`, `src/worldfactory.cpp`,
`src/ui/screens/{reference,dialogue,main_menu,new_game}*.cpp`.
Depends on: M1, M2, M4.
→ verify: every action in the keybindings JSON has a Reference entry; Guided Play still
starts with the kit; new game creates the same character as the legacy flow for a fixed seed
(golden test); snapshots.

## M9.5. UI-01: one guided start (M, 3 weeks)

Scope (`BACKLOG.md` UI-01, decided 2026-10-21): the Tutorial becomes chapter one of the
guided start and the separate Tutorial and Guided Play entries are removed from New game;
guided is on automatically for the first-ever character on a profile and off afterwards, with
one replay route (a single New game toggle "Guided start", default on for the first character);
the tutorial's `tut_lesson` set merges into the JSON lesson data used by `onboarding_core`;
chapter one runs inside the player's own world (the tutorial complex as a start location or
overmap special, `g->gamemode` active until chapter one ends) because a character cannot move
between worlds; existing guided-progress saves keep loading.
Files: `src/gamemode_tutorial.cpp` (334), `src/gamemode.cpp`, `src/onboarding.cpp`,
`src/onboarding_core.cpp`, `data/json/onboarding/*`, `src/main_menu.cpp` (entries), the New
game screen from M9, the Journal Lessons tab from M6, scenario/start-location JSON, `lang/`
(removed entries), `docs/en/dev/guides/guided_onboarding.md`.
Depends on: M6 (Lessons tab), M9 (New game screen). Parallel with M10.
→ verify: a fresh profile's first character starts in chapter one and continues into the
world with lessons active; a second character starts without it unless the toggle is set; a
saved game from before the merge loads with its progress; the `[onboarding]` tests pass with
the merged lesson set; no `TUTORIAL` special-world entry remains in New game; translations
extract without the removed strings.
Risks: how much `tutorial_game` depends on owning the whole world (to be measured first);
start-location placement of the tutorial complex in a generated world.

## M10. Vehicle (L, 4-6 weeks)

Scope: the Vehicle screen (part grid, card, 8 verbs) replacing `veh_interact`; driving
controls on the wheel and HUD cluster.
Files: `src/vehicle/veh_interact.*`, `src/vehicle/vehicle_display.cpp`,
`src/vehicle/vehicle_preview.cpp`, `src/ui/screens/vehicle*.cpp`.
Depends on: M5 (loot mode), M7.
→ verify: install/repair/remove paths produce identical vehicle state to the legacy screen for
a scripted vehicle (golden test); snapshots.
Risks: `veh_interact` mixes rules (skill checks, part validity) with drawing; extract the
rules first.

## M11. Legacy retirement and polish (M, 3 weeks)

Scope: delete replaced screens and contexts, remove options tagged "removed", keep the
legacy path only for debug, Lua console, computer terminals, minigames and Defense mode (or
cut per the user's decision); theme file for the classic look; translation pass; docs
(`docs/en/dev/guides/ui_overhaul.md`, user docs). **Curses retirement phase 3** (M, 2 weeks,
included): delete `src/cursesport.cpp/.h` and `src/cursesdef.h` once the remaining legacy
screens are ported to the widget layer or dropped, remove the remaining `#if defined(TILES)`
guards (230 in 66 files at the start; most go with their screens in M3-M10), `TERMINAL_X/Y`,
the bitmap-font path if unused, and ncurses mentions in packaging.
→ verify: LOC removed ≥ 30k (audit counts); no `uilist` use outside the legacy list; `rg
"catacurses::|cursesport|defined\(TILES\)" src/` is empty; all tests green; `--check-mods`
passes.

## M12 (optional, gated). AccessKit bridge (M, 3 weeks)

Scope: feed the `ui::node` tree to `accesskit-c`; platform adapters; CI toolchain for the
Rust artefact or prebuilt binaries.
→ verify: NVDA/Orca/VoiceOver can navigate Settings and Inventory natively (manual per OS).

## Parallelisation summary

- After M1 (gate passed): M1.5, M2 and M4 in parallel; M3 after M2 and HUD approval.
- After M3: M5 (two streams), M6 (three streams) and M8 in parallel.
- M7 after M5; M9 anytime after M4; M9.5 after M6 and M9; M10 last of the screens; M11 after all.
- Critical path: M0 → M1 → M2 → M3 → M5 → M7 → M10 → M11 ≈ 26-34 weeks for one developer
  (the cells backend's 15-20 % per-screen cost is gone; M1.5 and phase 3 add 3 weeks off the
  critical path); about 17-21 weeks with two.

## Risks (whole programme)

1. **Two UIs for a long time.** Mitigation: milestone order puts the most-used screens first;
   every new screen fully replaces its legacy screen in the same milestone (no half states,
   the CDDA lesson).
2. **Upstream merge friction from the curses retirement.** Every `#else` branch deleted in a
   file upstream still changes will conflict. Mitigation: delete guards only in files this
   fork already diverges in; keep the rest until the project decides whether upstream merges
   continue (open question 1).
3. **Hidden sidebar consumers.** Mitigation: M3 greps and tests; classic snapshots catch
   regressions.
4. **Mods and Lua** depend on context ids and `lua_sidebar_widgets`. Mitigation: aliases, and
   the badge API lands in M3.
5. **Platforms unverified** (macOS, Windows, Deck). Mitigation: per-OS testers before M8 is
   called done; speech backends fail to no-op.
6. **Scope creep in the widget layer.** Mitigation: the 12-widget list is the list; new widget
   types need a design note.

## Open questions only the user can decide

1. **Upstream merges:** does this fork still merge from upstream Bright Nights? It decides how
   aggressively `#if defined(TILES)` guards are removed (`06_frontend_options.md` §4).
2. **Approve the HUD design** (§4.1 and §3.1 of `03_design.md`) before M3 starts, including the
   clock-on-compass gating by a watch and the single minimap.
3. **Hotbar slot 1 = wielded item, fixed?** Or all eight free, with the wielded item shown
   elsewhere in the vitals strip.
4. **Default keyboard keys:** keep the "Legacy" binding group on by default for veterans
   (longer Reference list) or off (clean 32-key default, veterans opt in)?
5. **Feature cuts:** Defense mode and minigames stay on the legacy path, or are cut at M11?
6. **Lua sidebar widgets** become HUD badges plus F4 text (proposed), or a dedicated
   on-demand "Mod" panel?
7. **AccessKit phase (M12):** fund it (adds a Rust toolchain or prebuilt binaries to CI) or
   stop at self-voicing?
8. **Speech on Windows:** NVDA client + SAPI (proposed) versus Tolk (covers JAWS too but is
   unmaintained, LGPL-3.0)?
9. **Advanced file location and naming:** `config/advanced.json` next to `options.json`, or a
   single `options.json` with the curated/advanced split only in the UI?
10. **World options:** 8 in the New game screen as proposed, or a different eight?
11. **UI scale range:** 75-200 % proposed; Steam Deck may need 110 % default at 1280x800.
12. **Readable font licence check:** Atkinson Hyperlegible (OFL) acceptable to ship?
13. **Theme count:** Dark, High contrast and Classic only, or also a Light theme (adds a
    contrast-test matrix row)?
14. **Frontend gate outcome (end of M1):** if `accesskit-c` fails with two of three screen
    readers, re-cost Godot (option B) or accept self-voicing only? `06_frontend_options.md` §3.
15. **UI-01 replay route:** a New game toggle "Guided start" (proposed) or an option in
    Settings > Game?
