# Unreal Engine 5.x as a frontend for a large C++ game

Status: partial. The WebSearch budget ran out after 16 searches (limit 200 per session, shared across agents), and several Epic pages returned HTTP 403/503 (the EULA, licence and FAQ pages). Sections 4 and 5 are thin. Every claim below is tagged VERIFIED or NOT VERIFIED.

## Scope caveat (read first)

- The official Epic pages that could be fetched carry the header "Unreal Engine 5.8 Documentation". The user asked about 5.4 to 5.7. Where a page is labelled 5.8, that is the only version it was checked against. Behaviour in 5.4 to 5.7 is NOT VERIFIED unless stated.
- Release notes for 5.5 to 5.7 were not retrieved, so version-to-version changes are NOT VERIFIED.

## 1. Accessibility

### 1a. Slate / UMG screen reader

| Claim | Status | Source |
|---|---|---|
| The screen reader plugin is marked **Experimental**: "Learn to use this Experimental feature, but use caution when shipping with it." | VERIFIED (5.8 page) | https://dev.epicgames.com/documentation/unreal-engine/supporting-screen-readers-in-unreal-engine |
| Third-party screen readers are supported on **Windows** and **VoiceOver on iOS**. The page names no other platform. | VERIFIED (5.8 page) | same |
| macOS, Linux, Android, consoles: support | NOT VERIFIED | not stated on any fetched page |
| Windows support goes through **UI Automation (UIA)** | NOT VERIFIED. The phrase "Windows UI Automation" appears only in a third-party blog (StraySpark), which cites no official source | https://www.strayspark.studio/blog/game-accessibility-2026-unreal-engine |
| Enabled by config `Accessibility.Enable=1` in the project or engine config | VERIFIED (5.8 page) | same as above |
| Built-in UMG widgets: Text Block, Editable Text Box, Slider, Button, Checkbox | VERIFIED (5.8 page) | same as above |
| Slate Screen Reader plugin: "plug-and-play", vocalises Slate and UMG elements on focus; supports buttons, sliders, text fields, lists, windows; enabling it also enables TTS | VERIFIED (5.8 Epic page "blind accessibility features overview") | https://dev.epicgames.com/documentation/unreal-engine/blind-accessibility-features-overview-in-unreal-engine |
| Text To Speech plugin supports **English only** "across all platforms" | VERIFIED (same page) | same |
| Screen Reader subsystem: a registered user is inactive by default, and `ActivateUser()` is needed | VERIFIED from an Epic API index excerpt returned by search (page not fetched) | https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/SlateScreenReader/USlateScreenReaderEngineSubsyste-/ActivateUser |
| Custom C++ widgets: override `SWidget::CreateAccessibleWidget()` and return a new `FSlateAccessibleWidget`. Example: `FSlateAccessibleButton` in `SlateAccessibleWidgets.h` | VERIFIED (5.8 page). Note the user's name `CreateAccessibleWidget` is the method on `SWidget` | supporting-screen-readers page |
| Compile-time removal: `bCompileWithAccessibilitySupport = false` | VERIFIED (5.8 page) | same |
| UMG per-widget settings: Accessible Behavior (Auto, Summary, Custom, Tool Tip, Not Accessible) and "Can Children Be Accessible" | VERIFIED (5.8 page) | same |
| Known limitations beyond "Experimental" | NOT VERIFIED. The 5.8 page lists none | same |
| Changes in 5.5, 5.6 or 5.7 | NOT VERIFIED. Release notes were not retrieved | none |
| Screen reader reads TTS "in a packaged game" | NOT VERIFIED | none |

### 1b. Colour vision deficiency

| Claim | Status | Source |
|---|---|---|
| Enum `EColorVisionDeficiency`: `NormalVision`, `Deuteranope`, `Protanope`, `Tritanope` | VERIFIED (5.8 API page) | https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/EColorVisionDeficiency |
| Enum page says whether it is editor-only or ships in packaged games | NOT VERIFIED. The page does not say | same |
| `UWidgetBlueprintLibrary::SetColorVisionDeficiencyType` applies correction to the game window; the page gives a severity of 0 to 1 | NOT VERIFIED for current versions. Seen only in a 4.26 docs excerpt returned by search. A forum post gives 0 to 10 instead | https://docs.unrealengine.com/4.26/BlueprintAPI/Widget/Accessibility/SetColorVisionDeficiencyType/ |
| Editor colour-vision preview exists | NOT VERIFIED beyond a forum thread on bug UE-49566 (fixed in 4.18) | https://issues.unrealengine.com/issue/UE-49566 |
| `r.ColorBlindness.Type` CVar; editor menu "Window > Developer Tools > Color Blindness Simulation"; described as a development tool and not player-facing | NOT VERIFIED. Third-party blog only | strayspark blog |
| Built-in player-facing colour correction is shipped | NOT VERIFIED. The 4.26 excerpt suggests a correction function exists, but shipping status is unknown | none |

