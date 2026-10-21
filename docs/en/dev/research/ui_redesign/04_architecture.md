# 04. Architecture: mapping the design onto the BN codebase

Facts about the current code were checked in this session unless marked otherwise; the
fuller audits are `research/l_codebase_ui_audit.md` and `research/m_codebase_input_color_font_audit.md`.

## 1. What exists today

- **Windows and text:** every screen draws into `catacurses::window` cells (`src/cursesdef.h`),
  through `mvwprintz`/`draw_border`/`uilist` (`src/output.cpp`, `src/ui.cpp`). In the tiles
  build the cells are rasterised by `CachedTTFFont` (SDL3_ttf, one glyph texture per
  character, fixed cell size) or a bitmap font (`src/sdl_font.cpp`); font sizes are fixed at
  startup (`FONT_*` options "require restart"). Total UI code about 90k LOC (64.9k screens,
  25.5k primitives).
- **Redraw model:** `ui_adaptor` stack in `src/ui_manager.cpp`; each screen runs its own
  blocking input loop; redraws happen on input and on explicit `ui_manager::redraw()`.
- **3D:** `r3d::draw` renders with SDL_GPU into the `w_terrain` rectangle
  (`src/sdltiles.cpp:1684`), then `r3d::draw_hud` draws shapes with `SDL_Renderer` and queues
  `hud_text`s that `draw_r3d_hud_texts` rasterises in the map font (`sdltiles.cpp:1611`).
- **Input:** `input_context` resolves action ids against `data/raw/keybindings/*.json` (585
  actions, 61 contexts). `input_event` carries `keyboard | gamepad | mouse`. Gamepad is legacy
  `SDL_Joystick`: the hat becomes `JOY_*` gamepad events; **buttons are emitted as keyboard
  events with the raw index** (`sdltiles.cpp:3499-3500`), so `JOY_0`-style gamepad bindings
  cannot match them; axes are discarded. Mouse: left/right/wheel/move only; no hit-testing in
  `uilist`. The keybinding editor cannot capture modifiers or middle mouse.
- **Colour:** `nc_color` is a curses pair id; 145 named colours map to a 16-entry RGB palette
  (`data/raw/colors.json` → `windowsPalette`, user-overridable `base_colors.json`,
  name remaps in `custom_colors.json`). No colour-vision or theme support.
- **Accessibility:** none. The only screen-reader-related code is cursor placement so a
  terminal screen reader follows the caret (`ui_manager.h:41`, `string_input_popup.cpp:326`,
  `messages.cpp:887`).
- **No ImGui, no `data/json/ui`.** The CDDA fork's widget-JSON sidebar and ImGui material does
  not apply (research o §4).

## 2. Toolkit decision

### 2.1 Candidates

| Option | Licence | SDL3 + SDL_GPU | Gamepad focus | Accessibility tree | Verdict |
|---|---|---|---|---|---|
| **A. In-house retained widget layer** on `SDL_Renderer` + SDL3_ttf text engine | n/a | yes (it is the existing overlay path) | we write it | we write it (the tree is the a11y tree; bridged to the OS through `accesskit-c`, whose repository ships an SDL example) | **Chosen** |
| B. RmlUi 6.3 | MIT (verified) | `Backends/RmlUi_Renderer_SDL_GPU.cpp` exists (verified via GitHub API) | not verified | none | Rejected: second text/font stack, no a11y, HTML/CSS layout is more than 23 screens need |
| C. Dear ImGui 1.93 | MIT | `imgui_impl_sdl3` + `imgui_impl_sdlgpu3` (verified) | `NavEnableGamepad` | none; issues #4122 open, #7131 closed "typically don't work" (verified) | Rejected: FAQ says it is not for game UIs; CDDA's migration produced a multi-year defect tail (P3); immediate mode makes a stable a11y tree awkward |
| D. Clay (layout only, zlib) or Yoga (flexbox, MIT) under option A | permissive | n/a | n/a | n/a | Deferred: start with a 500-line box/flex layout; adopt Yoga only if wrapping/flex cases outgrow it |
| E. Qt, Slint, Noesis, Gameface, Ultralight | LGPL/GPL/commercial | no SDL_GPU path | varies | Qt yes | Rejected on licence, size or integration |
| F. A different 3D frontend (Godot, Unity, Unreal, Bevy, web) | see `06_frontend_options.md` | replaces it | engine's | Godot: AccessKit built in (experimental); others partial | Rejected on cost (20-30 weeks of renderer and embedding work for ~10 weeks of accessibility saving); decision gate at the end of M1 |

The decisive facts: no free library supplies the hard requirement (a controller focus model
that also feeds a screen reader), and adopting one would still leave the game with two text
renderers and two mouse models, which is what produced CDDA's bug tail. Option A costs more
up front (estimate: 8-12k LOC for the layer plus screens) but each screen is then written
once. Curses is retired (user decision 2026-10-21, `BACKLOG.md` UI-02), so the tree has one
renderer; the retirement itself is costed in `06_frontend_options.md` §4.

### 2.2 Layers

