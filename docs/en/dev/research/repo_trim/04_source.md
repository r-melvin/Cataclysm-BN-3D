# Repo trim audit 04: source, tests and build system

Scope: `src/`, `tests/`, `CMakeLists.txt`, `CMakeModules`, CMake options, PCH, platform ports.
Read-only audit on branch `chore/repo-trim-audit`. Measured against `upstream/main` after `git fetch upstream`
(merge-base `e94480e450`; fork is 138 commits ahead, 1 behind). Claims not checked by running something are marked
**(unverified)**.

## 0. Headline findings

1. Divergence is small and almost purely additive. `git diff --shortstat upstream/main...main` gives 203 files,
   +29,113/-487. Of those, 130 are newly added files (+~22.7k lines in src/tests alone), 73 are modified upstream
   files (+1,562/-487), 0 are deleted. Upstream game logic is essentially untouched.
2. The fork is NOT a separate frontend: the 3D view is a toggle inside the SDL3 "tiles" build (`r3d::draw()` is called
   from `sdltiles.cpp` into the map window; text, menus and HUD still go through cursesport + SDL_ttf). It also
   consumes the 2D tileset loader (`cata_tiles.cpp` `tileset_loader`; `cata_tiles.h` exposes `r3d_texture()`,
   `r3d_rect()`). So "retire curses" is cheap, but "retire 2D" is not (see 2.3).
3. Upstream is already SDL3 and curses-only code is already compiled out of the tiles build
   (`ncurses_def.cpp` line 1: `#if !(defined(TILES) || defined(_WIN32))`; `wincurse.cpp`: `#if !defined(TILES) && defined(_WIN32)`).
   Removing curses saves little source (about 1.4k lines) but removes a whole build matrix.
4. Lean-repo wins are NOT in src/. Biggest by bytes: `android/` (45.5 MB tracked, of which `deps.zip` 44.6 MB),
   `gfx/` 30 MB, `data/` 73 MB (not my scope). `src/` is 25 MB on disk (606k lines, 1,102 tracked files) and
   ~98% is upstream game logic or vendored Lua/sol that must stay.

## 1. Divergence map

### 1.1 Fork-only (new) files

| Group | Files | Lines | Notes |
|---|---|---|---|
| `src/r3d*.cpp/.h` (renderer, mesh, entities, HUD, GPU, sky, vox, camera, rules, profiles...) | 59 src files incl. `r3d_*`, 3 `src/shaders/r3d_*.hlsl` | about 15,900 (r3d.cpp 3021, r3d_mesh.cpp 2444, r3d_entities.cpp 2385, r3d_hud.cpp 625, r3d_gpu.cpp 619, ...) | Pure additions |
| `src/onboarding*.cpp/.h`, `onboarding_input.h` | 5 | about 2,000 (onboarding.cpp 1361) | Guided play; hooks in main_menu.cpp, input.cpp |
| `src/status_report.*`, `src/ui_theme.*` | 4 | about 205 | |
| `tests/r3d_*_test.cpp`, `tests/onboarding*_test.cpp`, `tests/data/r3d_geometry_snapshot.txt` | 25 | 5,682 (cpp only) | Fork-only tests |
| `data/raw/r3d/`, `data/json/onboarding/`, `data/mods/TEST_DATA` (1 file) | 14 | about 700+ | Outside scope |
| `tools/r3d_spike/` | 12 | 1,627 | python capture/compare tools |
| `docs/en/dev/{guides,research}`, `BACKLOG.md` | 10+ | about 2,500 | |

Total src additions: 68 files; src+tests additions: 93 files, 22,711 lines (`git diff --diff-filter=A --numstat`).
The `r3d_*` set is self-contained: it includes game headers (`avatar.h`, `game.h`, `creature.h`, ...) one way; nothing in
upstream game logic includes `r3d*` except the frontend files listed below (verified by grep: only `sdltiles.cpp`,
`handle_action.cpp`, `pixel_minimap.cpp`, `game.cpp`/`options.cpp` call into it).

### 1.2 Modified upstream files: top 40 by lines changed (added/removed)

