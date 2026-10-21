# 06. Frontend options: stay on SDL3 or move to an engine

Question from the user (2026-10-21, `BACKLOG.md` UI-02): now that curses is being retired, would
moving the 3D frontend to a different engine materially help accessibility? This compares the
options on verified facts and costs them against the plan in `05_plan.md`.

Evidence tags: **V** = verified this session from an official page, source file, release
list or class reference (URL given; several checked directly through the GitHub API);
**NV** = not verified (agents' web-search budget ran out; or the page was unreachable).
Nothing tagged NV is used to decide anything. Raw findings: `research/q_*.md`.

## 0. What has to be true for a move to pay off

A frontend move pays off only if it removes work that option A must do and does not add more
than it removes. The accessibility work in `03_design.md` §7 has five parts:

1. a UI tree with names, roles, values and states (needed by any screen reader path);
2. a narrator that decides what to say and when (reading order, HUD events, cursor-mode tile
   descriptions, log lines) with speech output;
3. an OS accessibility bridge (UIA / NSAccessibility / AT-SPI) so NVDA, VoiceOver and Orca can
   navigate the tree;
4. remapping, gamepad, text scale, colour modes, reduced motion;
5. per-screen design of what the tree contains.

An engine can supply 1 (as its widget set), 3 (as its screen-reader integration) and parts of
4. No engine supplies 2 or 5: every shipped narrated game the research found did that
per-screen itself (Sea of Thieves, Mortal Kombat 1: `research/q_case_studies.md`). So the
ceiling on what a move can save is roughly milestones M1 (widget layer, ~5 weeks), M12
(AccessKit bridge, ~3 weeks) and half of M2 (gamepad, ~1.5 weeks): **about 10 weeks**. Anything
a move costs above that is a net loss for accessibility.

{{svg:frontend_decision}}

## 1. The options

### A. Stay: SDL3 + SDL_GPU, own widget layer, self-voicing, AccessKit C

- **Accessibility reality.** Nothing exists today (verified: no TTS, narration or a11y code in
  `src/`). The plan builds the tree (M1), the narrator with NVDA-client/SAPI, AVSpeech and
  speech-dispatcher backends (M8), then the OS bridge through `accesskit-c` (M12).
  `accesskit-c` 0.23.1 was released 2026-09-26 and its repository ships `examples/sdl` and
  `examples/windows` (**V**, GitHub API listing); Godot 4.7 vendors the same header
  (`thirdparty/accesskit`, version 0.23.1, MIT: **V**, `thirdparty/README.md`). AccessKit's
  own docs call the adapters "best-effort" and the project "sparsely documented" (**V**,
  `ARCHITECTURE.md`); the Linux adapter needs the AT-SPI bus up (**V**). Remapping, gamepad,
  text scale and colour modes are all our own work (M2, M8).
- **Core embedding.** None: the core stays in-process, screens keep their blocking input loops
  (82 `handle_input` call sites, `while( !g->do_turn() )` main loop), Lua and saves untouched.
- **r3d reuse.** 100 %: the 15.2 k LOC in `src/r3d_*`, the 6.4 k LOC SDL_GPU compute lightmap in
  `src/compute/` and the 25 SPIR-V shaders stay as they are.
- **Platforms.** Linux (tested), macOS and Windows (unverified by this fork so far, per the
  renderer guide), Steam Deck via SDL gamepad labels and the Deck text-size rules.
- **Licensing / CI.** SDL (zlib), SDL_ttf (zlib), AccessKit (Apache-2.0 or MIT; GitHub's
  licence field says BSD-3-Clause because of Chromium-derived parts, **V**), libspeechd
  (LGPL-2.1+, dlopen), NVDA client (LGPL-2.1, dlopen). CI unchanged except a Rust toolchain or
  prebuilt binaries for M12.
- **Effort.** The plan: 28-36 weeks critical path for one developer (`05_plan.md`). Confidence:
  medium; the widget-layer size (8-12 k LOC) is an estimate.
- **Reversibility.** High: the view-model boundary (`04_architecture.md` §2.2) is exactly the
  seam a later frontend swap needs.

### B. Godot 4.x as the frontend, game core as a GDExtension

- **Accessibility reality (V unless noted).** Screen-reader support arrived in 4.5 (tag
  2025-09-15) "thanks to AccessKit" for Control nodes, "still in its experimental phase"
  (release page). Current stable is 4.7.2 (2026-08-18). In 4.7 the API moved from
  `DisplayServer` to a dedicated `AccessibilityServer` singleton (changelog GH-116839),
  "implemented on Android, Linux, macOS, and Windows" (class reference); the
  `FEATURE_ACCESSIBILITY_SCREEN_READER` flag lists Linux (X11/Wayland), macOS, Windows. The
  4.7 GDExtension API exposes `AccessibilityServer` (75 methods incl. `create_element`,
  `update_set_role`, `update_set_name`), `Control.accessibility_name/description/live/...`,
  and the virtuals `_accessibility_get_contextual_info`, `_get_accessibility_container_name`,
  `Node._get_focused_accessibility_element`, `Node.queue_accessibility_update` (checked in
  `godot-cpp/gdextension/extension_api-4-7.json`). So custom nodes written in C++ can be made
  accessible. Open issues include screen-reader-related crashes and wrong positions
  (#121388, #120999, #111028, #123623, all open on 2026-10-21). Runtime detection is automatic
  when a screen reader is running. Remapping: `InputMap` runtime methods exist; gamepads use
  the SDL controller database; no glyph API found (NV). Text scale: `Window.content_scale_*`;
  OS DPI is read only on macOS, Android and Wayland, "On Linux (X11) and Windows, you will need
  to provide a manual scaling option" (docs). Colour-blind: proposal #473 open, nothing built
  in. Narration (what to say, when) is still ours; Godot provides `accessibility_live` regions
  for announcements.
- **Core embedding.** godot-cpp 10.0.0 (2026-09-15, MIT; targets 4.3+ via `api_version`;
  extensions built for an earlier minor run on later ones, not the reverse: **V**, README).
  The scene tree "is not thread-safe"; cross-thread work goes through `call_deferred`
  (**V**, docs). Consequence: the game core, whose screens are blocking input loops, must run
  on a worker thread and talk to the scene tree through a message channel, and every screen
  must be rewritten as Godot Controls fed by view models, with input routed back. That is the
  thin-client split from `04_architecture.md` §2.2 made mandatory, plus Godot's threading
  rules. "libgodot" (embedding Godot into our own executable instead) is proposal #6267, still
  open since 2023 (**V**). Public case studies of a 300 kLOC C++ core inside GDExtension: none
  found (NV); the only data point is Caves of Qud "booting under Godot" after ~16 hours, a
  test and not a migration (**V**, fullcleared.com paraphrase).
- **r3d reuse.** The pure-C++ parts transfer: meshing, shapes, voxel conversion, wall/door/
  scene/vehicle rules and profiles (~4.9 k LOC), camera and math (~1.2 k), knowledge/memory
  rules. Rewritten: the SDL_GPU device layer, pipelines and atlas (~4.5 k), sky/decals/entity
  drawing (~3.0 k), HUD drawing (~1.2 k), the capture tooling, and the compute lightmap
  (6.4 k LOC + 16 compute shaders in HLSL/SPIR-V; Godot compute runs on `RenderingDevice`
  only on the Forward+/Mobile renderers, shaders would be re-authored in Godot's GLSL: NV for
  details). Roughly 10 k LOC of renderer and compute to rewrite and re-validate against the
  geometry snapshot and the comparison galleries.
- **Platforms.** Export templates for Linux/macOS/Windows and Steam Deck: NV this session
  (the agent could not fetch the export docs). Widely used in practice, but not verified here.
- **Licensing / CI.** Engine MIT (**V**). Headless editor and export templates are free
  downloads; GitHub-hosted runners suffice. Adds: a Godot version pin, a GDExtension build
  (SCons documented; a `CMakeLists.txt` exists in godot-cpp but is undocumented, **V**),
  export packaging, and a second test harness (Godot's) beside Catch2.
- **Effort.** Embedding and threading bridge 4-6 weeks; renderer and compute port 12-20
  weeks; screens in Godot Controls 20-30 weeks (the widget layer is free, each screen is not);
  input 2 weeks; a11y narration 3 weeks; geometry/visual re-validation 3-4 weeks. **About
  45-65 weeks** for one developer, versus 28-36 for A. Confidence: low-medium (no comparable
  case study; the threading bridge is the unknown).
- **Reversibility.** Low once the renderer is ported. The decision is effectively a rewrite of
  the presentation half of the fork.
- **Net for accessibility.** Saves the AccessKit bridge (M12) and the widget layer (M1),
  about 8-10 weeks, and buys a maintained accessibility tree with active bug fixing (the 4.7
  changelog lists a dozen accessibility fixes). Costs 20-30 weeks of renderer and embedding
  work that has nothing to do with accessibility. **Not justified on accessibility grounds.**

{{svg:embedding}}

### C. Unity 6

- **Accessibility (V).** `AssistiveSupport` / `AccessibilityHierarchy`: "Screen readers cannot
  detect GameObject components or UI elements directly", so the hierarchy is built by the
  developer (the same tree we build in A). The 6.6 guide names TalkBack, VoiceOver (iOS and
  macOS) and Narrator (Windows) as test readers; an older 6000.7 reference names Android
  only; the conflict is unresolved (NV which applies to which version). Linux desktop screen
  readers: not mentioned anywhere found. UAP (third-party, Apache-2.0) is tested only to
  Unity 2020.1 and re-implements speech rather than exposing UI to readers (**V**).
- **Embedding.** C# host with the C++ core as a native plugin via P/Invoke; Unity API is
  main-thread only (**V**); two debuggers; IL2CPP build times. No case study found.
- **Licensing / CI.** Personal free under $200 k revenue/funding, Runtime Fee cancelled
  2024-09-12 (**V**); closed-source editor; CI needs licence activation secrets (GameCI,
  **V**); Linux editor Ubuntu-only (**V**).
- **Verdict.** Rejected: the accessibility tree is still ours to build, Linux screen readers
  are unsupported, and the stack is closed-source C# around a CC-BY-SA C++ game.

### D. Unreal Engine 5

- **Accessibility (V, 5.8 docs).** Slate Screen Reader is "Experimental", third-party readers
  on Windows and VoiceOver on iOS only; Text To Speech "English only"; `EColorVisionDeficiency`
  enum exists (shipping status NV); CommonUI handles controller icons and cardinal navigation.
- **Embedding.** Engine is C++20 and avoids STL outside interop (**V**, coding standard);
  exceptions and RTTI defaults NV; a C++23 core with exceptions, STL and sol2/Lua would be
  interop code wrapped in a module. Engine source cannot be redistributed; royalty and EULA
  terms NV this session (pages 403).
- **Verdict.** Rejected: no Linux or macOS screen-reader support, heavy toolchain, licence
  and distribution constraints for an open-source game.

### E. Rust: Bevy, egui, Slint

- **Bevy 0.20.0 (2026-10-08, Apache-2.0 or MIT, V).** `bevy_a11y` exposes Button, Image and
  Label roles only; the Linux AT-SPI path "only works with experimental screen readers and
  forks" (**V**, Cargo.toml); open design issue #25213; no colour-blind mode (#2724). Pre-1.0
  with a breaking release every 3.5-5 months (**V**, release dates).
- **egui 0.37.0 + AccessKit (V).** Works in eframe; immediate mode; open issue #8671.
- **Slint (V).** AccessKit feature exists; royalty-free licence requires an About widget and a
  public badge, else GPL-3.0.
- **C++ interop (V).** `cxx`: an uncaught C++ exception in a non-`Result` function terminates;
  `autocxx` is archived. The core throws.
- **Verdict.** Rejected: a11y coverage below what A builds, plus a second language and FFI
  boundary around a throwing C++ core.

### F. Others

- **O3DE** (Apache-2.0/MIT, V): no screen-reader code found (NV, search was rate-limited).
  **Flax** (custom EULA with royalty, V): no accessibility statements. **Stride** (MIT, C#):
  none found. All rejected.
- **Web (Emscripten/WASM + DOM/ARIA).** The only option where the platform supplies the
  accessibility tree (DOM + ARIA, every browser screen reader). But CDDA's own
  `build-emscripten.sh` is self-declared unsupported and its release entry commented out
  (**V**, issue #76956 open); BN has no WASM build; SDL_GPU on the web is NV; threads need
  COOP/COEP; desktop distribution needs an Electron/Tauri wrapper. Cost would exceed B.
  Rejected for now; the one case where it would return is a browser-first distribution goal.
- **Thin-client split (core process + frontend process).** Not a frontend by itself; it is the
  architecture that makes any frontend swap cheap. Option A's view-model layer is the
  in-process version of it. Recommended as a design rule, not as a milestone.

## 2. Comparison

| | A. SDL3 + own layer | B. Godot 4.7 | C. Unity 6 | D. Unreal 5 | E. Bevy | F. Web |
|---|---|---|---|---|---|---|
| Screen-reader bridge supplied | No (build via accesskit-c, M12) | Yes: Linux/macOS/Windows/Android, experimental, active fixes (V) | Partly; Linux not mentioned (V) | Windows + iOS only, experimental (V) | 3 roles; Linux experimental (V) | Browser DOM (V in principle) |
| Narration (what/when) supplied | No | No (live regions only) | No | TTS plugin, English only | No | No |
| Remapping / gamepad | Build (M2) | InputMap + SDL db (V), no glyph API (NV) | Input System rebinding (V) | Enhanced Input / CommonUI (V) | leafwing / gilrs (NV) | Build |
| Text scale | Build (M1) | `content_scale_*`, manual on X11/Windows (V) | Panel settings DPI (V) | DPI curve (NV) | Build | CSS |
| Colour modes | Build (M8) | None built in (V, proposal open) | NV | Enum exists (V), shipping NV | None (V) | Build |
| Core embedding | None | Worker thread + channel, all screens rewritten | Native plugin, C# host | Interop module, C++20 | cxx FFI, no exceptions | Emscripten |
| r3d + compute reuse | 100 % | ~40 % (rules/math); ~10 k LOC rewritten | ~40 % | ~40 % | ~40 % | depends on SDL_GPU web (NV) |
| Platforms Linux/macOS/Win/Deck | Yes | Yes (templates NV this session) | Yes (Linux editor Ubuntu) | Linux weak for a11y | Yes | Browser |
| Licence | zlib/MIT/Apache | MIT (V) | Proprietary, free tier (V) | Proprietary, royalty (NV) | MIT/Apache (V) | n/a |
| CI burden (no paid CI) | Unchanged (+Rust for M12) | +Godot pin, templates, 2nd harness | +licence secrets, IL2CPP | +engine build, disk | +cargo | +Emscripten |
| Effort (1 dev) | 28-36 wk | 45-65 wk | >60 wk | >60 wk | >60 wk | >70 wk |
| Confidence | medium | low-medium | low | low | low | low |
| Reversible | high | low | low | low | low | low |

## 3. Recommendation

**Stay on SDL3 (option A).** The accessibility gain available from any engine is capped at
about ten weeks of the plan, and the cheapest engine (Godot) costs twenty to thirty weeks of
non-accessibility work and a low-confidence embedding of a blocking, exception-throwing C++
core into a scene tree that is not thread-safe. Godot's accessibility is real and improving
(4.5 → 4.7 changes verified), but it exposes the same AccessKit that option A reaches through
`accesskit-c`, whose repository already carries an SDL example.

Conditions under which the recommendation flips to B:

1. The M0/M1 spike cannot get `accesskit-c` working with NVDA, VoiceOver and Orca from the
   SDL window on at least two of the three desktops within two weeks, **and**
2. the renderer is going to be replaced anyway (for instance if `r3d_gpu` is abandoned for a
   different lighting or asset strategy), **and**
3. a second developer is committed for at least six months.

Also flip if the project's goals change to mobile or console targets, where Godot's export
pipeline is worth more than the renderer port costs. None of these holds today.

Decision gate: end of M1 (`05_plan.md`), with the spike results written into this file.

## 4. Curses retirement: scope and cost (UI-02)

Decided by the user on 2026-10-21. What exists (verified by grep/wc this session):

| Item | Size | Action |
|---|---|---|
| `src/ncurses_def.cpp/.h` (terminal backend) | 554 LOC | delete in phase 1 |
| `src/wincurse.cpp` (Windows console backend) | 796 LOC | delete in phase 1 |
| `CURSES` CMake option, `find_package(Curses)`, macOS ncursesw bundling, `tests/CMakeLists.txt` branch | `CMakeLists.txt:33, 207-212, 646-660, 893-934`; `src/CMakeLists.txt:208-230` | delete in phase 1 |
| Presets `linux-curses`, `dist-curses`, `osx-curses-x64-dist`, `osx-curses-arm-dist` and the `ci-curses` matrix entry | `CMakePresets.json`, `.github/workflows/{build,matrix,release}.yml` (4 jobs) | delete in phase 1 |
| `src/cursesport.cpp/.h`, `src/cursesdef.h` (the in-house cell model the tiles build draws legacy screens with) | 790 LOC | **keep** until the last legacy screen is replaced (M11), then delete |
| `#if defined(TILES)` guards | 230 guards in 66 files (`newcharacter.cpp` 33, `game.cpp` 32, `animation.cpp` 28, `options.cpp` 13, ...) | the `#else` branches become dead in phase 1; remove per file as each screen migrates (phase 2), remaining ones in phase 3 |
| `catacurses::` call sites | 932 references in 82 files | go away screen by screen with M3-M10; none deleted up front |
| Tests referring to curses/TILES | 39 files mention them; the `cata_test` (curses) target | drop the curses test target in phase 1; `cata_test-tiles` stays the only test binary |
| Docs | building guides, `ui_overhaul.md` ("F4 is also registered in the curses interface"), README | phase 1 |
| Options | `USE_TILES`, `TERMINAL_X/Y`, font-grid options | fold into the Display tab / advanced file in M4 |

{{svg:curses_retirement}}

Phases and cost:

- **Phase 1 (S, 1 week), right after M0:** remove the terminal and Windows-console backends,
  the build option, presets, CI jobs, the curses test target and docs. The tiles build keeps
  `cursesport` as the cell canvas for legacy screens. → verify: `cmake --preset linux-full`
  and the MSVC preset configure without `CURSES`; the release workflow matrix has no curses
  rows; `rg "CURSES" CMakeLists.txt` is empty.
- **Phase 2 (ongoing, inside M3-M10):** each migrated screen deletes its `catacurses` drawing
  and its `#else` branches. No separate cost beyond the screen itself.
- **Phase 3 (inside M11, M, 2 weeks):** delete `cursesport`, `cursesdef.h`, the remaining
  guards, `TERMINAL_X/Y`, the bitmap-font path if unused, and `ncurses` mentions in packaging.

Risks: **upstream merge friction.** Upstream Bright Nights keeps curses, so every file where
an `#else` branch is deleted will conflict on future upstream merges. Mitigation: delete
guards only in files this fork already diverges in (the screens being replaced), keep guards
untouched in files the fork has not changed, and record in `BACKLOG.md` whether upstream
merges are still planned (open question). A second risk is Android: the Android build uses
the tiles path, so it is unaffected by phase 1, but the `ANDROID_*` options and virtual
joystick code are tied to `sdltiles.cpp` and must survive M2's gamepad rewrite.

Accessibility consequence (from UI-02): terminal screen readers were never a supported path
in this fork (the only related code is caret placement), so retiring curses removes no working
accessibility; the narrator and AccessKit path is the plan either way.

## 5. What this changes in the plan

- `04_architecture.md`: the cells backend is removed; the widget tree has one renderer (SDL).
  The frontend decision gate is added at the end of M1.
- `05_plan.md`: new milestone **M1.5 Curses retirement phase 1**, the gate in M1's verify
  list, UI-01 as its own milestone after M6/M9, and phase-3 deletion inside M11.
- Effort: the plan loses the 15-20 % per-screen cells cost (about 4-6 weeks over the
  programme) and gains 1 week (phase 1) + 2 weeks (phase 3). Net: about 2-3 weeks shorter.

## 6. Verification notes

Verified directly through the GitHub API or raw file fetches in this session: Godot release
tags and dates; `thirdparty/README.md` (accesskit 0.23.1, MIT); `doc/classes/
AccessibilityServer.xml`, `Control.xml`, `DisplayServer.xml`; the 4.7.2 `CHANGELOG.md` entries
GH-116839, GH-116960, GH-117740 and the accessibility fix list; godot-cpp release 10.0.0 and
licence; `godot-cpp/gdextension/extension_api-4-7.json` contents; godot-proposals #6267 state;
the 4.5 release page sentence on AccessKit; accesskit-c examples and release 0.23.1; Bevy
0.20.0 release date and crate list; the local `src/` and build-file counts. Everything else
in this document is tagged with the agent's own verification status in `research/q_*.md`.

Not verified (and marked NV above): Godot export templates and Steam Deck specifics; Godot
compute-shader port details; any large-C++ GDExtension case study; Unreal royalty and EULA
text; Unity platform matrix for screen readers on desktop; Unity/Unreal/Bevy Steam Deck
status; web SDL_GPU support; thin-client case studies (DFHack, NetHack window ports).