```
 game state ──> view model (plain struct, no UI types) ──> ui::node tree ──> layout ──> sdl backend
                                                              │                      (SDL_Renderer + TTF_TextEngine)
                                                              ├──> narrator (speech backends)
                                                              └──> accesskit bridge (phase 2)
 SDL events ──> input_manager ──> input_context (action ids) ──> focus manager ──> node handlers
```

{{svg:layers}}

The view-model boundary is deliberate: it is also the seam a future frontend swap would use
(`06_frontend_options.md` §1F), so it is kept free of UI and SDL types.

- `src/ui/` new directory: `node.h` (tree, roles, a11y fields), `layout.cpp`, `focus.cpp`,
  `theme.cpp`, `text_engine.cpp`, `backend_sdl.cpp`, `narrator.cpp`,
  `speech_*.cpp` (per OS), widgets (`list.cpp`, `tabs.cpp`, `card.cpp`, `radial.cpp`,
  `body_figure.cpp`, `text_field.cpp`, ...). Each screen in `src/ui/screens/<name>.cpp` with
  its view model in a sibling `<name>_model.cpp` that has no UI includes (testable headless).
- Screens remain `ui_adaptor`s so the existing stack, resize and redraw model keep working and
  legacy screens can be opened from new ones during the migration.

### 2.3 Node and accessibility fields

```c++
enum class ui_role { panel, list, item, button, toggle, choice, slider, text, text_field, tab, tabs, progress, image, radial, card, heading };

struct ui_node {
    ui_role role;
    std::string id;            // stable, for tests and bindings
    std::string name;          // spoken label, translated
    std::string value;         // spoken value when it differs from the text shown
    std::string description;   // optional second sentence
    ui_state state;            // focused, selected, disabled, expanded, marked
    layout_box box;            // computed by layout
    std::vector<ui_node_ptr> children;
    std::function<auto( const ui_event & ) -> bool> on_event;
};
```

The narrator and the phase-2 AccessKit bridge read exactly these fields; a screen that
renders is therefore a screen that speaks. Tests assert the spoken transcript for a scripted
input sequence.

## 3. Text rendering

- New layer: SDL3_ttf **text engine** (`TTF_CreateRendererTextEngine`, `TTF_CreateText`,
  `TTF_DrawRendererText`), one `TTF_Font` per (family, size) in a cache, fallbacks via
  `TTF_AddFallbackFont` (unifont last, as today). Sizes derive from UI scale at layout time;
  changing scale rebuilds the font cache, no restart. Hinting light; SDF not used (needs a
  shader path; research i §2).
- Legacy screens and the map keep `CachedTTFFont` untouched. The HUD texts move from the map
  font to the text engine so they scale with the UI, not with the tileset.
- Readable font: Atkinson Hyperlegible (OFL) shipped in `data/font/`; selected by one option.
- Steam Deck: minimum glyph height check (≥ 9 px at 1280x800) is a unit test over the scale
  floor.
- Requires SDL3_ttf ≥ 3.0 (the CMake `FetchContent` for `SDL3_TTF` must be pinned to a tag;
  check `CMakeLists.txt:582-594` during M1).

## 4. Input abstraction

- Replace `SDL_Joystick` with `SDL_Gamepad` in `sdltiles.cpp`: hot-plug
  (`SDL_EVENT_GAMEPAD_ADDED/REMOVED`), `SDL_EVENT_GAMEPAD_BUTTON_*`, axes with dead zones
  mapped to 8-way movement and to camera, rumble via `SDL_RumbleGamepad`, glyphs via
  `SDL_GetGamepadButtonLabel` / `SDL_GetGamepadType` (SDL 3.2+; system header 3.4.18 has it).
- `input_event` gains modifier flags and a `device` so the editor can capture
  `Ctrl+Shift+K`, middle mouse, and `LT+A`. Binding storage stays JSON; gamepad bindings use
  `SDL_GamepadButton` names (`"gamepad": "south"`), not `JOY_0` indices.
- `input_context` gains a parent chain (`menu` ⊂ `inventory`); conflict detection walks the
  chain. The 61 contexts collapse to 12 by deleting per-screen navigation actions and binding
  them once in `menu`. Legacy contexts remain loadable for mods, mapped onto the new ones.
- "Last device used" drives glyphs everywhere (`ui::glyph( action )` returns text or an icon
  id). Steam Input is optional; SDL labels are enough for Deck Verified glyph rules if the
  default config is complete.
- Hold/toggle: a `holdable` attribute on the action; the focus manager turns press/release
  into begin/end or toggle per the setting.

## 5. Narrator and speech backends

- `ui::narrator` has `announce( text, priority )`, `speak_node( node, verbosity )`,
  `repeat()`, `read_line( ±1 )`; a queue with interrupt priorities; settings: mode (Off / Menus
  / Menus and log), rate, volume.
- `ui::speech` interface with one backend per OS, all loaded lazily and failing to a no-op:
  - Windows: `nvdaControllerClient` (LGPL-2.1, `LoadLibrary`) when NVDA responds, else SAPI 5
    `ISpVoice` (COM, no licence concern).
  - macOS: `AVSpeechSynthesizer` in `speech_macos.mm` (NSSpeechSynthesizer is obsoleted in
    macOS 14).
  - Linux: `libspeechd` via `dlopen` (LGPL-2.1+, daemon required; Flatpak/SteamOS unverified).
