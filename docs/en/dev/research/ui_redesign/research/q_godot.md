# Q: Godot 4.x as a frontend for an existing large C++ game

Research date: 2026-10-21. Tags: **VERIFIED** = confirmed from a fetched official source (docs, release page, class reference XML, source, or GitHub API). **NOT VERIFIED** = claim could not be confirmed in this session; not to be relied on.

Tool limits hit during this session: the WebSearch budget was exhausted (no further searches run, not worked around), and the GitHub REST API hit its rate limit partway through. Some items below are therefore NOT VERIFIED for reasons of access, not because they are false.

## 1. Screen reader support

- **AccessKit integration version: 4.5.** The 4.5 release page says: "Thanks to AccessKit, we added screen reader support to Control nodes" and "we also added screen reader bindings in order to customize the behavior of any type of Node." VERIFIED. https://godotengine.org/releases/4.5/
- **4.5 release date:** GitHub tag `4.5-stable` published 2025-09-15. VERIFIED. https://github.com/godotengine/godot/releases/tag/4.5-stable
- **Experimental status:** The 4.5 page says: "As this feature is quite new, please note that its integration is still in its experimental phase." VERIFIED. https://godotengine.org/releases/4.5/
- **Editor coverage:** "screen reader support for the Godot Editor itself is not complete yet ... it is only implemented for the Project Manager, standard UI nodes, and the inspector." VERIFIED. https://godotengine.org/releases/4.5/
- **Platforms (AccessibilityServer):** "[AccessibilityServer] is implemented on Android, Linux, macOS, and Windows." VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/AccessibilityServer.xml
- **Platforms (DisplayServer feature flag):** `FEATURE_ACCESSIBILITY_SCREEN_READER` is "Linux (X11/Wayland), macOS, Windows." VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/DisplayServer.xml
- **Windows UIA / macOS / Linux AT-SPI naming:** The docs name AccessKit as the library and give NVDA (Windows), VoiceOver (macOS) and Orca (Linux) as test readers. They do not name UIA or AT-SPI as such. The underlying OS API names are NOT VERIFIED from the docs. https://raw.githubusercontent.com/godotengine/godot-docs/master/tutorials/ui/creating_applications.rst
- **iOS / web / visionOS:** Not listed as supported in the sources above. A visionOS AccessKit PR is open: https://github.com/godotengine/godot/pull/124253 (state: open, verified via GitHub API). Support on iOS and web is NOT VERIFIED either way.
- **Runtime detection:** "Godot automatically enables screen reader support if a screen reader is detected as running." Setting `accessibility/general/accessibility_support` can force it on or off. VERIFIED. https://raw.githubusercontent.com/godotengine/godot-docs/master/tutorials/ui/creating_applications.rst
- **Dummy display driver:** the dummy driver disables screen reader support. VERIFIED (ProjectSettings.xml text). https://github.com/godotengine/godot/blob/master/doc/classes/ProjectSettings.xml
- **Control properties:** `accessibility_name`, `accessibility_description`, `accessibility_live`, `accessibility_labeled_by_nodes`, `accessibility_described_by_nodes`, `accessibility_controls_nodes`, `accessibility_flow_to_nodes` all exist. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/Control.xml
- **Custom Control hooks:** `_accessibility_get_contextual_info` (virtual, returns keyboard shortcut and contextual help text) and `_get_accessibility_container_name` (virtual, returns a description of a child's position in a custom container) exist in Control.xml. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/Control.xml
- **`_accessibility_update`:** NOT VERIFIED. Not found in Control.xml. The per-element API is `AccessibilityServer` (`accessibility_create_element`, `accessibility_update_set_*`, etc.). The `DisplayServer.accessibility_*` equivalents are marked deprecated in favour of `AccessibilityServer`. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/DisplayServer.xml
- **Build:** AccessKit is dynamically linked by default on Linux; static linking via `accesskit_sdk_path`. VERIFIED. https://raw.githubusercontent.com/godotengine/godot-docs/master/engine_details/development/compiling/compiling_for_linuxbsd.rst
- **Editor vs runtime:** The editor is partial (see above). The runtime is documented as covering Control nodes. Runtime behaviour per platform is NOT VERIFIED beyond these statements.
- **Open issues (GitHub API, state checked 2026-10-21):**
  - #121388 Editor crash due to AccessKit "TreeUpdate includes duplicate child" (open). https://github.com/godotengine/godot/issues/121388
  - #120999 Engine crash when typing into editor fields while a screen reader is running (open). https://github.com/godotengine/godot/issues/120999
  - #111028 Edit Sun and Environment Settings button crashes, AccessKit segfault (open). https://github.com/godotengine/godot/issues/111028
  - #123623 Accessibility position wrong with `top_level` or non-Control parents (open). https://github.com/godotengine/godot/issues/123623
  - #117252 Editor UI cluttered from screen reader POV (open). https://github.com/godotengine/godot/issues/117252
  - #117593 Context menus do not respond to application key / Shift-F10 (open). https://github.com/godotengine/godot/issues/117593
  - #110458 PR: avoid unnecessary accessibility calls when AccessKit is not supported (open). https://github.com/godotengine/godot/pull/110458
  - #124739 PR: do not use subclassing AccessKit adapters (open, created 2026-10-21). https://github.com/godotengine/godot/pull/124739

  Status and titles are VERIFIED. Root causes beyond the titles are NOT VERIFIED.

## 2. Input

- **Runtime remapping:** `InputMap` exposes `action_add_event`, `action_erase_event`, `action_erase_events`, `action_get_events`, `action_has_event`, `action_set_deadzone`, `add_action`, `erase_action`, `load_from_project_settings`, `get_actions`, `get_action_description`. The methods exist (VERIFIED). That they are the standard runtime rebinding path is inferred from the method set; a rebinding how-to was not fetched (NOT VERIFIED). https://github.com/godotengine/godot/blob/master/doc/classes/InputMap.xml
- **Gamepad database:** Godot uses SDL's game controller database. `Input.get_joy_name` "uses the SDL game controller database" and `Input.add_joy_mapping` takes "SDL format" mappings. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/Input.xml
- **Glyph API:** No glyph/button-icon API was found in `Input.xml`. A proposal to "Use glyphs for keyboard shortcuts" (#13630) is closed; its content and outcome were not read. Built-in glyph API: NOT VERIFIED. https://github.com/godotengine/godot-proposals/issues/13630
- **Hold / toggle actions:** No hold or toggle method was seen in the `InputMap` method list. A full absence check was not completed. NOT VERIFIED.
- **Steam Input / Steam Deck:** NOT VERIFIED (WebSearch budget exhausted).

## 3. Text scaling, UI scale, fonts, colour-blind

- **Window content scale:** `Window.content_scale_factor` (default 1.0), `content_scale_mode`, `content_scale_size`, `content_scale_stretch`. The docs say a window of 2560x1440 with `content_scale_size` 1280x720 scales 2D elements by 2.0. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/Window.xml
- **Hi-DPI:** Godot "currently only supports reading the screen scale factor from the OS settings on macOS, Android, and Linux" (Wayland for Linux). "On Linux (X11) and Windows, you will need to provide a manual scaling option for the user." VERIFIED. https://raw.githubusercontent.com/godotengine/godot-docs/master/tutorials/ui/creating_applications.rst
- **Theme default font size:** NOT VERIFIED (not read).
- **Fallback fonts:** `Font.fallbacks` is an array of fallback `Font` objects used when a glyph is missing. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/Font.xml
- **Oversampling:** Draw methods take an `oversampling` argument; if zero or less, "viewport oversampling settings are used". `font_get_global_oversampling` is deprecated in favour of Viewport oversampling. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/TextServer.xml
- **Oversampling changes in 4.5:** NOT VERIFIED (the 4.5 release page was read, but its oversampling text was not confirmed).
- **CJK rendering and locale fallback:** NOT VERIFIED.
- **Colour-blind support:** A proposal to "simulate common forms of color vision deficiency" in the editor (#473) is open. VERIFIED. https://github.com/godotengine/godot-proposals/issues/473. A built-in runtime colour-blind filter: NOT VERIFIED (code search rate-limited).

## 4. GDExtension / godot-cpp

- **Licence:** godot-cpp is MIT (GitHub licence API). VERIFIED. https://github.com/godotengine/godot-cpp
- **Versioning:** "Starting with version 10.x, godot-cpp is versioned independently from Godot." v10 can target Godot 4.3 or later via `api_version`. VERIFIED. https://github.com/godotengine/godot-cpp/blob/master/README.md
- **ABI / API compatibility:** "GDExtensions targeting an earlier version of Godot should work in later minor versions, but not vice-versa." Example: targeting 4.3 works on 4.4, not the reverse. VERIFIED. (same README)
- **`compatibility_minimum`:** Appears only in a sample `.gdextension` file as `"4.1"`. Its rules are NOT VERIFIED (GDExtension example page returned 404).
- **Build system:** SCons is documented (`scons api_version=4.3`, `custom_api_file=`). CMakeLists.txt exists in the godot-cpp repo root (VERIFIED via GitHub contents listing), but the README does not document CMake usage. NOT VERIFIED for CMake how-to. https://github.com/godotengine/godot-cpp
- **Linking one large C++23 codebase as one extension:** NOT VERIFIED. No source fetched on this point. Standard-library and toolchain constraints on the extension binary are NOT VERIFIED.
- **Startup cost:** NOT VERIFIED.
- **Threading rules (VERIFIED from the thread-safe APIs page):**
  - "Interacting with the active scene tree is not thread-safe."
  - To call functions or set properties from a thread, "you may use call_deferred or set_deferred."
  - "Most Global Scope singletons are thread-safe by default."
  - Rendering and physics servers: "thread-safe operation must be enabled in the project settings first."
  - "You should avoid calling functions involving direct interaction with the GPU on other threads."
  - Source: https://docs.godotengine.org/en/stable/tutorials/performance/thread_safe_apis.html
- **Blocking input loops inside the game logic:** the threading rules above say to use mutexes and `call_deferred`. A recommended pattern for a blocking loop running beside the engine was NOT VERIFIED.
- **Public large-C++ case studies:** NOT VERIFIED (WebSearch budget exhausted).
- **libgodot / embedding Godot in a C++ app:** The proposal is godotengine/godot-proposals#6267, "Add a build-time option to compile Godot as a shared library (for use in other languages)". State: open. Opened 2023-02-11. Its comments mention a similar Unreal-embedding solution by Migeran. VERIFIED. https://github.com/godotengine/godot-proposals/issues/6267. It does not, on its own, provide an embedding API. Current status of an official embedding API: NOT VERIFIED.

## 5. Platforms and distribution

- **Export templates per OS, Steam Deck Verified, binary size:** NOT VERIFIED.
- **Renderers (Vulkan / D3D12 / Metal / OpenGL):** NOT VERIFIED from fetched source.
- **Headless / dummy test runs:**
  - `--text-driver Dummy` exists as a command-line option that disables text rendering. VERIFIED. https://github.com/godotengine/godot/blob/master/doc/classes/ProjectSettings.xml
  - `--rendering-driver dummy`: NOT VERIFIED.
- **CI builds:** NOT VERIFIED.

## 6. Licence and trademark

- **Godot engine:** MIT (GitHub licence API). VERIFIED. https://github.com/godotengine/godot
- **Third-party components:** NOT VERIFIED (COPYRIGHT.txt not fetched).
- **Trademark use:** NOT VERIFIED.

## 7. Godot 3D capabilities for a voxel renderer

- **Classes exist:** `MultiMesh`, `ProceduralSkyMaterial`, `Sprite3D`, `GPUParticles3D`, `RenderingServer`. VERIFIED (class XML files exist). https://github.com/godotengine/godot/tree/master/doc/classes
- **Capabilities and performance notes for each:** NOT VERIFIED (class brief/description text was not read).
- **ArrayMesh built per frame from C++:** NOT VERIFIED.
- **Threading constraint for rendering:** rendering nodes are not thread-safe by default, and GPU calls should stay off other threads. VERIFIED. https://docs.godotengine.org/en/stable/tutorials/performance/thread_safe_apis.html

## Summary table

| # | Topic | Key result | Tag |
|---|---|---|---|
| 1 | AccessKit version | 4.5 (release 2025-09-15), experimental | VERIFIED |
| 1 | Platforms | Android, Linux, macOS, Windows | VERIFIED |
| 1 | Editor coverage | Project Manager, standard UI, inspector only | VERIFIED |
| 1 | Custom control hooks | `_accessibility_get_contextual_info`, `_get_accessibility_container_name`; `_accessibility_update` not found | VERIFIED / NOT VERIFIED |
| 1 | Open bugs | 6 listed with URLs | VERIFIED (status) |
| 2 | InputMap runtime methods | exist | VERIFIED |
| 2 | Gamepad DB | SDL mappings | VERIFIED |
| 2 | Glyph API, hold/toggle, Steam Input | not found / not checked | NOT VERIFIED |
| 3 | Window content scale | properties exist | VERIFIED |
| 3 | Hi-DPI | OS scale only on macOS, Android, Linux(Wayland) | VERIFIED |
| 3 | Oversampling | per-draw and viewport settings | VERIFIED |
| 3 | Colour-blind built-in | proposal #473 open, no built-in found | NOT VERIFIED |
| 4 | godot-cpp licence | MIT | VERIFIED |
| 4 | Compatibility | earlier target works on later minor | VERIFIED |
| 4 | Threading | scene tree not thread-safe; use call_deferred | VERIFIED |
| 4 | libgodot | proposal #6267 open | VERIFIED |
| 4 | Large C++ via GDExtension, startup cost | not checked | NOT VERIFIED |
| 5 | Templates, Steam Deck, renderers, dummy renderer | not checked | NOT VERIFIED |
| 6 | Engine licence | MIT | VERIFIED |
| 7 | 3D classes exist | exist; features not read | VERIFIED / NOT VERIFIED |

## Could not verify

- Whether `_accessibility_update` exists (not in Control.xml).
- Per-platform runtime behaviour of AccessKit (UIA, AT-SPI), and whether iOS or web are supported.
- Release notes on whether 4.5 changed oversampling, and CJK / locale fallback behaviour.
- Glyph API, hold/toggle actions, Steam Input and Steam Deck verification.
- Built-in colour-blind filter.
- godot-cpp `compatibility_minimum` rules, CMake usage, startup cost, linking a large C++23 codebase.
- Public case studies of large C++ codebases embedded via GDExtension.
- Libgodot embedding API status beyond proposal #6267.
- Export template sizes, renderer list, `--rendering-driver dummy`, CI guidance.
- Third-party licences and trademark rules.
- Performance of ArrayMesh per frame, and the capabilities of MultiMesh, ProceduralSkyMaterial, Sprite3D and GPUParticles3D.
- Blocking input loops inside game logic.
