# Unity 6 as a frontend for a large C++ game: research

Date of research: 2026-10-21. Tags: **VERIFIED** = read on an official page in this session (docs.unity3d.com, unity.com, github.com) or a forum post where stated. **NOT VERIFIED** = from search snippets only, from memory, or unreachable. The WebSearch quota for the session was reached partway through (the tool reported the shared limit as used up, not the count), so later items rely on WebFetch of known URLs only.

Repo facts used below were checked locally: `README.md` line 177-181 says the project is CC BY-SA 3.0 and that some code is under other software licences.

---

## 1. Accessibility

| Claim | Tag | Source |
|---|---|---|
| The `AssistiveSupport` / `AccessibilityHierarchy` APIs are the entry point for screen readers. Unity's own UI is not accessible to assistive tech by default. | VERIFIED | https://docs.unity3d.com/Manual/accessibility/screen-readers-get-started.html (search snippet of the same page family: https://docs.unity3d.com/ScriptReference/Accessibility.AccessibilityHierarchy.html) |
| The hierarchy is UI-system agnostic (UI Toolkit, uGUI, custom, non-UI objects). Screen readers "cannot detect GameObject components or UI elements directly", so you must build the hierarchy. | VERIFIED | https://docs.unity3d.com/Manual/accessibility/screen-readers-get-started.html |
| The hierarchy is activated via `AssistiveSupport.activeHierarchy` and dropped when the screen reader turns off, so it must be reassigned on each screen-reader status change. | VERIFIED (snippet of 6000.7 AssistiveSupport reference) | https://docs.unity.com/en-us/engine/6000.7/script-reference/unityengine/accessibility/assistivesupport |
| The module is enabled by default and can be enabled from Package Manager > Built-in. | VERIFIED | https://docs.unity3d.com/Manual/accessibility/screen-readers-get-started.html |
| The guide lists **Android (TalkBack), iOS and macOS (VoiceOver), Windows (Narrator)** as the platforms to test on. The page states no per-platform limitations and no roadmap. | VERIFIED | https://docs.unity3d.com/Manual/accessibility/screen-readers-get-started.html |
| An older snippet of the 6000.7 AssistiveSupport reference names **Android 8.0 (API 26)** as the only platform. This conflicts with the 6.6 guide above. The conflict is unresolved; the Windows and macOS docs were not read directly for the versions in question. | VERIFIED (conflict) | https://docs.unity.com/en-us/engine/6000.7/script-reference/unityengine/accessibility/assistivesupport |
| Unity 6.3 brings "native screen reader support to Windows and macOS" via the new APIs. This is from a Unity forum post dated 2025-02-07, not an official page. | NOT VERIFIED (forum only) | https://discussions.unity.com/t/unity-for-people-with-visual-impairments/361660 |
| The `invoked` event was renamed from `selected` in Unity 6.3. | VERIFIED | https://docs.unity3d.com/Manual/accessibility/screen-readers-get-started.html |
| The introduction version of the Accessibility module is not stated. | NOT VERIFIED | (no source found) |
| **Unity Accessibility Plugin (UAP)**, mikrima/UnityAccessibilityPlugin: Apache-2.0 licence (Example folder excluded). Platforms per README: Android, iOS, Windows (SAPI and NVDA), Mac, WebGL (some limitations). Tested only up to Unity 2020.1. Uses text-to-speech and "reimplements the functionality of VoiceOver"; it does not expose Unity UI to native readers. Latest commit date not visible; 27 commits, no releases. | VERIFIED | https://github.com/mikrima/UnityAccessibilityPlugin |
| UAP is "partially" a fix: screen-reader users must turn their own screen reader off to play, per a forum post. | VERIFIED (forum) | https://discussions.unity.com/t/unity-for-people-with-visual-impairments/361660 |
| NVDA controller client is LGPL 2.1 (usable, not modifiable). | VERIFIED | https://github.com/mikrima/UnityAccessibilityPlugin |
| UAP maintenance status (active or abandoned) is **NOT VERIFIED**; the repo page does not state it. | NOT VERIFIED | https://github.com/mikrima/UnityAccessibilityPlugin |
| Built-in colour-blind mode in Unity: **NOT VERIFIED** (not searched or fetched). | NOT VERIFIED | (no source) |
| Text scaling via UI Toolkit Panel Settings: scale modes `ConstantPixelSize` (0), `ConstantPhysicalSize` (1), `ScaleWithScreenSize` (2). `ConstantPhysicalSize` uses Reference DPI vs actual screen DPI, with Fallback DPI when DPI is unknown. | VERIFIED (snippet) | https://docs.unity3d.com/Documentation/ScriptReference/UIElements.PanelSettings-referenceDpi.html ; https://docs.unity3d.com/Manual/UIE-Runtime-Panel-Settings.html |
| Panel Settings page on 6.0 manual could not be fetched (404). Whether Panel Settings reads the OS text-size preference: **NOT VERIFIED**. | NOT VERIFIED | https://docs.unity3d.com/6000.0/Documentation/Manual/UIE-PanelSettings.html (404) |
| Input System: `InputActionRebindingExtensions.PerformInteractiveRebinding()`, `SaveBindingOverridesAsJson` and `LoadBindingOverridesFromJson` exist for runtime rebinding. | VERIFIED | https://docs.unity3d.com/Packages/com.unity.inputsystem@1.14/manual/ActionBindings.html |
| Display strings: `GetBindingDisplayString` / `InputBinding.ToDisplayString` / `InputControlPath.ToHumanReadableString`. The page does not show glyph art. Gamepad glyph artwork support: **NOT VERIFIED**. | VERIFIED (strings); NOT VERIFIED (glyphs) | https://docs.unity3d.com/Packages/com.unity.inputsystem@1.14/manual/ActionBindings.html |

