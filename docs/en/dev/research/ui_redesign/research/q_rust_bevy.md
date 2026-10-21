# Q: Rust engines and frameworks as a frontend for a large C++ game

Scope: Bevy (with bevy_a11y/AccessKit), egui, other Rust UI/game frameworks, C++ interop (cxx, autocxx, bindgen), and wgpu as a renderer for shaders currently built for SDL_GPU.

Method and limits:
- GitHub facts were read with `gh api` from the source repositories (default branches as of 2026-10-21). Commit/tag/source-file claims cite the file or API path.
- The WebSearch budget for this session was exhausted partway through, so no web search results were used. Claims that rest only on a WebFetch of a project page are marked as such.
- Labels: **VERIFIED** = checked in the cited source in this session. **NOT VERIFIED** = stated from general knowledge, inference, or not checked; treat as unconfirmed.

Source base URLs used below: `gh:` means `https://github.com/<repo>/...`.

---

## 1. Bevy

### 1.1 Version and release cadence

| Release | Date (GitHub release tag) | Source |
|---|---|---|
| 0.14.0 | 2024-07-13 | gh:bevyengine/bevy/releases (VERIFIED). bevy.org news lists "July 04, 2024" (VERIFIED conflict, see note). |
| 0.15.0 | 2024-11-29 | gh:bevyengine/bevy/releases (VERIFIED) |
| 0.16.0 | 2025-04-24 | gh:bevyengine/bevy/releases (VERIFIED) |
| 0.17.0 | 2025-09-30 | gh:bevyengine/bevy/releases (VERIFIED) |
| 0.18.0 | 2026-01-13 | gh:bevyengine/bevy/releases (VERIFIED) |
| 0.19.0 | 2026-06-18 | gh:bevyengine/bevy/releases (VERIFIED). bevy.org news lists June 19, 2026 (VERIFIED conflict). |
| 0.19.1 | 2026-08-13 | gh:bevyengine/bevy/releases (VERIFIED) |
| 0.20.0 | 2026-10-08 | gh:bevyengine/bevy/releases (VERIFIED); bevy.org/news lists October 08, 2026 (VERIFIED) |

- Gaps between minor releases from 0.14 to 0.20 are roughly 3.5 to 5.2 months, mean about 4.5 months (computed from the dates above; VERIFIED arithmetic).
- Versions are 0.x. Breaking changes between minor versions are the norm under pre-1.0 semver. Migration-guide content per release: NOT VERIFIED (the expected `release-content/` directory was not found at the repo root).
- Note: the default branch Cargo.toml reads `0.20.0-dev` (`gh:bevyengine/bevy/contents/Cargo.toml`, VERIFIED), so the default branch is at or past the 0.20 tag, not a frozen release.

### 1.2 bevy_a11y and AccessKit: what is exposed

Source: `gh:bevyengine/bevy/tree/main/crates/bevy_a11y/src/lib.rs` (VERIFIED).

- `bevy_a11y` depends on `accesskit = "0.25"` (`crates/bevy_a11y/Cargo.toml`, VERIFIED).
- Public types: `AccessibilityPlugin`, `AccessibilityNode` (a component wrapping an `accesskit::Node`), `AccessibilityRequested`, `ManageAccessibilityUpdates`, `ActionRequest` (wrapping `accesskit::ActionRequest`), `AccessibilitySystems` (VERIFIED).
- Doc comment states the crate "does not actually power accessibility features"; it coordinates; binary authors add `AccessibilityPlugin` (VERIFIED, lib.rs lines 18-30).

Node roles actually set by bevy_ui: `gh:bevyengine/bevy/tree/main/crates/bevy_ui/src/accessibility.rs` (VERIFIED):
- `button_changed` sets `Role::Button`.
- `image_changed` sets `Role::Image`.
- `label_changed` sets `Role::Label`.
- Label text is attached via a component with on-insert/on-remove hooks (`on_label_inserted`, `on_label_removed`) (VERIFIED).
- So: buttons, images, and labels are exposed. Text inputs, sliders, checkboxes, tabs, menus, scrollbars, and lists are NOT VERIFIED as exposed by bevy_ui; `bevy_ui_widgets` (crates/bevy_ui_widgets/src: button, checkbox, dialog, menu, slider, tabs, text_input, tree, scrollarea, etc.) depends on `bevy_a11y` and `accesskit = "0.25"` (VERIFIED dependency), but the per-widget role mapping was not read (NOT VERIFIED).
- Text: `bevy_text` uses `parley 0.11.0` and `swash 0.2.6` (`crates/bevy_text/Cargo.toml`, VERIFIED). Whether its text is exposed to AccessKit beyond the Label role: NOT VERIFIED.