### 1c. Text scaling and DPI

| Claim | Status |
|---|---|
| Built-in player-facing text scaling or DPI curve for UMG | NOT VERIFIED. The third-party blog says you build user-configurable scaling yourself, and cites no official source |
| Rich Text support with customisable font sizes | NOT VERIFIED (blog) |

### 1d. Input: CommonUI and Enhanced Input

Source for this section: https://dev.epicgames.com/documentation/unreal-engine/common-ui-plugin-for-advanced-user-interfaces-in-unreal-engine (5.8 page)

| Claim | Status |
|---|---|
| Input routing gives widgets selective interactivity and focus management | VERIFIED |
| Controller-specific button icons are supported | VERIFIED |
| Cardinal navigation management for gamepads | VERIFIED |
| Action bar / Common Input action icons and automatic gamepad vs keyboard glyph switching | NOT VERIFIED. Not on the fetched page. A separate Epic page on "Common Input and keys" returned an empty table of contents |
| Enhanced Input rebinding is built on the framework, and remapping UI is hand-built | NOT VERIFIED. Only a third-party blog says it, and that blog cites no source |

## 2. Embedding a C++ codebase

| Claim | Status | Source |
|---|---|---|
| Engine compiles with C++20 by default and requires C++20 to build | VERIFIED (5.8 page) | https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine |
| C++23 usable in engine modules | NOT VERIFIED. The page does not say | same |
| The coding standard avoids STL: prefer engine containers; standard containers and strings "should be avoided except in interop code" | VERIFIED | same |
| `<atomic>` "should be used in new code"; `<regex>` only in editor-only code | VERIFIED | same |
| `TArray`, `TMap`, `TSet`, `FString` have move semantics; `MoveTemp` is UE's `std::move` | VERIFIED | same |
| UE's own allocator and FMalloc details | NOT VERIFIED. The page does not cover them | same |
| Exceptions disabled by default; RTTI disabled by default; how to enable (`bForceEnableExceptions`, `bUseRTTI` on the target rules) | NOT VERIFIED. The search returned nothing on UBT defaults and the search budget was exhausted before this could be checked | none |
| Precompiled headers, UBT module layout for a 300+ kLOC external codebase | NOT VERIFIED | none |
| Threading and blocking loops with the game thread | NOT VERIFIED | none |
| Case studies of large non-UE C++ codebases embedded in UE | NOT VERIFIED. Not searched | none |

Key constraint, derived from the verified rules and not from external evidence: the codebase uses C++23 and the engine demands C++20. Whether the engine can compile a C++23 module is NOT VERIFIED and must be tested.

## 3. Licensing

| Claim | Status | Source |
|---|---|---|
| Royalty of 5% on lifetime gross revenue above the first $1 million, for shipped products that incorporate engine code | NOT VERIFIED against primary EULA text. The EULA, licence and FAQ pages returned HTTP 403 or 503. The figure appears in search excerpts only | https://www.unrealengine.com/en-US/license; https://gateway.unrealengine.com/en-US/faq (both failed) |
| Epic Games Store sales are royalty-free | NOT VERIFIED. Search excerpt only | Twinmotion FAQ (403) |
| Rate may fall to 3.5% if the game ships on the Epic Games Store at or before other stores | NOT VERIFIED. Search excerpt only; several Epic pages still show 5% | https://map.twinmotion.com/en-US/faq (403) |
| Quarterly reporting: payments due 45 days after the quarter; per-product royalty reports | NOT VERIFIED | search excerpt only |
| Non-game commercial use: seat fee $1,850 per seat per year | NOT VERIFIED | search excerpt only |
| EULA forbids redistributing engine source code | NOT VERIFIED from current EULA. A UE4-era Gentoo wiki draft says each user must accept the EULA and redistribution is not allowed. The page is marked outdated | https://wiki.gentoo.org/wiki/User:Maffblaster/Drafts/Unreal_Engine |
| Engine source access requires an Epic account with GitHub linked and EULA acceptance | NOT VERIFIED for current versions. A UE 4.27 docs excerpt says the repository requires subscriber status | https://docs.unrealengine.com/4.27/ProgrammingAndScripting/DownloadingSourceCode |
| Open-source CC-BY-SA game whose players must compile the engine: permitted | NOT VERIFIED. No source found. Treat as a legal question for the licensing team | none |
| Logo/splash: the startup logo is not mandatory; the preferred route is an in-game credits notice "[Product] uses the Unreal® Engine" plus a trademark line | NOT VERIFIED. UE4-era Epic staff replies on community forums, not the EULA | https://answers.unrealengine.com/questions/453664/view.html; https://forums.unrealengine.com/t/permission-regarding-ue4-intro-logo/438816 |
| Unreal splash screen may not be modified | NOT VERIFIED. Forum post only | same forum |
| Whether "Made with Unreal Engine" needs a trademark licence | NOT VERIFIED | none |