| # | File | +/- | Kind |
|---|---|---|---|
| 1 | src/options.cpp | +199/-65 | frontend (graphics options) |
| 2 | src/sdltiles.cpp | +183/-24 | frontend |
| 3 | src/handle_action.cpp | +153/-25 | frontend input (r3d controls) |
| 4 | src/input.cpp | +119/-9 | frontend |
| 5 | data/raw/keybindings/keybindings.json | +126/-0 | data (not scope) |
| 6 | src/action.cpp | +94/-11 | frontend |
| 7 | src/map/mapbuffer.cpp | +2/-101 | LOGIC: fork deleted dead helpers (warning cleanup) |
| 8 | README.md | +101/-0 | docs |
| 9 | .github/ISSUE_TEMPLATE/bug_report.yml | +5/-86 | meta |
| 10 | src/cata_tiles.h | +84/-1 | frontend |
| 11 | src/pixel_minimap.cpp | +56/-1 | frontend |
| 12 | tests/input_test.cpp | +48/-0 | test |
| 13 | build-scripts/generate-shaders.sh | +43/-5 | build |
| 14 | src/game.cpp | +39/-5 | frontend/hooks |
| 15 | CMakeLists.txt | +34/-0 | build |
| 16 | .github/ISSUE_TEMPLATE/feature_request.yml | +5/-28 | meta |
| 17 | AGENTS.md | +33/-0 | docs |
| 18 | src/main_menu.cpp | +20/-12 | frontend (guided play entry) |
| 19 | src/input.h | +6/-13 | frontend |
| 20 | src/panels.cpp | +18/-0 | frontend |
| 21 | src/map/map.cpp | +6/-10 | LOGIC (map cache bounds fix) |
| 22 | build-scripts/format-cpp.sh | +13/-3 | build |
| 23 | src/ui_manager.cpp | +9/-5 | frontend |
| 24 | src/character.cpp | +8/-6 | LOGIC (warning cleanup) |
| 25 | docs/en/dev/guides/formatting.md | +14/-0 | docs |
| 26 | src/map/submap.h | +13/-0 | LOGIC (header, many includers) |
| 27 | src/action.h | +12/-0 | frontend |
| 28 | lang/extract_json_strings.py | +11/-0 | i18n tooling |
| 29 | src/init.cpp | +8/-2 | hooks |
| 30 | tests/map_test.cpp | +9/-0 | test |
| 31 | src/savegame_json.cpp | +9/-0 | LOGIC (save format) |
| 32 | src/iexamine.cpp | +4/-5 | LOGIC |
| 33 | src/mapgen/mapgen.cpp | +0/-8 | LOGIC (warning cleanup) |
| 34 | src/main.cpp | +6/-1 | hooks |
| 35 | src/ranged.cpp | +5/-2 | LOGIC |
| 36 | src/enchantments/enchanter.cpp | +0/-7 | LOGIC (warning cleanup) |
| 37 | src/sdltiles.h | +5/-1 | frontend |
| 38 | src/output.cpp | +5/-1 | frontend |
| 39 | src/item.cpp | +3/-3 | LOGIC |
| 40 | src/consumption.cpp, shaders/lm_*.hlsl (x3), src/messages.cpp, src/cata_tiles.cpp | 5 each | mixed |

Only the first 4 src files exceed 100 changed lines. 73 modified files in total; the long tail is 1-10 line edits.
Conflict-prone subset: about 12 src files touch pure game logic (mapbuffer, map, submap.h, character, savegame_json,
iexamine, mapgen, enchanter, ranged, item, consumption, messages). These come from warning-cleanup commits
(`ab92747367`, `dc3fe31bc9`, `4349615dc2`) and one regression fix (`map_cache_query_rejects_out_of_bounds_levels`
test in `tests/map_test.cpp`). They are the actual friction for "only pull upstream game logic" and are easy to
upstream-or-drop. Everything else is frontend files the owner would own anyway.

Evidence of merge cost so far: 8 merge commits from upstream in 4 days (2026-10-18..21); fork history begins at
`f81afc927f`. No conflict-resolution data was inspected **(unverified)**. Upstream is 1 commit ahead right now.

## 2. Trim candidates (3D-only, curses retired)

Sizes are tracked lines (`git ls-files | cat | wc -l`) unless stated.