### 1.3 Platforms

- The winit adapter is `accesskit_winit` 0.34 (`crates/bevy_winit/Cargo.toml`, VERIFIED). `bevy_winit/src/accessibility.rs` implements `AccessKitPlugin` and an action handler (VERIFIED).
- The AccessKit repo contains adapters: `adapters/` contains android, atspi-common, ios, macos, unix, windows, winit (VERIFIED: `gh:AccessKit/accesskit/tree/main/adapters`).
- Bevy's own root `Cargo.toml` (`gh:bevyengine/bevy/blob/main/Cargo.toml`, around lines 631-632, VERIFIED) has this comment on the `accesskit_unix` feature: "Enable AccessKit on Unix backends (currently only works with experimental screen readers and forks.)". So **Linux AT-SPI is opt-in, and Bevy's own comment says it only works with experimental screen readers and forks.** This is the most important platform finding.
- Windows and macOS are the primary supported paths (VERIFIED by the existence of the adapters and the macOS issue below; default-on status in shipped builds: NOT VERIFIED).
- Web (wasm): NOT VERIFIED. No web adapter directory in AccessKit (VERIFIED), and the AccessKit site lists Windows, macOS, Unix adapters, winit, C and Python bindings, and does not mention web or Android/iOS maturity (VERIFIED by WebFetch of https://accesskit.dev/, which is a summary of a page, not a source file).

### 1.4 Known gaps and issues (bevy issues, verified by `gh api` on bevyengine/bevy)

- #25213 "Rethinking bevy_a11y from the ground up", open, created 2026-07-28. Argues that `accesskit::Node` is used as a persistent data structure instead of a wire format, which hurts composition (VERIFIED, body read). https://github.com/bevyengine/bevy/issues/25213
- #25644 "bevy_a11y does not work correctly on macos with voiceover", created 2026-09-02, closed as completed 2026-09-11. Reported on Bevy 0.19.1: the tree "only updates very infrequently", VoiceOver reports "not responding" and updates only every few seconds, including official bevy_ui examples (VERIFIED, body read; the fix is not verified). https://github.com/bevyengine/bevy/issues/25644
- #20515 "AccessKit is slow and buggy when Bevy is in continuous update mode", open, 2025-08-11. Reports VoiceOver laggy on macOS 15.5 with Bevy 0.17 in continuous mode; switching to winit `desktop_app` update mode removed the problem (VERIFIED, body read). https://github.com/bevyengine/bevy/issues/20515
- #16312 "Feature-gate accessibility in bevy_ui", open, 2024-11-09 (VERIFIED). #7862 "Accessibility by default", open, 2023-03-01 (VERIFIED). Together these say accessibility is not on by default.
- #2724 "Add configurable colorblind modes as a standard feature", open, 2021-08-25 (VERIFIED). No colour-blind or high-contrast mode in `bevy_a11y/src/lib.rs` (VERIFIED by reading the file).
- #21260 "Add gamepad support for feathers / headless widgets", open, 2025-09-28 (VERIFIED title and state).
- #21207 "Add support to bevy_ui for units relative to font sizes", open, 2025-09-25 (VERIFIED title and state). Relevant to text scaling; actual text-scale setting: NOT VERIFIED.
- #25111 "InteractionDisabled and accessibility", open, 2026-07-21 (VERIFIED title).
- #25074 "Add support for accessibility 'object nav'", open, 2026-07-20 (VERIFIED title).
- #17644 "Design of bevy_a11y is BSN-unfriendly", closed (VERIFIED). #20524 "AccessibleName component", closed (VERIFIED).
- Count: search for `label:I-Accessibility` returned at least one label, and `gh search` shows the accessibility topic is active but unfinished. No `A-Accessibility` label exists on the repo (VERIFIED: the label list contains `I-Accessibility` only).

### 1.5 UI maturity, focus and navigation

- `bevy_input_focus` exists with `tab_navigation.rs`, `directional_navigation.rs`, `navigator.rs`, `pointer_focus.rs`, `autofocus.rs`, `gained_and_lost.rs` (VERIFIED, `crates/bevy_input_focus/src`).
- The directional navigation module's doc comment says virtual cursors are a common way to navigate with a gamepad or arrow keys (VERIFIED, `directional_navigation.rs` line 8). Gamepad input is reached through `bevy_input/gamepad` via the crate's `gamepad` feature (VERIFIED, `crates/bevy_input_focus/Cargo.toml`). Whether a gamepad is wired into the directional navigator by default: NOT VERIFIED.
- `bevy_ui` depends on `bevy_input_focus`, `bevy_a11y`, `bevy_text` (VERIFIED, `crates/bevy_ui/Cargo.toml`). There is an `auto_directional_navigation.rs` module in `bevy_ui/src` (VERIFIED).
- `bevy_ui_widgets` exists (see 1.2) with a set of headless widgets (VERIFIED file list).
- Text input: `bevy_text/src/text_edit.rs`, `editing.rs`, `cursor.rs` exist (VERIFIED file list). Issue #23014 "Text Input" closed 2026-02-17 (VERIFIED).
- Scaling: `bevy_ui` has no `text_scale` setting found in this session (NOT VERIFIED either way; the file list was read but not grepped for the feature).

### 1.6 Input remapping and gamepad

- Gamepad: `bevy_gilrs` depends on `gilrs = "0.11.0"` (`crates/bevy_gilrs/Cargo.toml`, VERIFIED). `bevy_input/src/gamepad.rs` exists (VERIFIED file list).
- Input remapping: `leafwing-input-manager` is a separate crate, licence Apache-2.0 (VERIFIED via `gh:Leafwing-Studios/leafwing-input-manager`), tags up to `v0.22` (VERIFIED tag exists; its release date: NOT VERIFIED). Compatibility with Bevy 0.20 specifically: NOT VERIFIED.

### 1.7 Licence

- Bevy root contains `LICENSE-APACHE` and `LICENSE-MIT` (VERIFIED, root listing). Dual Apache-2.0 OR MIT (VERIFIED via file names; the GitHub API reports `Apache-2.0` as the primary SPDX id, VERIFIED).

### 1.8 Shipped games on Steam / Steam Deck

- Not verified. The bevy.org assets page was fetched (WebFetch; first 100,000 of 148,383 characters read) and listed no Steam-shipped games in the visible portion; the games section was cut off (VERIFIED as a partial read). Steam Deck support in any shipped Bevy game: NOT VERIFIED. Web search could not be used.
- `bevy-steamworks` exists as a plugin (VERIFIED via bevy.org assets page, WebFetch).

---

## 2. egui and AccessKit

### 2.1 Versions

- egui 0.37.0 released 2026-10-15; 0.36.2 on 2026-09-08; 0.36.0 on 2026-08-05 (VERIFIED: `gh:emilk/egui/releases`).

### 2.2 Integration status

- `crates/egui/Cargo.toml` has a direct dependency `accesskit.workspace = true` (VERIFIED, line 99) and serde feature gating (VERIFIED).
- `crates/egui-winit/Cargo.toml`: `accesskit = ["dep:accesskit_winit"]` is an opt-in feature; the `default` feature list is `clipboard, links, wayland, winit/default, x11` and does not include `accesskit` (VERIFIED, lines 27-30).
- `crates/eframe/Cargo.toml`: the `default` feature list includes `"accesskit"` (VERIFIED, lines 20-32), so eframe enables it by default. The eframe `accesskit` feature maps to `egui-winit/accesskit` (VERIFIED, line 42).
- So: with eframe defaults, AccessKit is on. With raw egui-winit, you must enable it.
- Documented platform behaviour, screen reader support depth, and per-widget coverage: NOT VERIFIED beyond the issue list below (docs.rs fetch returned no feature detail).

### 2.3 Open issues in emilk/egui (VERIFIED by gh search, state and titles)

- #8671 "eframe (native): an occluded or minimized window never answers AccessKit: action requests and the initial tree request wait until the window is shown", open. Relevant to a game that minimises.
- #8700 "egui_extras Table: expose table, row and cell roles to AccessKit", open.
- #8733 "A scroll target inside a ScrollArea that cannot scroll on that axis never reaches the enclosing ScrollArea", open.
- #8674 "Modifiers::shift_only ignores ctrl, so on macOS Ctrl+Shift+Tab moves focus like Shift+Tab", open.
- #3647 "accesskit panics when giving invalid Id to Response::labelled_by", open. Panic risk.
- #2647 "accessibility: expose live regions", open.
- #2831 "Keyboard Shortcuts To Open Menus", open.
- #8699 "TextEdit ignores AccessKit SetValue, so UI Automation can't set its text", closed.
- #8474 "Add WidgetType::Tab", closed.

### 2.4 Limitations for a game HUD (analysis)

- The immediate-mode model rebuilds widgets every frame; an AccessKit tree for a game HUD would have to be derived per frame. Whether egui's AccessKit tree is updated only on change: NOT VERIFIED.
- Game HUDs usually need custom drawing, which egui supports through painter calls, but accessibility labels for custom-drawn shapes depend on the developer supplying them: NOT VERIFIED per API.
- Licence: egui repo SPDX is Apache-2.0 (VERIFIED via `gh api repos/emilk/egui`).

---

## 3. Other Rust frameworks

| Project | Licence (VERIFIED) | Accessibility status (VERIFIED unless stated) | Notes |
|---|---|---|---|
| Slint | Repo SPDX `NOASSERTION`; `LICENSES/` has `GPL-3.0-only.txt` and `LicenseRef-Slint-Royalty-free-2.0.md` | `internal/backends/winit/Cargo.toml` has an `accessibility` feature using `accesskit 0.25` and `accesskit_winit 0.34` (VERIFIED). Open issue #3867 says the accessibility feature has a large performance impact on big ListViews. | Latest release v1.18.1, 2026-09-21 (VERIFIED). Royalty-free licence (VERIFIED text, LicenseRef-Slint-Royalty-free-2.0.md): usable for desktop/mobile/web Applications, royalty-free; attribution required: display the `AboutSlint` widget in an About dialog reachable from the top-level menu (or the splash screen), and display the Slint attribution badge on a public webpage where binaries are downloaded. The licence does not permit distributing the Software alone. For a closed-source game, the attribution condition is a product decision. |
| Xilem / Masonry | Apache-2.0 (VERIFIED) | Masonry depends on `accesskit`, `accesskit_consumer`, `accesskit_winit` (VERIFIED, `masonry/Cargo.toml`). Open issue #924 "Automated testing of accessibility"; #1003 "Noting widget relationships" open. Closed #1733 "Accessibility not working on Linux". | Alpha/maturity status: NOT VERIFIED. |
| Iced | Not checked | No AccessKit integration found in issue list. #552 "Implement accessibility support" open; #489 "In native UI, cannot focus most widgets, control them via keyboard, or tab between widgets" open (VERIFIED titles). | Treat iced as having no shipped accessibility. Repo licence: NOT VERIFIED. |
| Fyrox | MIT (VERIFIED) | `fyrox-ui/Cargo.toml` contains no `accesskit` dependency (VERIFIED by grep). Issue search for "accessibility" returned only an unrelated closed issue (VERIFIED). | No accessibility support found. |
| macroquad | Apache-2.0 (VERIFIED) | `src/` has no accessibility module; file list has `ui.rs`, `text.rs`, `input.rs` only (VERIFIED). | No accessibility support. |
| winit AccessKit adapter | winit root `Cargo.toml` has zero `accesskit` references (VERIFIED) | AccessKit's own winit adapter is `accesskit_winit` in the AccessKit repo (`adapters/winit`, VERIFIED). AccessKit site: "If you are using the winit ... library for Rust, you should use the AccessKit winit adapter" (VERIFIED via WebFetch of accesskit.dev). | Bevy and egui use `accesskit_winit`. |

Note on AccessKit consumers: the AccessKit site names Bevy, egui, Freya, Slint, Xilem as integrating AccessKit (VERIFIED via WebFetch of accesskit.dev, summarised page).

---

## 4. C++ interop

### 4.1 cxx

- Licence: Apache-2.0 (VERIFIED). Latest release 1.0.205, 2026-10-11 (VERIFIED).
- Exceptions: `Result<T>` in a bridge maps C++ exceptions to Rust `Result`. An exception thrown from an `extern "C++"` function **not** declared to return `Result` calls `std::terminate`. A panic in any `extern "Rust"` function aborts the process (VERIFIED: `gh:dtolnay/cxx/blob/master/book/src/binding/result.md`). Consequence for a large C++ game: every bridged function that can throw must be declared `Result` or wrapped in a noexcept shim.
- `std::string`: exposed to Rust as `CxxString`; Rust cannot obtain a `CxxString` by value (VERIFIED: `book/src/binding/cxxstring.md`).
- `std::vector`, `std::map`, `std::optional`, and other STL types: only partially verified. The `vector.md` page read returned empty content; full support matrix: NOT VERIFIED.
- Blocking input loops and callbacks into Rust from C++ (the game's main loop calling Rust): NOT VERIFIED beyond the general bridge model.
- Lua (sol2) interop through cxx: NOT VERIFIED (not checked).

### 4.2 autocxx

- Repository `google/autocxx` has `archived: true` (VERIFIED via gh API). Last release v0.30.0, 2025-03-03 (VERIFIED). Last commit on default branch 2026-09-15 (VERIFIED). Treat as unmaintained for planning purposes; the archive status is the decisive fact.

### 4.3 bindgen

- Latest release v0.73.2, 2026-10-05 (VERIFIED: `gh:rust-lang/rust-bindgen/releases`). Suited to C APIs; for a C++ codebase it generates opaque bindings for classes and needs a C shim for most of the game's code (general knowledge; NOT VERIFIED in this session).

### 4.4 Threading, build, CI, distribution

- Bevy's task pool and threading model: NOT VERIFIED in this session.
- cargo + CMake build integration: NOT VERIFIED (not read). The repo's AGENTS.md specifies a CMake preset and `cataclysm-bn-tiles` target (VERIFIED from `/home/richard/Projects/Cataclysm-BN-3D/AGENTS.md`); any Rust layer would need cargo driven from CMake or the reverse.
- GitHub-hosted runner Rust availability and cost: NOT VERIFIED. AGENTS.md states "No CI on this fork: GitHub Actions does not run here" (VERIFIED from AGENTS.md), so CI cost is not a current constraint.
- Binary distribution for Linux/macOS/Windows: NOT VERIFIED beyond Bevy's own template claim: `bevy_game_template` / official template advertises Windows, Linux, macOS and wasm release workflows (VERIFIED via bevy.org assets page WebFetch, summarised).

---

## 5. Rendering: wgpu and SDL_GPU shader reuse

### 5.1 wgpu versions and shader paths

- Latest release v30.0.1, 2026-08-22 (VERIFIED: `gh:gfx-rs/wgpu/releases`). Bevy 0.20 depends on `wgpu-types = "30"` (VERIFIED, `crates/bevy_winit/Cargo.toml`).
- naga front-ends: `naga/src/front/` contains `spv`, `wgsl`, `glsl` (VERIFIED). naga features: `spv-in`, `spv-out`, `wgsl-in`, `wgsl-out`, `glsl-in`, `glsl-out` (VERIFIED, `naga/Cargo.toml` lines 24-62).
- So naga can ingest SPIR-V into its IR, and wgpu uses naga for WGSL input. Whether wgpu's standard `create_shader_module` path accepts SPIR-V directly: NOT VERIFIED in this session.
- wgpu `Features::PASSTHROUGH_SHADERS` (bit 52) with `Device::create_shader_module_passthrough` (VERIFIED: `wgpu-types/src/features.rs` ~line 1060). This is the path for submitting backend-native shaders without naga translation. Which backends accept it and what SPIR-V subsets pass: NOT VERIFIED. Consider the passthrough path experimental until checked.

### 5.2 SDL_GPU shaders via SDL_shadercross

- SDL_shadercross licence: Zlib (VERIFIED: `gh:libsdl-org/SDL_shadercross`).
- README (`README.txt`, VERIFIED): "It takes SPIRV or HLSL as the source and outputs DXBC, DXIL, SPIRV, MSL, or HLSL." It depends on SPIRV-Cross for SPIR-V translation and on DXC for DXIL. **It does not list WGSL or any direct WGSL output.**
- Reuse path (analysis, based on the above):
  1. HLSL -> SPIR-V with shadercross (or DXC), as the current SDL_GPU pipeline already does.
  2. SPIR-V -> naga IR -> WGSL, or SPIR-V passthrough, for wgpu.
  - Coverage of naga's SPIR-V front-end for the game's actual shaders: NOT VERIFIED. Requires a test with the real shaders.
- AGENTS.md notes HLSL changes require shadercross recompilation and that the precompiled fallback does not rebuild changed sources (VERIFIED from AGENTS.md). A wgpu path would need its own shader build step.
- Uniform/layout compatibility (std140 vs SDL_GPU): NOT VERIFIED.

### 5.3 Voxel meshing from C++-produced buffers

- Feasibility of handing C++-produced vertex/index buffers to wgpu via FFI: NOT VERIFIED in this session. Analysis: buffer ownership and lifetime across the cxx boundary, and copy cost per frame, need a prototype. AGENTS.md notes that camera mouse events must not render within `SDL_PollEvent` draining and that geometry fingerprints cannot detect GPU command accumulation (VERIFIED from AGENTS.md); any new renderer front-end must respect this.

---

## Summary table

| Topic | Finding | Status |
|---|---|---|
| Bevy version | 0.20.0 released 2026-10-08 | VERIFIED |
| Bevy cadence | minor releases every ~3.5-5 months since 0.14 | VERIFIED |
| Bevy a11y roles | Button, Image, Label only in bevy_ui | VERIFIED |
| Bevy a11y Linux | AT-SPI opt-in, "only works with experimental screen readers and forks" | VERIFIED (Bevy root Cargo.toml comment) |
| Bevy macOS VoiceOver | Reported lag and infrequent tree updates (#20515, #25644 closed) | VERIFIED (issues) |
| Bevy colour-blind mode | Not present; issue #2724 open | VERIFIED |
| Bevy focus/gamepad nav | bevy_input_focus has directional and tab nav; gamepad feature gated | VERIFIED (structure); default wiring NOT VERIFIED |
| Bevy licence | Apache-2.0 OR MIT | VERIFIED |
| Bevy shipped Steam Deck games | Not found | NOT VERIFIED |
| egui version | 0.37.0 on 2026-10-15 | VERIFIED |
| egui a11y default | ON in eframe defaults; OFF in egui-winit defaults | VERIFIED |
| egui open a11y issues | occluded window does not answer AccessKit (#8671) | VERIFIED |
| Slint licence | Royalty-free with AboutSlint + badge conditions; GPL-3.0 alternative | VERIFIED |
| Xilem/Masonry | AccessKit dependency present; Apache-2.0 | VERIFIED |
| Iced | No accessibility; #552 open | VERIFIED |
| Fyrox / macroquad | No AccessKit | VERIFIED |
| cxx exceptions | Uncaught exception -> std::terminate | VERIFIED |
| autocxx | Archived | VERIFIED |
| bindgen | v0.73.2 2026-10-05 | VERIFIED |
| wgpu | v30.0.1 2026-08-22; naga spv-in; PASSTHROUGH_SHADERS | VERIFIED |
| SDL_shadercross output | DXBC, DXIL, SPIRV, MSL, HLSL; no WGSL | VERIFIED |
| naga SPIR-V coverage for game shaders | Not tested | NOT VERIFIED |

## Could not verify

- Bevy shipped Steam or Steam Deck games (web search budget exhausted; bevy.org assets page only partly read).
- Bevy migration-guide contents and breaking-change counts per release.
- Bevy per-widget a11y role coverage in `bevy_ui_widgets`; text scaling setting; default wiring of gamepad navigation.
- Bevy on web (wasm) accessibility.
- AccessKit behaviour on Android and iOS beyond the existence of adapters.
- egui AccessKit update model and custom-drawn widget labelling.
- Slint, Xilem, Iced maturity statements beyond the cited issues; Iced repo licence.
- cxx std container support matrix; cxx build integration with CMake; Lua (sol2) interop.
- Threading model across the Bevy/C++ boundary; cargo-from-CMake build recipe.
- Runner cost for Rust on GitHub-hosted runners.
- wgpu acceptance of SPIR-V via the standard path; passthrough backend coverage; naga SPIR-V coverage for the game's shaders.
- Voxel mesh buffer handover cost across FFI.