Licensing risks for an open-source CC-BY-SA game (analysis, not verified fact): CC-BY-SA covers the game's own work. It does not change what the EULA allows for engine code. Anything that bundles engine binaries or source must be checked against the EULA in full. The EULA text is required before any decision.

## 4. Platforms and CI

| Claim | Status | Source |
|---|---|---|
| Editor minimum on Linux: Rocky Linux 8 or RHEL 8 or newer; kernel 4.18 or newer; glibc 2.28 or newer (2.35 recommended for slow starts) | VERIFIED (5.8 page) | https://dev.epicgames.com/documentation/unreal-engine/linux-development-requirements-for-unreal-engine |
| Recommended Linux editor: Ubuntu 22.04; quad-core 2.5 GHz+; 32 GB RAM; GeForce 2080 class; 8 GB VRAM or more | VERIFIED (5.8 page) | same |
| Vulkan RHI: AMD RADV 24.2.8+ (25.0.0+ recommended), NVIDIA 570+ | VERIFIED (5.8 page) | same |
| Nanite and Virtual Shadow Maps on Linux require `VK_KHR_shader_atomic_int64` | VERIFIED (5.8 page) | same |
| Development on Ubuntu 22.04 or Rocky 8 with clang 20.1.8 | VERIFIED (5.8 page) | same |
| Disk space is not listed as a requirement | VERIFIED (5.8 page; the page gives no disk minimum) | same |
| Linux or Windows **game** build targets (cook and package for each platform) | NOT VERIFIED. Not fetched | none |
| macOS editor support | NOT VERIFIED | none |
| Steam Deck Verified status for UE games; Steam Deck performance | NOT VERIFIED from Valve. One SteamDeckHQ weekly roundup lists a UE5 game as Playable, not Verified. A Spanish blog reports 30 to 40 FPS. Neither is authoritative | https://steamdeckhq.com/steam-deck-verified-highlights-8-3-26-8-9-26/ ; https://www.noticias3d.com/noticia/93778/steam-deck-unreal-engine.html |
| Engine source disk use: about 190 GiB installed, 340+ GiB to build (UE 5.4) | NOT VERIFIED. Arch Linux wiki, third-party | https://wiki.archlinux.org/title/Unreal_Engine_5 |
| Forum guidance: plan for 150 to 200 GB free for a 5.8.1 install | NOT VERIFIED. Forum | https://forums.unrealengine.com/t/storage-required-to-install-unreal-engine-5-8-1/2740177 |
| Build time: at least an hour, possibly many hours (macOS / Xcode) | NOT VERIFIED. Single forum comment; no Windows or Linux figure | https://forums.unrealengine.com/t/required-disk-space-to-install-ue5-from-source/2742506 |
| Linux source build is `Setup.sh`, `GenerateProjectFiles.sh`, then `Build.sh` | NOT VERIFIED. Arch wiki; `make -jN` cautioned against | Arch wiki |
| Source repository access is gated by Epic account plus GitHub link | NOT VERIFIED for current versions (see section 3) | none |
| GitHub-hosted runner feasibility for a UE build (disk, time, licence) | NOT VERIFIED. Not searched | none |
| Packaged game sizes | NOT VERIFIED. Not searched | none |
| Nanite and Lumen can be turned off for low-end targets | NOT VERIFIED. Not fetched; the Nanite page did not cover it | none |

## 5. Rendering

| Claim | Status | Source |
|---|---|---|
| Nanite supports Opaque and Masked materials; Translucent materials are not supported | VERIFIED (5.8 page) | https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine |
| Nanite does not support Forward Rendering, VR stereo, MSAA, Lighting Channels, or distance culling | VERIFIED (5.8 page) | same |
| Nanite instances are hard-capped at 16 million streamed in the scene | VERIFIED (5.8 page) | same |
| Nanite needs DX12 with SM6 on current desktop and console; SSDs recommended | VERIFIED (5.8 page) | same |
| Nanite skeletal meshes are supported; morph targets are not; WPO is limited | VERIFIED (5.8 page) | same |
| Nanite supports runtime-generated or procedural meshes | NOT VERIFIED. The 5.8 page does not address procedural or runtime-generated meshes | same |
| Realtime Mesh Component (RMC): runtime, procedural, voxel-capable; faster and lighter than ProceduralMeshComponent; supports UE 5.5 to 5.8 | NOT VERIFIED. Vendor listing on Fab only. The GitHub page returned 404 | https://www.fab.com/listings/bb2e4fbb-617c-41d3-aac6-e181eddf8b3b |
| RMC runtime Nanite generation works on unmodified engines | NOT VERIFIED. Vendor claim | same |
| RMC Core is free and open source; Pro adds Nanite, SDF and Lumen cards | NOT VERIFIED. Vendor claim | same |
| ProceduralMeshComponent limits | NOT VERIFIED. Not fetched | none |
| Sprite billboards, sky and weather built-ins | NOT VERIFIED. Not searched | none |

