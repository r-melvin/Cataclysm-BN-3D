# UI library evaluation for an SDL3 + SDL_GPU game (C/C++)

Research date: 2026-10-21. Scope: data-driven/themable layout, gamepad navigation, text scaling, accessibility tree for screen readers, permissive licensing, SDL3 + SDL_GPU compatibility (with an SDL_Renderer 2D overlay as an option).

Evidence labels: **VERIFIED** = read from a fetched page, the GitHub API, or a search snippet that names the URL. **UNVERIFIED** = memory or unconfirmed. **NOT FOUND** = searched, no evidence either way.

Release and push dates come from the GitHub REST API (`gh api`) on 2026-10-21 unless noted. "Last push" is the last commit to the default branch, not a release.

## 1. Comparison table

| Library | Licence | SDL3 integration | GPU path | Text / scaling | Gamepad / keyboard nav | Screen-reader / a11y tree | Last release (GitHub) | Last push | Integration cost (for this game) |
|---|---|---|---|---|---|---|---|---|---|
| **RmlUi** 6.3 | MIT (VERIFIED) | SDL backends for SDL2 and SDL3 (`Backends/RmlUi_Platform_SDL.cpp`) | **SDL_GPU backend shipped** (`Backends/RmlUi_Backend_SDL_GPU.cpp`, `RmlUi_Renderer_SDL_GPU.cpp`); also GL2/GL3/VK/DX11/DX12 | Not checked in depth; RCSS/HTML-like styling, fonts via a font engine (UNVERIFIED which) | Not verified in fetched pages | None mentioned (VERIFIED: fetched page does not mention it) | 6.3 on 2026-08-22 | 2026-10-17 | Medium. Retained-mode, data-driven markup and CSS-like styling fit the stated requirement best. Renderer is ready-made; you still wire input and layout. |
| **Dear ImGui** v1.93.0 | MIT (VERIFIED) | `imgui_impl_sdl3.cpp` and `imgui_impl_sdlgpu3.cpp` both exist (VERIFIED: `backends/` listing) | SDL_GPU backend `sdlgpu3/` + `imgui_impl_sdlgpu3.cpp`; SDL_Renderer3 backend also exists; page says "Prefer using SDL_GPU!" | Font atlas; FAQ: "Since 1.92 (June 2025) fonts may be dynamically used at any size." Style values "not easily scalable dynamically"; FAQ recommends `style.ScaleAllSizes(factor)` (VERIFIED) | `io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad` with a supporting backend (VERIFIED, FAQ). Keyboard tab nav works (issue #4122 comment says tab cycles all items since 1.89.4) | **None.** FAQ does not mention accessibility. Issue #7131 closed: maintainer said accessibility systems "typically don't work with dear imgui output" (VERIFIED). Issue #4122 open. | 1.93.0 on 2026-10-14 | 2026-10-16 | Low to medium. Immediate mode, easy to bolt onto an existing SDL_GPU frame. Poor fit for end-user menus that need themes and accessibility. |
| **Clay** (layout) | Zlib (VERIFIED) | SDL3 renderer exists in `renderers/SDL3`, SDL3 example exists in `examples/SDL3-simple-demo` (VERIFIED: GitHub contents listing). Renderer shown in search snippet: borders/rounded borders added. | None. Layout only; you draw the output list yourself. | Not applicable; you supply text measurement callbacks (UNVERIFIED detail) | Not provided | None; layout only | v0.14 on 2025-06-06 (no release since) | 2026-05-20 | Low for layout, but you still build the input, rendering, and any focus model yourself. Single C header. |
| **Nuklear** | Public domain **or** MIT, per README (VERIFIED). GitHub reports NOASSERTION. | Demo folder lists GLFW, SDL, X11, GDI, D3D, Vulkan "and more" (VERIFIED). SDL_Renderer/SDL_GPU not confirmed. | Not confirmed for SDL_GPU | Font baking via its own font API (UNVERIFIED detail) | Not verified | None mentioned | v4.13.3 on 2026-05-05 | 2026-10-17 | Medium; you write the SDL_GPU backend. Immediate mode, C, very small. |
| **microui** (rxi) | MIT (VERIFIED) | None mentioned (VERIFIED: page does not mention SDL) | Renderer-agnostic; does no drawing | None | Not verified | None | No release tag | 2024-08-13 (stale) | Low for tiny tool UIs; not a full toolkit. Reference only. |
| **NanoGUI** | BSD-style (VERIFIED) | No SDL; GLFW + OpenGL 3.x + NanoVG (VERIFIED) | OpenGL 3.x only | NanoVG-drawn | Not verified | None | No release tag | 2023-04-28 | High. Maintenance-only per README (VERIFIED). Not usable with SDL_GPU. Not recommended. |
| **NanoVG** | Not checked in this pass | Not checked | OpenGL-based | Not checked | N/A | N/A | Not checked | Not checked | Not recommended. Drawing library, not a UI toolkit. |
| **LVGL** v9.6.0 | MIT (VERIFIED) | SDL display driver exists in `src/drivers/sdl/` (VERIFIED: GitHub contents; also "SDL as a desktop simulator option" in fetched page) | Software/SDL texture/OpenGL ES drivers (`lv_sdl_sw.c`, `lv_sdl_texture.c`, `lv_sdl_egl.c`); no SDL_GPU driver seen | Built-in fonts, scalable via config (UNVERIFIED detail) | Keypad/encoder input groups exist in LVGL (UNVERIFIED detail) | **None mentioned** (VERIFIED: fetched page silent). Search returned nothing for LVGL a11y. | 2026-09-16 | 2026-10-20 | Medium-high. Built for embedded; its own widget and style system is C and not data-driven markup. |
| **AccessKit** | BSD-3-Clause per GitHub; README says Apache-2.0 or MIT, at your option (VERIFIED, note the discrepancy in GitHub metadata) | N/A: this is an accessibility bridge, not a UI kit | N/A | N/A | N/A | **Yes, this is the a11y layer.** Rust core; platform adapters for Windows UIA, macOS, Linux AT-SPI, Android; winit adapter (VERIFIED). C bindings in separate repo `accesskit-c`, generated by cbindgen (VERIFIED). Python bindings on PyPI (VERIFIED). | accesskit_winit 0.34.1, 2026-09-25 | 2026-10-18 | High. You would build the accessibility tree yourself and feed it. No SDL adapter exists in the listing. |
| **Yoga** | MIT (VERIFIED) | N/A (layout only) | N/A | N/A | N/A | None | v3.2.1 on 2024-12-13 | 2026-10-21 | Low-medium as a flexbox layout engine inside your own renderer. C++ with bindings. |
| **Slint** | Triple: Royalty-free, GPLv3, or Commercial (VERIFIED) | No SDL integration checked. Renderers: FemtoVG (OpenGL ES 2.0), Skia, software (VERIFIED) | Not SDL_GPU | Not verified | Not verified | Not verified on the page. 2022 issue suggests AccessKit integration (VERIFIED: search snippet, slint-ui/slint#32). Status NOT FOUND. | v1.18.1 on 2026-09-21 | 2026-10-21 | High. Has a C++ API (VERIFIED). Licence is a blocker unless you take the royalty-free terms, which need an application that is not open source or an application-level commercial terms check. |
| **Qt 6** | LGPLv3 / GPLv3 / commercial (VERIFIED via qt.io FAQ and search) | No SDL backend; you would embed Qt Quick or Widgets with your own SDL_GPU window (UNVERIFIED feasibility) | Qt RHI (UNVERIFIED for SDL_GPU) | Mature | Mature | **Yes: QAccessible on MSAA, macOS, AT-SPI** (VERIFIED: doc.qt.io/qt-6/qaccessible.html) | Not checked this pass | Not checked | Very high. Static linking and LGPL relinking obligations (VERIFIED: qt.io FAQ summary). Not practical for a CDDA-style SDL game. |
| **Noesis GUI** | Commercial (VERIFIED: forum and trade article; current terms not fetched) | Not verified. Documented plugins: Unity, Unreal, Unigine, Ogre, Xenko (VERIFIED: trade article via search). Custom C++ SDK exists (VERIFIED: older forum post). | Not verified | XAML; not verified | Not verified | Not verified | n/a | n/a | High cost, paid licence per project. Could not reach noesisgui site (DNS failure). |
| **Coherent Gameface** | Commercial, per title (VERIFIED: search snippet from docs.coherent-labs.com) | Not verified; search returned no SDL info | Not verified | HTML/CSS (UNVERIFIED) | Not verified | Not verified | n/a | n/a | High cost. Per-title licence. Not recommended without a quote. |
| **Ultralight** | Core proprietary; free for indie under $100K revenue; paid otherwise (VERIFIED: github README summary via search) | Not verified | Not verified | HTML/CSS via WebCore (LGPL/BSD parts) | Not verified | Not verified | No release tag | 2024-04-22 (stale in GitHub data) | High. Commercial terms, last push April 2024. Not recommended. |
| **FLTK** | LGPL with exception (UNVERIFIED) | No | No | No | No | No | Not checked | Not checked | Not recommended; out of scope. |
| **libui** | MIT (UNVERIFIED) | No | No | Native widgets only | No | Native a11y on each OS (UNVERIFIED) | Not checked | Not checked | Not recommended; no SDL, no GPU. |
| **Godot UI** | MIT (UNVERIFIED) | Not embeddable | N/A | N/A | N/A | Godot 4.5 added screen reader support via AccessKit (VERIFIED: caniplaythat.com snippet) | N/A | N/A | Not an option: it is an engine, not a library. |

## 2. Per-library notes

### RmlUi (rank 1 for the stated requirements)
- Licence: MIT (https://github.com/mikke89/RmlUi). Current release 6.3, 2026-08-22 (GitHub API).
- SDL3 + SDL_GPU: the official Backends folder ships `RmlUi_Backend_SDL_GPU.cpp`, `RmlUi_Renderer_SDL_GPU.cpp/.h`, `RmlUi_Platform_SDL.cpp/.h`, and a `RmlUi_SDL_GPU` subfolder (VERIFIED: https://github.com/mikke89/RmlUi/tree/master/Backends via GitHub API). The fetched README lists "SDL (2 and 3)" and "SDL GPU" among backends.
- This is the only candidate in the list that already has an SDL_GPU renderer shipped. That cuts integration cost materially for this project.
- Data-driven: RML (HTML-like) documents and RCSS (CSS-like) stylesheets, so layouts and themes live in data files. This is the closest match to the "data-driven/themable" requirement.
- Gamepad/keyboard focus: NOT VERIFIED. The fetched page covers touch and IME but not gamepad focus. Check the `Source` for a focus or navigation model before relying on it.
- Accessibility: NOT FOUND in the fetched page. Assume none.
- Risk: the sources and docs are less broad than Qt's; C++ and smaller community. 6.3 release is recent.

### Dear ImGui (rank 2)
- Licence: MIT. v1.93.0 on 2026-10-14; last push 2026-10-16 (GitHub API).
- Backends: `imgui_impl_sdl3.cpp`, `imgui_impl_sdlgpu3.cpp` (with `sdlgpu3/` subfolder and shaders header), `imgui_impl_sdlrenderer3.cpp` (VERIFIED: https://github.com/ocornut/imgui/tree/master/backends and the GitHub API listing). Doc says "Prefer using SDL_GPU!" for the SDL3 renderer path (VERIFIED: https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md).
- Gamepad: `ImGuiConfigFlags_NavEnableGamepad` (VERIFIED: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md). Gamepad keys are supplied via `io.AddKeyEvent`/`AddKeyAnalogEvent` with `ImGuiKey_GamepadXXX` (VERIFIED: BACKENDS.md summary).
- Text scaling: fonts are used at any size since 1.92 (VERIFIED: FAQ). Style sizes are not dynamically scalable; use `ScaleAllSizes` at startup (VERIFIED: FAQ). Rapid DPI change still needs a rebuild of style (UNVERIFIED consequence).
- Accessibility: none. Issue #7131 (F1 menu not blind accessible) was closed 2024-01-03 with the maintainer saying accessibility systems "typically don't work with dear imgui output." Issue #4122 ("Focus navigation and other features for accessibility?") is open, last activity January 2024, and says "screen readers currently won't work with GUI created with dear imgui" (VERIFIED: https://github.com/ocornut/imgui/issues/4122 and https://github.com/ocornut/imgui/issues/7131).
- Positioning: FAQ says "Dear ImGui is NOT designed to create a user interface for games" and is "very programmer-centric" (VERIFIED: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md). This is the strongest argument against using it as the main player-facing UI.
- Docking: not checked in this pass. Check `imgui_internal` docking branch docs before claiming.

### Clay (layout-only, rank 4 as a standalone)
- Licence: Zlib (VERIFIED: https://github.com/nicbarker/clay).
- Renderer-agnostic, outputs render commands. Renderers folder contains `SDL3`, `SDL2`, `raylib`, `sokol`, `GLES3`, `terminal`, `web`, and others (VERIFIED: GitHub contents listing). The SDL3 renderer exists, and a 2025 commit notes the SDL3 example was updated (search snippet via git.objectionable.solutions mirror; UNVERIFIED on canonical source).
- Layout only: no accessibility, no focus model. You would own the input and a11y.
- Latest tag v0.14 (2025-06-06). The push date is recent (2026-05-20), so it is maintained, but without tagged releases.

### Nuklear (rank 6)
- Licence: public domain or MIT (VERIFIED: https://github.com/Immediate-Mode-UI/Nuklear).
- SDL backends are in the demo folder list (VERIFIED: fetched README), but an SDL_GPU backend is not documented. You would write the GPU bridge.
- No accessibility. Immediate mode, C.

### microui (rank 9)
- MIT, ANSI C, immediate mode, no drawing (VERIFIED: https://github.com/rxi/microui). Last push 2024-08-13. README says feature PRs will likely not be merged. Too small for this scope.

### NanoGUI / NanoVG (rank 10)
- NanoGUI: BSD-style, OpenGL 3.x, GLFW, maintenance-only (VERIFIED: https://github.com/wjakob/nanogui). Last push 2023-04-28. Not suitable for SDL_GPU.
- NanoVG: not checked in detail; it is a 2D vector-drawing library, not a UI toolkit.

### LVGL (rank 5)
- Licence: MIT (VERIFIED: https://github.com/lvgl/lvgl). v9.6.0 on 2026-09-16; last push 2026-10-20.
- SDL display driver: `src/drivers/sdl/` with `lv_sdl_sw.c`, `lv_sdl_texture.c`, `lv_sdl_egl.c`, plus keyboard, mouse, and mousewheel input files (VERIFIED: GitHub contents API). No SDL_GPU driver seen.
- Accessibility: none found (search and fetched page, VERIFIED negative within those sources).
- Embedded-first; style system is C code, not data-driven markup.

### AccessKit (rank 3 as a complement, not as a toolkit)
- Licence: GitHub reports BSD-3-Clause; README says Apache-2.0 or MIT (VERIFIED: https://github.com/AccessKit/accesskit). The two statements differ; check `LICENSE-*` files before relying on either.
- Languages: Rust canonical; C via `accesskit-c` (separate repo, cbindgen header); Python via PyO3 (VERIFIED: GitHub README fetch).
- Platform adapters: Windows UI Automation, macOS, Linux AT-SPI, Android, and a winit wrapper (VERIFIED: search snippet and README). An SDL adapter is not listed.
- Toolkit adapters: egui (merged, per search snippet, egui PR #2294 link: https://github.com/emilk/egui/pull/2294), Bevy (`bevy_a11y`, snippet), Godot 4.5 (snippet). Slint had an open 2022 proposal (https://github.com/slint-ui/slint/issues/32), status NOT FOUND. Makepad: NOT FOUND.
- Implication: AccessKit is the only standalone path to a real screen-reader tree in C/C++. You would have to build the tree from your own widget model and wire the platform adapter yourself. That is a large cost, but the only one on the list that actually reaches UIA/AT-SPI/NSAccessibility.

### Yoga (layout helper)
- MIT, C++ with bindings, flexbox (VERIFIED: https://github.com/facebook/yoga). v3.2.1 (2024-12-13); pushed 2026-10-21. No accessibility. Usable as a layout engine under a custom renderer.

### Slint (rank 8 due to licence)
- Triple licence: Royalty-free, GPLv3, Commercial (VERIFIED: https://github.com/slint-ui/slint). The royalty-free terms apply to proprietary desktop, mobile, or web apps; check the exact terms before adopting. GPLv3 is incompatible with a closed-source release.
- C++ API exists (VERIFIED: `api/cpp`). Renderers: FemtoVG, Skia, software (VERIFIED).
- Accessibility: not confirmed on the current page. A 2022 issue proposed AccessKit (VERIFIED: https://github.com/slint-ui/slint/issues/32). Current status NOT FOUND.
- No SDL_GPU path. Would need a separate window and renderer, which conflicts with the SDL_GPU frame.

### Qt 6 (rank 7, not practical)
- Dual licence: LGPLv3/GPLv3 or commercial (VERIFIED: qt.io FAQ and forum summaries via search). Static linking under LGPL raises relinking obligations; the official guidance favours dynamic linking (VERIFIED: https://www.qt.io/faq/3.7.-what-are-my-obligations-when-using-qt-under-the-lgpl; searched via Qt FAQ and forum).
- Accessibility: QAccessible covers MSAA, macOS Accessibility, AT-SPI (VERIFIED: https://doc.qt.io/qt-6/qaccessible.html). This is the most complete a11y tree on the list.
- Cost: a Qt runtime, a second event loop, and a second renderer next to SDL_GPU. High.

### Noesis GUI (rank 9 for this project)
- Commercial, XAML-based. Trade article lists Unity, Unreal, Unigine, Ogre, Xenko integrations (VERIFIED via search snippet). Older forum post gives a per-project indie price of $195 (dated, UNVERIFIED current).
- Current licence terms and the custom-engine/SDL path could not be verified: https://www.noesisgui.com/ returned DNS failure in this session.

### Coherent Gameface (rank 10)
- Commercial, per title, source access one-off fee (VERIFIED: https://docs.coherent-labs.com/cpp-gameface/what_is_gfp/overview/ via search). Supported platforms include PC (Windows, macOS, Linux). SDL and accessibility were NOT FOUND in search.

### Ultralight (rank 11)
- Core proprietary; free for indie under $100K annual revenue; paid tiers for others (VERIFIED: https://github.com/ultralight-ux/Ultralight README summary via search). Last GitHub push 2024-04-22 (VERIFIED: GitHub API). Stale. SDL and accessibility: NOT FOUND.

### Godot UI
- Not embeddable as a UI library. Godot 4.5 added screen reader support through AccessKit (VERIFIED: caniplaythat.com snippet, https://caniplaythat.com/?p=15209). Not an option for an SDL C++ game.

### FLTK, libui
- Not checked in depth; no SDL_GPU path and no a11y evidence found. Excluded from the ranking. FLTK licence is LGPL with exception (UNVERIFIED). libui is MIT (UNVERIFIED).

## 3. Cataclysm: Dark Days Ahead (CDDA) ImGui lessons

Sources are GitHub PR and issue pages and search snippets, read on 2026-10-21. Reading status is noted per item.

1. **ImGui is in upstream CDDA as vendored third-party code.** A search snippet quotes an assertion path `src/third-party/imgui/imgui_tables.cpp:1464` from a tiles build (VERIFIED as snippet only: https://github.com/CleverRaven/Cataclysm-DDA/pull/73134). Not confirmed by opening the tree.
2. **Mission UI migration (PR #73134).** Merged early May 2024 per search snippet. A reviewer reported a table assertion crash; changing a column to `ImGuiTableColumnFlags_WidthFixed` fixed it (snippet, VERIFIED as snippet: https://github.com/CleverRaven/Cataclysm-DDA/pull/73134). Android selection and height issues were raised later and redirected to a separate bug report (snippet).
3. **Item info window (PR #73456).** Fetched page: merged 2024-09-03 by Maleclypse after being marked stale in June and reopened in August. Known issues at merge: damage info "slightly broken", X-button close does not exit `execute`, keyboard scrolling needs refactor, open AIM window sizing question. Works both as a standalone window and as embeddable controls. Used for inventory selector, AIM, and crafting; worn/wielded items still use the old UI (VERIFIED: https://github.com/CleverRaven/Cataclysm-DDA/pull/73456).
4. **Open migration issue.** Good-first-issue #78465 asks contributors to migrate string popups to ImGui one file per PR (snippet: https://goodfirstissue.org/CleverRaven/Cataclysm-DDA/issues/78465, not a github.com URL; treat as a secondary source).
5. **Performance, fonts, and mouse problems in CDDA: NOT FOUND.** Searches returned no CDDA issue on ImGui performance, font, or mouse behaviour. The only font-related generic guidance came from the Dear ImGui troubleshooting page (white squares from font atlas, large atlas) (snippet, https://www.mintlify.com/ocornut/imgui/guides/troubleshooting, not the official source).
6. **Curses vs tiles.** PR #73456 notes ImGui is for SDL/tiles, with ImTui for curses builds (VERIFIED: fetched page). No CDDA evidence found on ImTui's status.
7. **Local repo (Cataclysm-BN-3D).** `git ls-files | grep imgui` returns nothing; `src/third-party` contains only `pinyin`. This repo does not vendor ImGui. The local `src/r3d_gpu.cpp` and `src/dynamic_atlas.cpp` exist and are the SDL_GPU path (VERIFIED by file listing).

## 4. Other games

- **Caves of Qud:** wiki says the UI uses Source Code Pro, fixed 80x25 grid, 16x24 tiles (VERIFIED: https://wiki.cavesofqud.com/wiki/Visual_Style). Unity interview says Bucklew experimented with running Caves of Qud inside Unity as a host, mainly for mobile releases (VERIFIED: https://unity.com/resources/systems-driven-design-in-caves-of-qud). Whether its UI is uGUI, custom, or ImGui: NOT FOUND. No ncurses detail found.
- **Dwarf Fortress (Steam):** coverage says the new Steam UI uses mouse-driven menus, clickable tabs and buttons, scrollbars, and scales to window size. Tarn Adams is the source per PCGamer and PCGamesN snippets (VERIFIED as press coverage: https://www.pcgamer.com/uk/the-new-dwarf-fortress-ui-looks-so-much-better, https://www.pcgamesn.com/dwarf-fortress/menus). The toolkit used is NOT FOUND. No 2026 release confirmation was found.

## 5. Could not verify

- Noesis current licence terms, SDL integration, custom-engine pricing (noesisgui.com DNS failure; forum data is dated).
- Coherent Gameface SDL integration and accessibility (search returned none).
- RmlUi gamepad focus navigation model and accessibility (fetched README silent).
- RmlUi text rendering path and font engine (not fetched).
- Clay SDL3 renderer code details beyond the search snippet (only the GitHub contents listing and a mirror commit were seen).
- Slint current accessibility status and whether AccessKit is integrated (2022 issue only).
- Qt 6 current licence page (qt.io 404 on the URL tried; forum and FAQ used).
- Ultralight SDL integration and current terms (search only).
- Dear ImGui docking behaviour on SDL_GPU (not read).
- CDDA: whether PR #73134 and #73456 are still the current ImGui state; upstream vendored path confirmed only from a snippet.
- Caves of Qud and Dwarf Fortress UI toolkits: not documented in any source reached.
- LVGL keypad/encoder focus navigation details (not read).
- AccessKit SDL adapter: none found in the listing; third-party SDL integrations not searched.

## 6. Ranking for this project (SDL3 + SDL_GPU, data-driven theme, gamepad, text scaling, a11y, permissive)

1. **RmlUi 6.3 (MIT)**: best fit for data-driven themable layout, has an SDL_GPU backend in-tree, and is retained-mode. Accessibility missing; needs AccessKit or a custom tree to meet the a11y requirement.
2. **Dear ImGui v1.93.0 (MIT)**: best integration cost, SDL3 + SDL_GPU backends, gamepad nav flag, already in CDDA's tree. Poor for player-facing themed menus and has no a11y.
3. **AccessKit (Apache-2.0/MIT, C bindings)**: not a toolkit. Only route to real screen-reader trees. Pair with RmlUi or a custom layer; large cost.
4. **Clay (Zlib)**: layout only, SDL3 renderer exists. Good if you own the widget and input layer.
5. **LVGL 9.6 (MIT)**: SDL driver exists, but no SDL_GPU driver and no a11y.
6. **Nuklear (public domain or MIT)**: small, immediate mode, you write the GPU bridge.
7. **Qt 6 (LGPL/commercial)**: best a11y tree on the list, but a second runtime and LGPL linking constraints. Not practical.
8. **Slint (triple licence)**: C++ API, possible a11y, but no SDL_GPU path and GPL/royalty-free constraints.
9. **Yoga (MIT)**: layout only, pair with any renderer.
10. **Noesis (commercial)**, **Coherent Gameface (commercial)**, **Ultralight (commercial/stale)**: paid, unverified SDL paths.
11. **microui, NanoGUI** (stale or too small); **FLTK, libui, Godot UI** (not applicable).

Overall recommendation for the stated needs: RmlUi for player-facing themed layouts with ImGui kept for developer tools, and AccessKit only if a screen-reader requirement is confirmed as a deliverable. Neither RmlUi nor ImGui provides a screen-reader tree today.

## Sources

- https://github.com/ocornut/imgui/tree/master/backends
- https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
- https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md
- https://github.com/ocornut/imgui/issues/4122
- https://github.com/ocornut/imgui/issues/7131
- https://github.com/mikke89/RmlUi
- https://github.com/mikke89/RmlUi/tree/master/Backends
- https://github.com/nicbarker/clay
- https://github.com/Immediate-Mode-UI/Nuklear
- https://github.com/rxi/microui
- https://github.com/wjakob/nanogui
- https://github.com/lvgl/lvgl
- https://github.com/AccessKit/accesskit
- https://github.com/facebook/yoga
- https://github.com/slint-ui/slint
- https://github.com/slint-ui/slint/issues/32
- https://github.com/emilk/egui/pull/2294
- https://doc.qt.io/qt-6/qaccessible.html
- https://www.qt.io/faq/3.7.-what-are-my-obligations-when-using-qt-under-the-lgpl
- https://docs.coherent-labs.com/cpp-gameface/what_is_gfp/overview/
- https://github.com/ultralight-ux/Ultralight
- https://github.com/CleverRaven/Cataclysm-DDA/pull/73134
- https://github.com/CleverRaven/Cataclysm-DDA/pull/73456
- https://goodfirstissue.org/CleverRaven/Cataclysm-DDA/issues/78465
- https://wiki.cavesofqud.com/wiki/Visual_Style
- https://unity.com/resources/systems-driven-design-in-caves-of-qud
- https://www.pcgamer.com/uk/the-new-dwarf-fortress-ui-looks-so-much-better
- https://www.pcgamesn.com/dwarf-fortress/menus
- https://caniplaythat.com/?p=15209