**Assessment.** Unity's built-in screen reader support is mobile-first with desktop added in 6.3 per a forum post. Anything desktop (Windows and Linux) for a text-heavy game like this one still needs either a native path or UAP. Linux is not covered by any Unity accessibility API found. This is a material gap for a roguelike whose UI is mostly text.

---

## 2. Embedding C++

| Claim | Tag | Source |
|---|---|---|
| IL2CPP converts IL to C++ and compiles it to native code. Cross-compilation is generally not supported. Linux is the exception: building Linux Players on other desktop platforms is supported. | VERIFIED | https://docs.unity3d.com/Manual/il2cpp-introduction.html |
| IL2CPP build time and binary size increase compared to Mono ("generally increases both the build time and the size"). | VERIFIED | https://docs.unity3d.com/Manual/il2cpp-introduction.html |
| IL2CPP is supported on all platforms; Linux and QNX allow LTO mode changes. | VERIFIED | https://docs.unity3d.com/Manual/il2cpp-introduction.html |
| Native plugins are unmanaged libraries; managed code calls them via `DllImport`; both precompiled libraries and source can be used. | VERIFIED | https://docs.unity3d.com/Manual/NativePlugins.html (redirects to http://docs.unity3d.com/Manual/plug-ins-native.html) |
| Unity's API is not thread-safe; use it only from the main thread. Development builds throw "can only be called from the main thread"; live builds do not check and may crash. | VERIFIED (quoted in search snippets from the Unity multithreading page; the exact page was not fetched) | https://docs.unity.com/en-us/engine/6000.6/manual/scripting/programming-distribute-work-threads/async-await-support/async-awaitable-continuations |
| `Awaitable.BackgroundThreadAsync()` moves work off the main thread, and continuation resumes on the main thread. | VERIFIED (snippet) | https://docs.unity.com/en-us/engine/6000.6/manual/scripting/programming-distribute-work-threads/async-await-support/async-awaitable-continuations |
| Native plugin callbacks: which thread they fire on, `[MonoPInvokeCallback]` / AOT attribute requirements, `IUnityInterfaces`, calling `Debug.Log` from native code, and "Call functions" rules. | NOT VERIFIED (the landing page points to sub-topics that were not fetched) | https://docs.unity3d.com/Manual/plug-ins-native.html |
| P/Invoke marshaling follows .NET rules with a few unsupported types. | VERIFIED (snippet) | https://docs.unity3d.com/2020.1/Documentation/Manual/windowsstore-plugins-il2cpp.html |
| Source-level C++ compiled into the IL2CPP player, and `"__Internal"` linking. | VERIFIED (snippet; the page is an old UWP-era guide) | https://docs.unity3d.com/2020.1/Documentation/Manual/windowsstore-plugins-il2cpp.html |
| Known C++ engine to Unity frontend case studies: **none found**. Not searched further (budget). | NOT VERIFIED | (no source) |