### 2.1 Safe or near-safe removals

| Item | Files | Lines / bytes | Evidence | Risk |
|---|---|---|---|---|
| `src/ncurses_def.cpp`, `.h` | 2 | 554 | Body is `#if !(TILES || _WIN32)` i.e. empty in tiles build; used only by CURSES target. `hsv_color.cpp:12,101` calls `ncurses::color_to_RGB`, which is also defined in cursesport.cpp for non-ncurses builds (check before delete **(unverified)**) | Low |
| `src/wincurse.cpp` | 1 | 796 | `#if !defined(TILES) && defined(_WIN32)`: never compiled when TILES | Low |
| CURSES CMake target + option | CMakeLists.txt L33,209-212,646-662,893-935; src/CMakeLists.txt L209-249; tests/CMakeLists.txt L37-52 (`cata_test`) | about 110 lines | `if (TILES) set(CURSES OFF)`; linux-full cache has `CURSES:BOOL=OFF` | Low |
| Curses presets: `linux-curses`, `dist-curses`, `ci-curses`, `osx-curses-*` | CMakePresets.json | about 8 presets | Presets reference option only | Low |
| `#if !defined(TILES)` branches | editmap.cpp:510, game_ui.cpp:3, main.cpp:779/819, ui_manager.cpp:145-196 (x5), output.cpp:2199, action.cpp:1215, animation.cpp:365/689, game.cpp:601, input.cpp:84, options.cpp:975/2515 | about 20 sites; roughly 300 lines **(estimate)** | grep `!defined( *TILES` | Low-Med (these are upstream files; deleting them creates upstream merge conflicts, so prefer leaving them) |
| `CMakeModules/Find/FindSDL2*.cmake` (4) | 4 | 577 | No `find_package(SDL2` anywhere; only config-mode `find_package(SDL3... CONFIG)` (CMakeLists 405,582,615,677) | Low |
| `CMakeModules/Find/FindSDL3*.cmake` (4) | 4 | 334 | All SDL3 lookups are CONFIG mode, so Find modules look unused. **(unverified: confirm by deleting in a scratch build)** | Low-Med |
| `build-scripts/SDL2-devel-2.26.2-mingw.tar.gz.sha256` | 1 | 1 | Only reference is the file itself | Low |
| `android/` | 78 files | 45.5 MB (deps.zip 44.6 MB), 16 java/kt files 6.5k lines | No reference from CMakeLists/presets; `__ANDROID__` appears in 37 src files / 128 lines (sdltiles 26, options 18, debug 10, main 6, input.h 6...) but those are ifdef branches in upstream files | Low to delete dir; Med to strip the ifdefs (upstream conflicts) |
| `msvc-full-features/`, `cmake-build.bat`, `cmake-build.ps1`, `setup.ps1`, `CMakeSettings.json`, `launch.vs.json` | about 18 | about 2,800 lines | Visual Studio project route, referenced only by docs/vs_* guides and `.gitignore` | Low |
| `build-scripts/{MSVC,VsDevCmd,x64-windows-static,windows-tiles-sounds-x64-msvc}.cmake`, `generate-shaders.ps1`, `WindowsLavapipe.cmake` | 6 | about 740 | WindowsLavapipe.cmake is included from CMakeLists L568 under `if (WIN32 AND CATA_SDL)` | Low if Windows is dropped; the owner must say |
| `update-pch.sh` + `pch/main-pch.hpp` | 2 | 77 | PCH is used only by tests (`tests/CMakeLists.txt:28,49`, and skipped when `CMAKE_EXPORT_COMPILE_COMMANDS`, which the linux-full preset sets). Effectively dead in the owner's workflow | Low |
| `doxygen_doc/` | 2 | 2,112 | Docs generation only | Low |
| `CATA_GPU_VERIFY` option | CMakeLists L88,282,433 | 3 sites | Dev-only | Low (keep if r3d work uses it) |
| `DYNAMIC_ATLAS` option | 3 files, 17 lines | | Cache has it ON; always-on would simplify | Low |

### 2.2 Candidates needing an owner decision