- Phase 2: AccessKit (`accesskit-c`, Apache-2.0 or MIT per the repo's LICENSE files; GitHub
  metadata shows BSD-3-Clause because of Chromium-derived parts). Needs a Rust toolchain or
  prebuilt `accesskit-c` binaries in CI; adapters are marked incomplete by the project. Gated on
  a user decision. `accesskit-c` 0.23.1 (2026-09-26) ships `examples/sdl`, which is the
  starting point for the M0 spike (verified: repository listing).

## 6. Theming and colour

- `data/raw/ui/themes/<name>.json`: semantic colour tokens, spacing scale, radius, border,
  font family. `ui::theme::get()` is the only source of colours for the new layer. The
  existing `ui_theme` (modern borders) is folded in.
- Colour vision modes: `data/raw/colors_<mode>.json` remap the 16-entry palette for everything
  legacy (hook in `color_loader<SDL_Color>::load` and `cata_tiles.cpp:3406` which reads the
  palette directly), plus token overrides for the new layer and 3D tints.
- CI tests: WCAG contrast of tokens; pairwise distance under simulated deutan/protan/tritan
  (Machado 2009 matrices implemented in a small test helper, no external dependency).

## 7. Classic 2D and curses

**Curses is retired** (user decision 2026-10-21, `BACKLOG.md` UI-02). There is no character-cell
backend in the new layer. The terminal (`ncurses_def.cpp`) and Windows-console (`wincurse.cpp`)
backends, the `CURSES` build option, its presets and CI jobs go in retirement phase 1; the
in-house cell canvas `cursesport.cpp` stays only as the drawing surface of legacy screens until
the last one is replaced, then goes in phase 3. Scope, counts and risks (230 `TILES` guards in
66 files, 932 `catacurses::` references in 82 files, upstream merge friction) are in
`06_frontend_options.md` §4.

The classic 2D tiles mode (no 3D renderer) stays: it keeps `panels.cpp` as its sidebar and
gets every new screen from the SDL backend at full fidelity, since it already draws through
`SDL_Renderer`.

**Frontend decision gate.** The option of moving to another 3D frontend was evaluated in
`06_frontend_options.md`; the recommendation is to stay on SDL3. The gate is the end of M1:
the M0/M1 spike must show `accesskit-c` working from the SDL window with at least two of
NVDA, VoiceOver and Orca, or the Godot option is re-costed.

## 8. Rendering order and integration

Per frame in the tiles build: SDL_GPU 3D picture → legacy curses windows (unchanged) → new
layer (HUD or the top modal screen, dimming the scene under a modal) → present. The new
layer registers one `ui_adaptor` for the HUD (redraw on `game::draw`) and one per modal
screen. `refresh_display()` ordering already used by the HUD fades is reused.

The sidebar removal predicate `ui::sidebar_hidden()` is `RENDERER_3D && use_tiles`; it is
read by `panel_manager::get_width_*`, `game::draw_panels` and the five sidebar-relative
placements listed in `03_design.md` §3.2.

## 9. Data-driven layouts: how far

Screens are built in C++ from view models with theme tokens; there is no HTML/XML layout
file. Reason: 23 screens, all bound to game data, no modder demand demonstrated, and a
layout language is a second parser to maintain. What is data: themes, colour modes,
keybindings, the advanced config, and a small Lua API (`ui.add_badge`, `ui.add_card`) that
replaces `lua_sidebar_widgets` so mods can show text in the HUD and Journal.

## 10. Testing strategy

- **Pure tests (Catch2, no window):** view models (inventory grouping, sort stability,
  recipe filters, delta computation), layout (wrapping, scale floors), focus navigation
  (spatial moves, tab order), narrator transcripts (scripted input → spoken lines), binding
  conflict detection, theme contrast and CVD distance, advanced config validation.
- **Screenshot tests:** render each screen with the SDL software renderer into an offscreen
  surface at 100 % and 200 % scale, default and high-contrast themes, each colour mode, and
  compare PNGs with a tolerance; stored like the r3d geometry snapshot
  (`tests/data/ui_snapshots/`), regenerated with an env var and named in the commit.
- **Input tests:** synthetic `SDL_Event` sequences through `sdltiles` for gamepad mapping and
  modifier capture (extends `tests/input_test.cpp`).
- **In-game smoke:** the existing Xvfb/software-Vulkan private-save procedure
  (`docs/en/dev/guides/renderer_3d.md`) with the new HUD, plus `[r3d]` tests before any push.
- **Deck check:** a 1280x800 run of the screenshot suite with the glyph-height assertion.

## 11. Performance and localisation

Retained tree with dirty flags; relayout only on scale/theme/resize or model change; redraw
on input as today (no per-frame UI cost in a turn-based game). All strings through `_()`,
including narrator phrases and glyph names; translators get the new strings via the normal
extraction.