**Design implications (analysis, not sourced).**
- A blocking simulation loop must not run on the Unity main thread. Run the C++ sim on its own thread, and marshal state to Unity via a snapshot or double-buffer, pulled once per frame. Unity's main-thread rule (above) applies to Unity objects, not to raw native memory.
- Callbacks from C++ into C# need a static function with the attribute required for IL2CPP (per the native-plugin pages, NOT VERIFIED here) and must not throw across the boundary.
- Debugging across the boundary means two debuggers (Visual Studio/Rider for C#, GDB/LLDB for C++). Cost: significant. This is analysis, not a verified claim.

---

## 3. Licensing 2025-2026

| Claim | Tag | Source |
|---|---|---|
| Personal (free): revenue and funding up to $200,000 in the most recent 12 months; measured on each day of use; above that, Personal cannot be used at all. | VERIFIED | https://unity.com/products/pricing-updates ; https://unity.com/legal/editor-terms-of-service/software |
| Pro is required above $200,000 total annual revenue and funding (wording "over"). | VERIFIED | https://unity.com/products/pricing-updates |
| Enterprise is required above $25M annual revenue. | VERIFIED | https://unity.com/products/pricing-updates |
| Pro price 2026: $2,310 per seat per year prepaid, or $210 per seat per month. | VERIFIED (official page); a Nov 2025 trade report says a 5% rise effective 12 Jan 2026 (NOT VERIFIED beyond snippet) | https://unity.com/products/pricing-updates ; https://www.cgchannel.com/?p=171673 |
| Runtime Fee cancelled: decided 12 Sept 2024, "effective immediately", and "No new or existing Unity games have been, or will be, subject to the Runtime Fee." | VERIFIED | https://unity.com/runtime-fee |
| Splash screen optional for Personal with Unity 6. | VERIFIED | https://unity.com/products/pricing-updates |
| Havok Physics is no longer included with Pro from Unity 6.3. | NOT VERIFIED (snippet only) | https://unity.com/products/pricing-updates |
| Unity 6 and earlier projects get royalty-free runtime distribution, conditional on continuing Tier Eligibility. Industry Customers need express authorisation. | VERIFIED | https://unity.com/legal/editor-terms-of-service/software (section 2.2) |
| Editor source code: "Read Only" for internal troubleshooting; modifications may not be published or distributed with a project. | VERIFIED (snippet) | https://unity.com/en/legal/editor-source-code-terms |
| Open-source project exemption from the Personal terms: **none found** in the Editor Terms. Free use under Personal would rely on the revenue threshold, which a free open-source project meets. | NOT VERIFIED (absence in the text checked) | https://unity.com/legal/editor-terms-of-service/software |

**Licence conflict analysis (not legal advice).**
- Cataclysm is CC BY-SA 3.0 (repo `README.md` lines 177-181). The CC FAQ states: "We recommend against using Creative Commons licenses for software." It suggests FSF/OSI software licences, and says CC0 is acceptable for software (VERIFIED: https://creativecommons.org/faq/#can-i-apply-a-creative-commons-license-to-software). The repo already mixes licences for code. A Unity port inherits the same mix.
- Unity Runtime distribution does not conflict with CC BY-SA: the Runtime is proprietary but is a separate component licensed to you under section 2.2; the game's own licence governs your code and content. Confirmed only at the level of the text read.
- Unity project files (YAML scenes, `.meta` files) are human-readable but not a source-code form suited to CC BY-SA attribution. This is analysis; not verified against any Unity or CC guidance.
- Editor source access is read-only and cannot be redistributed. A fork of the Unity Editor is not needed for this project.

---

## 4. Platforms and CI

| Claim | Tag | Source |
|---|---|---|
| Linux editor is supported on Ubuntu only ("Unity provides Linux support only for Ubuntu"). | VERIFIED | https://docs.unity3d.com/6000.4/Documentation/Manual/linux.html |
| Linux standalone players can be built and run on Linux systems supported by Unity. | VERIFIED | https://docs.unity3d.com/6000.4/Documentation/Manual/linux.html |
| Ubuntu versions listed: 22.04 and 24.04 (Unity 6.0 system requirements); Unity 6 Beta listed 20.04 and 22.04. Wayland details vary by release (6.6 page says supported on AMD and Nvidia 550+). | NOT VERIFIED (search snippets; system-requirements pages not fetched) | https://docs.unity.com/en-us/engine/6000.6/manual/get-started/install-and-upgrade/getting-started-installing-unity/system-requirements |
| Batch mode: `-batchmode` runs without human interaction. `-nographics` means "Unity doesn't initialize the graphics device". `-quit`, `-executeMethod`, `-logFile`, `-buildTarget`, `-projectPath` all exist. | VERIFIED | https://docs.unity3d.com/Manual/EditorCommandLineArguments.html |
| Test Framework: `-runTests`, `-testPlatform` (EditMode default, PlayMode, or a build target), `-testResults` (NUnit XML), `-testFilter`, `testCategory` (note: documented as `testCategory`, not `-category`). | VERIFIED | https://docs.unity3d.com/Packages/com.unity.test-framework@1.4/manual/reference-command-line.html |
| `-nographics` does not work for automated input without a focused window; GI baking not supported. | VERIFIED | https://docs.unity3d.com/Manual/EditorCommandLineArguments.html |
| GameCI (`game-ci/unity-builder@v4`) activation: free licence stored as `UNITY_LICENSE` (`.ulf` file) plus `UNITY_EMAIL` and `UNITY_PASSWORD`; Pro uses `UNITY_SERIAL`. A floating licence server can be used. | VERIFIED | https://game.ci/docs/github/activation/ |
| Whether personal-licence activation works on free GitHub runners without a paid plan, and the licence's terms for CI: **NOT VERIFIED** (the GameCI page does not say). | NOT VERIFIED | https://game.ci/docs/github/activation/ |
| Steam Deck support: **NOT VERIFIED** (not searched). | NOT VERIFIED | (no source) |
| Build sizes for a C++ backed game: not measured; no Unity figure found. | NOT VERIFIED | (no source) |
| This repo runs no GitHub Actions per `AGENTS.md`, so CI here is local only. | VERIFIED (project file) | /home/richard/Projects/Cataclysm-BN-3D/AGENTS.md |

---

## 5. Rendering

| Claim | Tag | Source |
|---|---|---|
| `Graphics.RenderMeshInstanced` draws instanced meshes from script with a prebuilt material that has "Enable GPU Instancing". | VERIFIED | https://docs.unity3d.com/Manual/GPUInstancing.html |
| Under URP or HDRP, custom shaders need the SRP Batcher disabled or the shader made incompatible with it. Under the Built-in Render Pipeline, Shader Graph shaders do not support GPU instancing. | VERIFIED | https://docs.unity3d.com/Manual/GPUInstancing.html |
| Instancing benefit depends on the platform and the GPU, and is larger on mobile than desktop. | VERIFIED | https://docs.unity3d.com/Manual/GPUInstancing.html |
| `Graphics.RenderMeshIndirect(ref RenderParams, Mesh, GraphicsBuffer argsBuffer, int commandCount, int startCommand)` draws many instances of one mesh from a GPU args buffer, with multiple commands per call. | VERIFIED | https://docs.unity3d.com/ScriptReference/Graphics.RenderMeshIndirect.html |
| `RenderMeshIndirect` "only works on platforms that support compute shaders." | VERIFIED | https://docs.unity3d.com/ScriptReference/Graphics.RenderMeshIndirect.html |
| `RenderMeshIndirect` needs the `UNITY_INDIRECT_DRAW_ARGS` define, `UnityIndirect.cginc`, and `InitIndirectDrawArgs(0)` in the shader. | VERIFIED | https://docs.unity3d.com/ScriptReference/Graphics.RenderMeshIndirect.html |
| The Mesh API (`Mesh.SetVertices`, `SetIndices` with `MeshUpdateFlags`) for dynamic per-frame mesh generation: **NOT VERIFIED** (not fetched). | NOT VERIFIED | (no source) |
| 2D sprite billboards in a 3D world: **NOT VERIFIED** as a Unity-documented technique. Analysis: a camera-facing quad per sprite, instanced via `RenderMeshInstanced`, is the usual approach. | NOT VERIFIED | (no source) |
| The current renderer (`src/r3d_*`) is an existing C++ path; a port would replace it with Unity draw calls. The cost is analysis-only here. | VERIFIED (repo) | /home/richard/Projects/Cataclysm-BN-3D/AGENTS.md (r3d reference) |

---

## Summary table

| Area | Status in one line | Tags |
|---|---|---|
| Accessibility (screen readers) | Built-in path exists; mobile-first, Windows/macOS added in 6.3 per forum only; UAP is Apache-2.0 but stale (tested to 2020.1). | VERIFIED / NOT VERIFIED mixed |
| Text scaling / input | Panel Settings DPI modes and Input System rebinding confirmed. | VERIFIED |
| Colour-blind | No built-in found. | NOT VERIFIED |
| Native C++ embedding | IL2CPP and P/Invoke confirmed; threading and callback rules not confirmed. | VERIFIED / NOT VERIFIED mixed |
| Licensing | Personal below $200K; Runtime Fee cancelled; Unity 6 runtime distribution royalty-free; CC BY-SA is not a software licence. | VERIFIED |
| Linux | Editor on Ubuntu only; Linux player supported; IL2CPP cross-builds to Linux allowed. | VERIFIED |
| CI | GameCI activation documented; free-runner behaviour not verified. | VERIFIED / NOT VERIFIED mixed |
| Rendering | Instancing and RenderMeshIndirect confirmed; Mesh API and billboards not verified. | VERIFIED / NOT VERIFIED mixed |

---

## Could not verify

1. Unity 6.3 native Windows and macOS screen reader API (only a 2025 forum post).
2. Which Unity version introduced the Accessibility module.
3. UAP maintenance status after 2020.1.
4. Colour-blind support in Unity.
5. Whether Panel Settings follows the OS text-size preference (6.0 page returned 404).
6. Gamepad glyph artwork support in the Input System.
7. Native plugin threading and callback rules (`[MonoPInvokeCallback]`, `IUnityInterfaces`, Debug.Log from native).
8. Known C++-engine-to-Unity case studies (none found).
9. Ubuntu versions per Unity 6 release (search snippets only).
10. Whether GameCI personal-licence activation works on free GitHub runners.
11. Steam Deck support.
12. Build sizes for this game.
13. Havok Physics removal in Unity 6.3 (snippet only).
14. The exact Pro price change from 12 Jan 2026 (only a trade-report snippet).
15. The Unity Mesh API details for per-frame mesh generation.
16. Billboard sprite technique in Unity.
17. Unity's automated-input behaviour with `-nographics` for test automation beyond the statement quoted above.

The WebSearch quota was reached partway through; items that needed a search were checked with WebFetch only where a URL was known.
