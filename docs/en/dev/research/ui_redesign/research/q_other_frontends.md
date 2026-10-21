# Other frontends and accessibility routes for a large C++ game (research)

Date of research: 2026-10-21. Scope: alternative engines, web/WASM, thin-client splits, and SDL3 + RmlUi + AccessKit.

Labels: **VERIFIED** = checked in a primary source this session (URL given). **NOT VERIFIED** = not checked, or a search snippet only, or a source could not be read. **NOT FOUND** = searched, nothing relevant located.

Tool limits hit during research: the GitHub API rate limit (5,000/hr shared) was exhausted partway through, and the WebSearch budget (200 per session) ran out before the last searches. Items that depended on those calls are marked NOT VERIFIED.

---

## Summary table

| Option | Licence (verified?) | C++ path | Screen-reader / accessibility | Linux / macOS | Health (last push / release) | Verdict for this project |
|---|---|---|---|---|---|---|
| O3DE | Apache-2.0 and MIT licence files in repo; GitHub reports NOASSERTION (VERIFIED) | Native C++ engine, Gems (VERIFIED: `Gems/` dir exists) | No screen-reader code found by code search; not exhaustive (NOT FULLY VERIFIED) | NOT VERIFIED | Pushed 2026-10-21; latest release 2605.0 published 2026-05-27 (VERIFIED) | Engine switch: very high cost. Not a fit for a UI-only problem. |
| Flax Engine | Custom Flax EULA; 4% royalty above $250,000 per calendar quarter per game (VERIFIED, flaxengine.com/faq) | C++ and C# (NOT VERIFIED beyond search snippet) | None stated on FAQ (VERIFIED: page has no a11y statements) | Linux and macOS listed (VERIFIED, FAQ) | Latest release 1.13.7012, 2026-10-21 (VERIFIED) | Custom licence plus engine switch. Not recommended. |
| Stride | MIT (VERIFIED, LICENSE.md) | C# only (VERIFIED, README describes C# engine) | No UI screen-reader support found; the search hit was in a scripting file (NOT VERIFIED what it is) | Runtime builds on Linux Vulkan/OpenGL (VERIFIED, README badge); editor on Linux NOT VERIFIED | Pushed 2026-10-21; latest release 4.4.1, 2026-10-17 (VERIFIED) | Wrong language for a C++ codebase. Not recommended. |
| CDDA Emscripten/WASM | n/a | Emscripten build, abandoned | n/a | n/a | Build script self-declares UNSUPPORTED (VERIFIED) | Evidence that the WASM route is hard to keep alive. |
| Browser DOM/ARIA UI | n/a | Emscripten + JS/DOM UI | ARIA exposes semantics to AT (VERIFIED, W3C ARIA 1.2) | n/a | n/a | Plausible for a menu/UI layer; the 3D view is the weak point (see section 4). |
| Tauri (Rust) + WebView | MIT/Apache (NOT VERIFIED) | Rust shell + C++ core (NOT VERIFIED) | Per-OS a11y in webview: NOT VERIFIED | n/a | n/a | Adds a second toolchain. Accessibility per engine not verified. |
| Thin client (DFHack / NetHack / Brogue) | n/a | n/a | n/a | n/a | n/a | Pattern exists; specific examples NOT VERIFIED (fetches failed). |
| AccessKit C bindings + SDL example | Apache-2.0 OR MIT (README VERIFIED); GitHub licence field reports BSD-3-Clause (VERIFIED, conflict noted) | C API via cbindgen (search snippet only, NOT VERIFIED in repo) | Windows, macOS, Unix (AT-SPI) adapters (VERIFIED, accesskit.dev) | Unix/AT-SPI adapter exists; known caveats (VERIFIED) | Adapter releases 2026-09-25; core 0.23.1 on 2026-09-26 (VERIFIED) | The most relevant option. An `sdl` example folder exists in accesskit-c (VERIFIED). Quality NOT VERIFIED. |

---

## 1. O3DE (Apache-2.0)

- **Licence.** The repo contains `LICENSE_APACHE2.TXT` and `LICENSE_MIT.TXT`. GitHub's licence field reads NOASSERTION (a dual-licence layout). Apache-2.0 text confirmed in `LICENSE_APACHE2.TXT`. **VERIFIED** via `gh api repos/o3de/o3de/contents/LICENSE_APACHE2.TXT`.
- **Health.** Last push 2026-10-21 (VERIFIED). Latest release `2605.0`, published 2026-05-27 (VERIFIED). ~9,768 stars (VERIFIED).
- **C++ integration.** O3DE is C++ with a Gems plugin system. `Gems/` exists with about 83 entries (VERIFIED). Build burden and CI cost: **NOT VERIFIED**.
- **Accessibility.**
  - A code search for "narrator" plus "accessibility screen reader" returned 0 results. GitHub code search is incomplete, so this is **NOT FULLY VERIFIED** as an absence.
  - Titles of three accessibility-labelled issues (2021 to 2022) concern cursor settings and editor/sub-object accessibility, not screen readers (VERIFIED from issue titles).
  - Web search returned nothing on O3DE screen-reader support (**NOT FOUND** by search; search coverage weak).