| Item | Files | Lines | Evidence | Risk |
|---|---|---|---|---|
| Defense special game | `src/gamemode_defense.cpp/.h` (1,565 lines), plus hooks: `gamemode.cpp:2,16,33`, `main_menu.cpp:956` (menu entry sel2==6), `worldfactory.cpp:129,269,1501`, `overmap/overmap.cpp:3123`, `enums.h:129`; data: `defense_mode.json`, `defense_bot.json` | about 1.6k + 5 hooks | Only call sites are those. Hooks are in upstream-owned files (main_menu is fork-modified), so deletion creates 4-5 small recurring conflicts | Med: payoff 1.6k lines vs merge friction; recommend leaving it |
| Tutorial special game | `gamemode_tutorial` family **(unverified)** | | `special_game_type::TUTORIAL` still used; guided onboarding is fork-added and may supersede | Med |
| `src/sdlsound.cpp/.h` (SOUND) | 2 | 954 | `SOUND:BOOL=ON` in cache; used via `SDL_SOUND` (10 lines in 5 files) | Do not drop unless audio is dropped; owner call |
| `src/ime.cpp` + `src/pinyin.cpp` + `src/third-party/pinyin/` | 4 | about 270 + 453 | Text-input IME for CJK; game feature | Keep (upstream feature) |
| `tools/clang-tidy-plugin/` | 66 | 6,171 | CMake `CATA_CLANG_TIDY_PLUGIN`, ON in `linux-full` preset; CI uses | Low risk to move; owner uses it |
| `tools/format/` (JSON formatter) | 4 | 522 | `JSON_FORMAT` ON in 5 presets; lint scripts need it | Keep, or move with data tooling |
| `tools/{iwyu,json_tools,gfx_tools,graphs,...}`, `utilities/`, `scripts/` (43 TS/py) | many | about 12k | Not built from src; upstream data/i18n/CI helpers | Low; outside source scope, see other reports |
| `BACKTRACE`/`LIBBACKTRACE`/`USE_TRACY` | CMake L68-71,692-760; src: 4 files/23 lines BACKTRACE, 2 lines TRACY | | Dev ergonomics; cache has LIBBACKTRACE ON. libbacktrace is downloaded from GitHub master at configure (supply-chain note) | Keep |
| `USE_UNITY_BUILD`, `USE_XDG_DIR`/`USE_HOME_DIR`/`USE_PREFIX_DATA_DIR`, `LANGUAGES` | CMake | few lines | Packaging options | Keep or drop with release story |

### 2.3 NOT removable now (and why)

| Item | Why |
|---|---|
| `src/cursesport.cpp/.h`, `cursesdef.h` (561+90+139 lines) | `#if defined(TILES) || defined(_WIN32)`: this is the cell-grid window layer that every menu in the tiles build draws through (`catacurses::window`). All of `ui_manager`, `output.cpp`, panels depend on it. Replacing it is a UI rewrite, not a trim. |
| `src/sdltiles.cpp` (4,463), `sdl_font`, `sdl_utils`, `sdl_wrappers`, `font_loader` etc. | r3d is drawn from `sdltiles.cpp` (grep: `r3d::draw` at L1684, input at L3508-3544, pick at L4306) |
| `src/cata_tiles.cpp` (7,552) and `.h` (1,305) | Contains `tileset_loader` (about lines 1125-3000, about 2k lines) which r3d uses for sprite textures, plus the 2D draw path (about lines 3040-7200, about 4k lines: `draw`, `draw_terrain`, `draw_vpart`, `draw_critter_at`, explosions, weather, SCT, zones). The explosion/weather/SCT/zone overlays are also fork-integrated (`2f1d3e9f4a feat(ui): integrate native HUD and weather overlays`). The docs (`docs/en/dev/guides/renderer_3d.md:40`) say other backends keep the 2D view; README says 3D is "optional". Dropping the 2D map path would remove at most about 4k lines and needs r3d to cover everything 2D does (animations, targeting cone, overmap/minimap, zones) **(unverified: not tested)**. Not recommended for this round. |
| `src/pixel_minimap.cpp` (708) | Fork-modified; first-person minimap uses `r3d::minimap_turn` |
| `src/lua/` (62 files, 23k lines, vendored C) and `src/sol/` (5 files, 30k lines) | Lua is mandatory: `target_compile_definitions(... LUA)` is unconditional in src/CMakeLists.txt. Excluded from the main glob via REGEX filter. |
| `tests/catch/` (657 KB single header) | Test framework |
| `src/compute/*` (7 files, 6.4k lines), `src/shaders` | GPU lightmap/transparency (upstream) and r3d shaders |
| `src/platform/sdl_video.*` (36 lines) | Headless SDL offscreen hint, used by main.cpp:63 |