## Summary table

| Area | Verified | Open |
|---|---|---|
| Screen reader | Experimental; Windows and iOS VoiceOver; widget list; `CreateAccessibleWidget` override; `bCompileWithAccessibilitySupport` | macOS, Linux, UIA details, 5.4 to 5.7 changes |
| Colour vision | Enum values | Shipping status, CVar, severity range |
| Text scaling | Nothing built-in verified | DPI curve |
| CommonUI | Input routing, controller icons, gamepad navigation | Action bar, glyph switching, Enhanced Input remap |
| C++ | C++20 default and required; STL policy; containers | C++23 use, exceptions, RTTI, UBT defaults |
| Licensing | Nothing primary | 5% rate, source redistribution, logo wording, CC-BY-SA question |
| Linux | Editor minimums and recommended hardware; Vulkan extension | Game build targets, macOS, Steam Deck |
| Source build | Nothing official | Disk size, build time, GitHub runner feasibility |
| Rendering | Nanite material, mesh, and count limits | Procedural and runtime Nanite, RMC claims, billboards, sky |

## Could not verify

1. EULA text: royalty rate, $1M threshold, Epic Games Store terms, seat fee, source redistribution, splash and logo clauses. HTTP 403/503 on all EULA, licence and FAQ pages.
2. Engine versions 5.4 to 5.7: every fetched official page is labelled 5.8.
3. Release notes for 5.5, 5.6 and 5.7 (screen reader, colour vision, C++ changes).
4. macOS and Linux screen reader support; Windows UIA integration.
5. Colour vision deficiency: shipping vs editor-only; the CVar; severity range.
6. Text scaling and DPI curve in UMG.
7. CommonUI action bar and glyph switching; Enhanced Input remapping.
8. UBT defaults for exceptions, RTTI, and C++23 compilation.
9. Engine source access and redistribution terms in the current EULA.
10. Splash and logo requirements in the current EULA.
11. Open-source CC-BY-SA games that require players to build the engine.
12. Linux and Windows game build targets, macOS editor, Steam Deck Verified status.
13. Source-build disk and time figures for 5.4 to 5.8 (only third-party numbers found).
14. GitHub-hosted runner feasibility, packaged game sizes, Lumen and Nanite off switches.
15. RealtimeMesh plugin claims and limitations; ProceduralMeshComponent; Nanite and runtime meshes.
16. Sky, weather, and sprite billboard built-ins.
17. Engine case studies for embedding large external C++ codebases.

## Sources used

- https://dev.epicgames.com/documentation/unreal-engine/supporting-screen-readers-in-unreal-engine (5.8, fetched)
- https://dev.epicgames.com/documentation/unreal-engine/blind-accessibility-features-overview-in-unreal-engine (5.8, fetched)
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/EColorVisionDeficiency (5.8, fetched)
- https://dev.epicgames.com/documentation/unreal-engine/common-ui-plugin-for-advanced-user-interfaces-in-unreal-engine (5.8, fetched)
- https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine (5.8, fetched)
- https://dev.epicgames.com/documentation/unreal-engine/linux-development-requirements-for-unreal-engine (5.8, fetched)
- https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine (5.8, fetched)
- https://docs.unrealengine.com/4.26/BlueprintAPI/Widget/Accessibility/SetColorVisionDeficiencyType/ (4.26, search excerpt)
- https://docs.unrealengine.com/4.27/ProgrammingAndScripting/DownloadingSourceCode (4.27, search excerpt)
- https://www.strayspark.studio/blog/game-accessibility-2026-unreal-engine (third party, fetched; not authoritative)
- https://wiki.archlinux.org/title/Unreal_Engine_5 (third party, search excerpt)
- https://www.fab.com/listings/bb2e4fbb-617c-41d3-aac6-e181eddf8b3b (vendor listing, search excerpt)
- https://steamdeckhq.com/steam-deck-verified-highlights-8-3-26-8-9-26/ (search excerpt)
- https://www.unrealengine.com/en-US/license, https://gateway.unrealengine.com/en-US/faq, https://map.twinmotion.com/en-US/faq (failed: 403/503)