- **Platform support (Linux/macOS/Windows).** **NOT VERIFIED** in this session.
- **Assessment.** Adopting O3DE would replace the engine. It offers nothing on accessibility, so it doesn't address the problem. Not recommended.

## 2. Flax Engine

- **Licence and revenue share.**
  - The FAQ page states: "When you're making a commercial product you have to pay a 4% royalty on earnings exceeding $250,000 per calendar quarter." Also: royalty "due 45 days after the close of each calendar quarter." **VERIFIED** (https://flaxengine.com/faq/, fetched).
  - Non-commercial and educational use is free (VERIFIED, FAQ).
  - A search snippet quoted the 1.0 release notes as raising the threshold from $3,000 to $25,000 per quarter. The current FAQ says $250,000. **Conflict**: the FAQ is the more current source, but the release notes were not fetched (NOT VERIFIED).
  - Source licence: the search snippet says Flax Editor and source are under the Flax EULA. **NOT VERIFIED** beyond the snippet. GitHub's licence field reads NOASSERTION (VERIFIED, `gh api repos/FlaxEngine/FlaxEngine`).
- **C++ scripting.** The search snippets say C++ and C# are both supported, with a visual scripting system (NOT VERIFIED from a primary doc).
- **Platforms.** FAQ: "Flax currently supports Windows, Linux, Android, macOS, iOS, PlayStation 4, PlayStation 5, Xbox One" (VERIFIED). The same page lists Xbox Series X/S, Web and Switch, with a formatting error (VERIFIED as listed).
- **Accessibility.** The FAQ contains no accessibility statements (VERIFIED). No other source checked: **NOT VERIFIED**.
- **Health.** Latest release `1.13.7012`, published 2026-10-21 (VERIFIED, `gh api`). Active.
- **Assessment.** Custom licence with a revenue share, and an engine switch. Not recommended for a UI accessibility problem.

## 3. Stride (MIT, C#)

- **Licence.** MIT, "Copyright (c) 2021 .NET Foundation and Contributors" (VERIFIED, `LICENSE.md`).
- **Health.** Pushed 2026-10-21; latest release `releases/4.4.1`, 2026-10-17 (VERIFIED, `gh api`).
- **Language.** C# engine, with the Game Studio editor (VERIFIED, README).
- **Linux.** The Linux runtime workflow exists (`build-linux-runtime.yml`), and the README shows a "Linux Vulkan/OpenGL" build badge (VERIFIED). Linux editor support: **NOT VERIFIED**. The workflow runs on `windows-2025-vs2026` (VERIFIED), which is a separate concern.
- **Accessibility.** One code-search hit in `sources/engine/Stride.Assets/Scripts/Accessibility.cs`. I did not open it, so its meaning is **NOT VERIFIED** (it may be C# member access modifiers, not UI accessibility). The rate-limited search for "narrator/UIA/AccessKit" returned no result (NOT VERIFIED).
- **Assessment.** MIT is attractive, but C# doesn't fit a C++ codebase. Not recommended.

## 4. Web frontend (Emscripten/WASM, DOM/ARIA, Tauri/Electron, Steam Deck)

### 4a. CDDA WASM build
- **CDDA repo `build-scripts/build-emscripten.sh`** opens with: "UNSUPPORTED: this build cannot currently succeed. The target was abandoned in 2025. The PR CI leg was commented out on 2025-07-14 in commit f530407bda (PR #81827)... SDL2 support has since been removed... Emscripten does ship an SDL3 port (3.4.2)... but no SDL3_image or SDL3_mixer port." (VERIFIED via `gh api repos/CleverRaven/Cataclysm-DDA/contents/build-scripts/build-emscripten.sh`.)
- **CDDA release workflow.** The "WebAssembly Bundle" matrix entry is commented out (`release.yml`, lines 191-198) (VERIFIED). A separate build step remains (line 485-489) for `matrix.wasm` (VERIFIED, but not run).
- **Open issue** #76956 "Emscripten Build link broken", created 2024-10-11, last updated 2025-07-20, still open (VERIFIED).
- **nornagon.** The search returned nothing about nornagon and CDDA wasm. Claim **NOT VERIFIED** and treated as unconfirmed; the user's "cdda wasm" recollection could not be confirmed. The play.cataclysmdda URL was not checked (NOT VERIFIED).
- **Cataclysm-BN.** A local grep of the BN repo for "emscripten" in `*.sh`, `*.txt`, `*.cmake`, `*.yml` found no matches (VERIFIED, local).

### 4b. Browser limits (threads, memory)
From the Emscripten pthreads doc (https://emscripten.org/docs/porting/pthreads.html, fetched):
- Multithreading uses SharedArrayBuffer, which browsers gate behind COOP and COEP headers (VERIFIED).
- "Pthreads code will not work in deployed environment unless these headers are correctly set" (VERIFIED).
- The doc states pthreads builds "currently only work in the Firefox Nightly channel", describing SharedArrayBuffer as "still in an experimental research stage" (VERIFIED as written in the doc). **Currency of that claim NOT VERIFIED**: the doc date is unknown and this may be out of date for 2026.
- Pthreads with memory growth "currently causes JS accessing the Wasm memory to be slow" (VERIFIED as written; currency NOT VERIFIED).
- Firefox Nightly caps threads at 20 per domain by default (VERIFIED as written; currency NOT VERIFIED).

### 4c. ARIA and accessibility of canvas
- W3C WAI-ARIA 1.2 says ARIA "provides a framework to improve the accessibility and interoperability of web content" (VERIFIED, https://www.w3.org/TR/wai-aria-1.2/).
- "When WAI-ARIA roles override host language semantics, there are no changes in the DOM, only in the accessibility tree" (VERIFIED, Section 4.1).
- **Correction to my assumption.** ARIA does not only change the accessibility tree. Section 4.3.2 requires user agents to handle focus and keyboard events for `aria-activedescendant` (VERIFIED). So ARIA can affect keyboard behavior, not only what is exposed.
- MDN `HTMLCanvasElement` fetch returned no accessibility text. A canvas-is-opaque-to-AT claim is **NOT VERIFIED** from that page.
- WebGL/WebGPU for 3D: **NOT VERIFIED** (not researched this session).

### 4d. Tauri and Electron wrappers
- Tauri webview engines (search snippet, not fetched): WebView2 on Windows, WKWebView on macOS, webkit2gtk on Linux (VERIFIED as snippet only).
- Accessibility of those webviews per OS: **NOT VERIFIED**. The search returned only the engine list.
- Electron, Steam integration, Steam Deck: **NOT VERIFIED** (not researched).

## 5. Hybrid thin-client architectures

- **DFHack remote.** Fetches of the DFHack Lua API docs failed (404 on the original URL, then a 404 on the redirect target). Whether DFHack exposes a remote protocol: **NOT VERIFIED**.
- **NetHack window port.** Fetch of the NetHack wiki page failed (404). Claims about the core/window-port split: **NOT VERIFIED**.
- **Brogue platform layer.** Web search was blocked by the session budget. **NOT VERIFIED**.
- **Factorio, latency numbers, Lua implications.** **NOT RESEARCHED / NOT VERIFIED**.
- **Relevance to this codebase.** Cataclysm-BN already separates `cata_tiles` from the core via SDL. I did not inspect that separation in this session; claims about it are **NOT VERIFIED**.

## 6. SDL3 + RmlUi + AccessKit

### 6a. AccessKit repo and licence
- The AccessKit README says: "AccessKit is licensed under the Apache License, Version 2.0 or the MIT license, at your option." It also says "Significant portions of AccessKit are derived from Chromium" (VERIFIED, `README.md` via WebFetch of GitHub). **Licence conflict**: GitHub's licence field for `AccessKit/accesskit` and `AccessKit/accesskit-c` reads BSD-3-Clause (VERIFIED, `gh api`). The README covers the derived Chromium portions as BSD-style. The two readings need resolving before shipping; for now: Apache-2.0 OR MIT for AccessKit's own code (VERIFIED from README), with Chromium-derived BSD-3 portions.
- Meson build files are LGPL-2.1 (VERIFIED, README).

### 6b. accesskit-c examples
- `examples/` in `accesskit-c` contains two directories: `sdl` and `windows` (VERIFIED, `gh api repos/AccessKit/accesskit-c/contents/examples`). So an SDL example exists. Whether it compiles and what it covers: **NOT VERIFIED** (not built or read).
- The accesskit-c README says the C bindings have "header file, pre-built libraries, examples, and source code" (VERIFIED, accesskit.dev).
- The README's cross-compiling notes mention Windows, Linux and ARM, and require Rust 1.87 or higher and CMake 3.20 or higher (VERIFIED, README).
- A FreeBSD port listed accesskit-c 0.17.1 (search snippet). That is an old version; current version NOT VERIFIED.

### 6c. Platform adapter completeness
- accesskit.dev lists three adapters: "macOS adapter," "Unix adapter," and "Windows adapter" (VERIFIED). Android, iOS and other platforms: NOT FOUND on that page. There is an `accesskit_ios` release (`v0.2.1`, 2026-09-25) (VERIFIED, `gh api`).
- `ARCHITECTURE.md` says: "Adapters are best-effort implementations and may not cover all the accessibility APIs of their platform." and "Accesskit is currently sparsely documented." (VERIFIED, WebFetch of GitHub `ARCHITECTURE.md`).
- `README-APPLICATION-DEVELOPERS.md` calls itself "a work in progress and will be expanded over time" (VERIFIED).
- Linux AT-SPI caveat: accerciser does not display AccessKit trees unless the ATSPI bus is enabled (running a screen reader or `busctl` enables it) (VERIFIED, `README-APPLICATION-DEVELOPERS.md`).
- Open-issues "not supported" lists: **NOT VERIFIED** (the `gh api` budget ran out before issues were checked).

### 6d. Release cadence and funding
- Core crate releases: 0.22.2 (2026-06-15), 0.22.3 (2026-07-14), 0.23.0 (2026-09-07), 0.23.1 (2026-09-26) (VERIFIED, `gh api repos/AccessKit/accesskit/releases`).
- Per-adapter releases on 2026-09-25: winit 0.34.1, windows 0.35.1, unix 0.24.0, macos 0.27.1, ios 0.2.1 (VERIFIED).
- Funding: accesskit.dev links to a "Sponsor the project" page (GitHub Sponsors). Who funds the project and in what amounts: **NOT VERIFIED**. The org description is "Accessibility infrastructure for UI toolkits", with 7 public repos (VERIFIED).

### 6e. RmlUi + AccessKit
- No integration found. Search for RmlUi with AccessKit returned none (**NOT FOUND**, search-based).
- RmlUi's own accessibility support: **NOT VERIFIED** (search returned only generic ARIA pages).
- Searching for "any project wired to AccessKit C" returned no concrete downstream project (**NOT FOUND**).

### 6f. This repo
- A local grep for `accesskit|tolk|nvda|screen.?reader|narrator` in `src`, `CMakeLists.txt` and `cmake` returned matches in `src/string_input_popup.cpp`, `src/status_report.h`, `src/ui_manager.h`, `src/game.cpp`, `src/messages.cpp` and `src/overmap/overmap_ui.cpp`. I did not open these, so their content is **NOT VERIFIED**. No AccessKit dependency was found in the build (VERIFIED by grep of `CMakeLists.txt` and `cmake/`).

---

## Could not verify

- O3DE: Linux/macOS/Windows support, build and CI burden, accessibility beyond code search, project health details beyond push date and release.
- Flax: source licence terms beyond the EULA snippet, release-notes threshold conflict ($25,000 vs $250,000), accessibility, C++ scripting docs.
- Stride: Linux editor, meaning of the `Accessibility.cs` hit, any UI accessibility.
- CDDA: nornagon's involvement, the play.cataclysmdda site, the current status of the Emscripten pthreads claims, and the current state of the SDL3 port in Emscripten.
- Browser: MDN canvas accessibility, WebGL/WebGPU, performance numbers, Electron, Steam and Steam Deck.
- Tauri: accessibility of WebView2, WKWebView and webkit2gtk per OS.
- Thin client: DFHack remote protocol, NetHack window port details, Brogue platform layer, Factorio, latency numbers.
- AccessKit: whether the SDL example builds, adapter "not supported" lists from open issues, full funding picture, Android support, and the licence conflict (Apache/MIT vs BSD-3 in GitHub's field).
- RmlUi: any AccessKit or accessibility integration.
- Local: content of the grep matches in `src/` and whether the codebase has any existing a11y hooks.

---

## Sources

- O3DE: https://github.com/o3de/o3de (licence files, release 2605.0)
- Flax FAQ: https://flaxengine.com/faq/
- Flax 1.0 notes (search snippet, not fetched): https://docs.flaxengine.com/manual/release-notes/1_0/index.html
- Stride licence: https://github.com/stride3d/stride/blob/master/LICENSE.md
- CDDA Emscripten script: https://github.com/CleverRaven/Cataclysm-DDA/blob/master/build-scripts/build-emscripten.sh
- CDDA issue #76956: https://github.com/CleverRaven/Cataclysm-DDA/issues/76956
- Emscripten pthreads: https://emscripten.org/docs/porting/pthreads.html
- W3C WAI-ARIA 1.2: https://www.w3.org/TR/wai-aria-1.2/
- AccessKit README: https://github.com/AccessKit/accesskit/blob/main/README.md
- AccessKit ARCHITECTURE: https://github.com/AccessKit/accesskit/blob/main/ARCHITECTURE.md
- AccessKit developer notes: https://github.com/AccessKit/accesskit/blob/main/README-APPLICATION-DEVELOPERS.md
- accesskit-c README: https://github.com/AccessKit/accesskit-c
- accesskit.dev: https://accesskit.dev
- Tauri webview versions (search snippet): https://v2.tauri.app/reference/webview-versions/