Totals (high-confidence removable, excluding "needs decision"): about 5 MB-excluding-Android source; by count:
- C++ source: ncurses_def (554) + wincurse (796) = 1,350 lines / 3 files.
- CMake: about 110 lines curses + 577 SDL2 Find + possibly 334 SDL3 Find.
- Non-source: android 78 files / 45.5 MB; MSVC/VS tooling about 25 files / about 3.5k lines.
So removable from `src/` is about 1,350 lines (0.2% of 606k). Everything bigger touches upstream files.

## 3. Tests

- 283 tracked files under `tests/` (212 `.cpp` incl. headers, 58,776 lines in `tests/*.cpp`).
- Fork-only: 25 files (24 `r3d_*`/`onboarding*` `.cpp` + 1 snapshot), 5,682 lines (about 9.7% of test LOC).
  Biggest: `r3d_test.cpp` 1,042, `r3d_vehicle_rules_test.cpp` 939, `r3d_presentation_profiles_test.cpp` 578,
  `r3d_first_person_test.cpp` 517, `onboarding_test.cpp` 345. They run under tag `[r3d]`
  (`CATA_TEST_COMPUTE_ACCELERATION=cpu ... cata_test-tiles "[r3d]"`).
- Fork edits to upstream tests: `input_test.cpp` +48, `map_test.cpp` +9. Everything else (about 188 files) is upstream
  game logic (crafting, map, iuse, vehicle, npc_talk, rot, catalua 3,356 ...).
- 16 test files reference `TILES` (`tile_helpers.*`, `overmap_test`, `sound_mechanics_test`, `ranged_aiming_test` and the r3d tests); the others are renderer-independent.
- The only test binary in the current config is `cata_test-tiles` (965 MB in `out/build/linux-full/tests`, debug info). `cata_test` (curses) is only added when CURSES=ON, so retiring curses removes one 200-file target.
- Runtime: no CTest log present. `build-scripts/linux-test-profile.md` (fork-maintained) reports
  full sharded run `run-linux-test-shards.ts --jobs 4 --non-slow-shards 8`: **6:52 wall** before the `vehicle_drag`
  fixture fix; single largest shard 165 s; CI baseline was 707 s for the old non-slow shard. I did not run the suite.
  The fork-only `[r3d]` subset is a few seconds to tens of seconds **(unverified)**.
- Could move: the 24 r3d/onboarding tests to `tests/r3d/` or a separate `cata_test-r3d` target (they only need the
  `cataclysm-bn-tiles-common` object lib, same as now). tests/CMakeLists.txt globs `tests/*.cpp` non-recursively, so
  subdirectory files need an explicit glob. Upstream test additions land in `tests/*.cpp`, which stays untouched.
  Cost: about 15 lines of CMake.

## 4. Structure options for pulling upstream game logic

Numbers behind the options: 130 fork-only files, 73 modified (of which about 50 in src/, about 12 touching pure game logic),
0 deletions, 138 commits, +1,562/-487 in modified files. Upstream is a moving target (one commit landed between my two checks).

| Approach | How | Pros | Cons | Effort |
|---|---|---|---|---|
| A. Keep long-lived fork + periodic merge (current) | `git merge upstream/main` | Already working (8 merges in 4 days); zero tooling; additive footprint means conflicts are confined to about 12-20 files | Every file you touch in an upstream path is a standing conflict; warning-cleanup edits in logic files (mapbuffer, character, enchanter, mapgen) are pure avoidable cost; history grows with merge commits | Low now; stays low if rule: "no edits to non-frontend upstream files" |
| A+. Same, plus tightening | Revert/upstream the 12 logic-file edits; move r3d/onboarding into `src/r3d/` and tests into `tests/r3d/`; keep hooks in `sdltiles.cpp`, `handle_action.cpp`, `options.cpp`, `input.cpp`, `main_menu.cpp`, `game.cpp` (about 5-6 files, about 600 lines) | Merge surface drops to about 6 frontend files; subdirectory makes `git diff upstream/main` readable; trimming candidates in 2.1 are all fork-owned or build-only, so no extra conflicts | Moving r3d files changes include paths (`#include "r3d.h"` in `sdltiles.cpp`, `handle_action.cpp`, `pixel_minimap.cpp`, tests) and CMake glob excludes `src/lua|sol|third-party` only, so `src/r3d/` would need no CMake change **(unverified)** | Low: about half a day plus a rebuild |
| B. Patch queue (quilt/StGit/topgit; rebase onto each upstream tag) | Keep fork as small series on top of upstream/main | Cleanest diff vs upstream; rebase conflicts are explicit | 138 commits and the 3 docs-heavy commits to curate; losing merge history; two people editing is painful; heavy hand tooling | Medium-High; ongoing |
| C. Git submodule/subtree of upstream core + frontend repo | Upstream in `core/`, fork has CMake that builds `core/src/*.cpp` + `frontend/` | Perfect upstream isolation; trivially "pull logic" | Not possible without patching: the 3D frontend edits 6 upstream files (sdltiles, handle_action, options, input, main_menu, game) and the CMake globs `src/*.cpp`; the upstream build expects its own paths (`data/`, `lang/`, `gfx/`, tests link against its object lib); you'd need to carry those 6-file patches anyway, i.e. a patch queue in disguise. Subtree squashes history. | High |
| D. Own the frontend as a plugin-style static lib | Expose a narrow interface (`r3d::init/draw/...` already a namespace) and have 6 hook files call it | Already the case: r3d public API is the namespace in `r3d.h`; hooks are 1-5 lines each except options (+199) and handle_action (+153) | Needs hook-count discipline | Already done |

Recommendation: **A+** (long-lived fork, merge, but enforce a "fork owns only frontend paths" rule). Evidence: 0 deleted
files, additive-only r3d, only 4 upstream src files above 100 changed lines, 12 logic files with trivial edits.
A patch queue or submodule costs far more than it saves at this divergence level. Revisit B if the number of modified
upstream files passes about 100 or if upstream starts rewriting `sdltiles.cpp`/`cata_tiles.cpp` heavily
(**the one real risk**: fork-edited `sdltiles.cpp` and `cata_tiles.h` are where upstream also changes graphics code).
Trim deletions of upstream-owned code (Defense, `!TILES` branches, Android ifdefs) buy about 2k lines but each adds
standing conflicts; prefer leaving ifdef branches in upstream files and delete only whole files/dirs the fork owns
or that upstream never touches (android/, VS tooling, SDL2 Find modules, curses CMake target).

## 5. Owner questions

1. Is Windows (WIN32 CMake, VS tooling, Lavapipe, `_WIN32` in 53 src files) dropped, or only curses?
2. Is Android dropped (45.5 MB)? Should the `__ANDROID__` ifdefs in upstream files stay (conflict-avoidance)?
3. Keep sound (SDL3_mixer, `SOUND=ON` in your cache)?
4. Keep Defense and Tutorial game modes? (Defense is 1.6k lines + 5 hooks; I advise keeping.)
5. Is the 2D map path (cata_tiles draw functions, about 4k lines) allowed to go, given the README says 3D is optional? If yes, expect
   rewriting overlays (animations, targeting, weather, zones) in r3d first.
6. Are fork edits to logic files (mapbuffer, map, character, enchanter, mapgen, iexamine) wanted, or should they be reverted/sent upstream?
7. Do you want `tests/r3d/` as a subdirectory and a separate `[r3d]` target?
8. Are the clang-tidy plugin (6.2k lines) and JSON formatter staying in-tree?

## 6. Caveats

- LOC for "needs decision" items is not exact; totals exclude data, lang, docs, gfx.
- I did not build or modify anything. The suite runtime comes from `build-scripts/linux-test-profile.md`, not a run.
- `FindSDL3*.cmake` unused and `hsv_color.cpp` independence from ncurses_def are inferences from grep.
