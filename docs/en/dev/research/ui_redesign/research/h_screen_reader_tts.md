# H. Screen reader and self-voicing TTS for games

Research date: 2026-10-21. Scope: libraries and OS APIs for speech output and accessibility trees, per-game implementation notes, blind-player UX requests, and a recommendation for a C++ SDL3 game.

Verification legend: **[V]** verified on a fetched page or search snippet this session. **UNVERIFIED** = from memory or from an unfetched source; check before relying on it. Where a source was a search snippet rather than a fetched page, it is noted.

## 1. Summary

1. Two different things are being compared: (a) **self-voicing**, where the game speaks its own UI through a TTS engine, and (b) **exposing an accessibility tree**, where a screen reader reads the game's UI through an OS API. Most shipped games do both or pick one.
2. Tolk (LGPL-3.0) is a Windows-only DLL that wraps JAWS, NVDA, SuperNova, ZoomText, Window-Eyes, System Access and SAPI. Its README says **"this project is not currently being developed"** [V].
3. NVDA Controller Client is a Windows DLL under **LGPL-2.1** [V]. It only works when NVDA is running.
4. Speech Dispatcher is the standard Linux TTS daemon. `libspeechd` (C client) is **LGPL-2.1-or-later** [V]. It needs a running daemon, not a screen reader.
5. AccessKit (Rust, C and Python bindings) is **Apache-2.0 or MIT** [V] and has adapters for UIA (Windows), NSAccessibility (macOS), AT-SPI (Unix), Android and iOS [V]. Bevy, egui, Freya, Slint and Xilem use it, per the AccessKit site [V]. **Godot** uses it, but Godot's own release page calls the integration **experimental** [V]. A Godot forum user reports TalkBack on Android does not work [V, user report].
6. SDL3 has **no** speech or accessibility API as far as I could find. Searches turned up only 2012–2015 SDL forum threads proposing one, with no shipped result [V, search snippets]. A fetch of the libsdl-org issue search returned three accessibility issues, none proposing a speech API (see section 6).
7. Unreal Engine 5 ships a "Slate Screen Reader" plugin and third-party reader support on Windows and iOS (VoiceOver), marked **Experimental** [V]. Unity has a third-party UI Accessibility Plugin (Apache-2.0 with LGPL NVDA controller bundled) [V, search snippet].
8. Recommended approach for a C++ SDL3 game: a small in-house speech abstraction with a per-OS backend (Tolk or NVDA+SAPI on Windows, AVSpeechSynthesizer on macOS, speech-dispatcher on Linux), used for self-voicing and announcements. Add an accessibility tree (AccessKit C API) later if the game needs real screen-reader navigation. See section 7.

## 2. Comparison table: libraries and APIs

| Library / API | Platform | Licence | How it speaks | Needs a screen reader running? | Maintained? | C/C++ usable? |
|---|---|---|---|---|---|---|
| **Tolk** (dkager/tolk) | Windows only (x86/x64 DLL) | LGPL-3.0 [V] | Forwards text to the active screen reader (JAWS, NVDA, SuperNova, ZoomText, Window-Eyes, System Access) or SAPI [V] | Not strictly. Falls back to SAPI when no reader is found, per the README's auto-detection order [V] | **No**: README says "this project is not currently being developed" [V]. 161 commits, 104 stars [V] | Yes, C-style DLL API; built with MSVC and Windows SDK [V] |
| **NVDA Controller Client** (`nvdaControllerClient.dll`) | Windows | LGPL-2.1 [V] | `nvdaController_speakText` and similar calls, sent to NVDA | **Yes**. Returns an error if NVDA is not running [V, error code note] | Repo page shows new functions added in NVDA 2024.1 and 2026.3 [V]; maintenance beyond that UNVERIFIED | Yes. C header `nvdaController.h`; examples in C, C#, Python, Rust [V] |
| **JAWS API** | Windows | Proprietary (Freedom Scientific) | JAWS scripting language (JSL) runs *inside* JAWS per application; third parties ship scripts for specific apps [V, search snippet] | **Yes** (JAWS) | Vendor-maintained | Not as a C library. Integration is via JAWS scripts or via Tolk/UIA [V] |
| **SAPI 5** (`ISpVoice`) | Windows | OS API (no licence concern) | Direct TTS through installed voices | No | Legacy but still shipped | Yes, via COM (`sapi.h`) [UNVERIFIED for API details and "legacy" status; my fetch of the docs page returned the wrong page] |
| **WinRT SpeechSynthesizer** (`Windows.Media.SpeechSynthesis`) | Windows 10+ | OS API | Synthesizes to a stream, plays through `MediaElement`. Only Microsoft-signed voices [V] | No | Current | Needs C++/WinRT or C++/CX. Designed around XAML/UWP media playback [V] |
| **UI Automation (UIA)** | Windows | OS API | Screen reader queries a provider tree (names, control types, patterns, events) | Yes, the consumer is the reader | Current Windows accessibility API [V, via AccessKit page] | Yes, via COM. Heavy to implement directly |
| **NSSpeechSynthesizer** | macOS | OS API | Old AppKit TTS | No | **Obsoleted in macOS 14.0**; message says use `AVSpeechSynthesizer` [V, via .NET API catalog snippet] | Objective-C/Swift only |
| **AVSpeechSynthesizer** (AVFoundation) | macOS 10.14+, iOS | OS API | Speaks text with `AVSpeechSynthesisVoice` voices [V, snippet] | No | Current | Objective-C/Swift. Needs a small ObjC++ bridge from C++ [UNVERIFIED that Apple's current docs match; Apple page fetch failed] |
| **NSAccessibility / VoiceOver** | macOS | OS API (AppKit protocol) | Exposes elements; VoiceOver reads them. Apps post notifications on UI change [UNVERIFIED: Apple page fetch returned only the title] | Yes (VoiceOver), but VoiceOver is always present on macOS | Current | Objective-C/Swift. AccessKit has a macOS adapter [V] |
| **speech-dispatcher / libspeechd** | Linux (also BSD) | Server: GPL-2.0 (mixed). C client `libspeechd`: LGPL-2.1-or-later [V] | Daemon queues messages and sends to eSpeak-NG, Festival, Piper, Mimic3 and others [V] | **No**. The daemon is the TTS service; Orca uses it too [V] | Repo active (4,204 commits, copyright 2018–2026) [V]. No release version visible | Yes, C library (`libspeechd`) [V] |
| **Orca** (GNOME screen reader) | Linux | GPL (UNVERIFIED version) | Uses speech-dispatcher by default [V] | N/A (it is a reader) | Active | Not a library |
| **AT-SPI2** (atspi, ATK) | Linux (D-Bus) | LGPL for libatspi (UNVERIFIED) | Apps expose a widget tree over D-Bus; Orca reads it [V] | Yes (Orca or other AT client) | Current | C client library `libatspi` exists [V]. Server side for a custom app means implementing the D-Bus interfaces yourself [V, Ubuntu docs] |
| **AccessKit** | Windows (UIA), macOS (NSAccessibility), Unix (AT-SPI over zbus), Android, iOS (planned web) [V] | Apache-2.0 or MIT [V] | Not a speech library. Provides an accessibility tree to the OS, which then reaches the screen reader | Yes, a screen reader is needed to hear it | Active (717 commits, 46 open issues on the repo page). No release date visible [V] | **C API** (`cbindgen` header) and Python (PyO3) [V] |
| **Godot 4.5 AccessKit integration** | Desktop, Android (partial) | MIT (Godot; UNVERIFIED) | Built on AccessKit; Control nodes plus "screen reader bindings" for any Node | Yes | Experimental [V] | GDScript/C# API |
| **Unreal Slate Screen Reader** | Windows, iOS (VoiceOver) | Epic engine | Experimental plugin with third-party reader support for listed UMG widgets [V] | Yes for third-party readers; Text To Speech plugin is English-only [V] | Shipped with UE5 | C++ via `SWidget::CreateAccessibleWidget()` [V] |
| **Unity UI Accessibility Plugin (UAP)** | Windows (SAPI, NVDA), Android, iOS, Mac, WebGL [V] | Apache-2.0 (examples excluded) [V] | Bundles NVDA controller client (LGPL-2.1) and SAPI fallback [V] | Optional (SAPI fallback) | Unknown | C# |
| **SDL3** | n/a | zlib | **No speech or accessibility API** found [V, search only] | n/a | n/a | n/a |

## 3. Library notes

### Tolk
- Windows-only DLL. README says it lets "Windows applications" output text, in 32- and 64-bit [V].
- Supported screen readers listed in the README: JAWS, Window-Eyes, NVDA, SuperNova, System Access, ZoomText and SAPI. SuperNova "does not have a 64-bit compatible API" [V].
- Licence: LGPL-3.0 [V]. Dynamic linking is the usual way to satisfy LGPL.
- Maintenance: explicitly not developed [V]. Forks may exist; I did not check.

### NVDA Controller Client
- LGPL-2.1, Windows DLL, x86/x64/arm64 builds [V].
- Functions named on the page: `nvdaController_speakText`, `nvdaController_isSpeaking` (NVDA 2026.3), `nvdaController_getProcessId` and `nvdaController_speakSsml` (NVDA 2024.1). Full list is in `nvdaController.h` [V].
- Returns non-zero on failure. Older NVDA versions return error 1717 for the newer functions [V].
- Only useful when NVDA runs. Tolk handles this for you by also covering JAWS and SAPI.

### speech-dispatcher / libspeechd
- README calls it a "high-level device independent layer for access to speech synthesis" [V].
- Licence split: server mostly GPLv2; C API client `libspeechd` is LGPL-2.1-or-later [V].
- Orca uses it by default [V]. Red Hat docs say it stops messages from other apps from interrupting Orca's messages [V].
- **Steam Deck / Flatpak: not verified.** My search found nothing on SteamOS or Flatpak access to the host speech-dispatcher socket. The Arch bug report suggests the actual dependency is `espeak-ng` [V, bug report]. Treat Flatpak and SteamOS support as **UNVERIFIED**. Speech-dispatcher is a host daemon, so a sandboxed Flatpak needs the socket exposed or a fallback.

### AccessKit
- Apache-2.0 or MIT at your option [V]. Note the repo sidebar says "BSD-3-Clause and 2 other licenses" because parts are derived from Chromium [V].
- Adapters: `accesskit_windows` (UIA), `accesskit_macos` (NSAccessibility), `accesskit_unix` (AT-SPI over zbus), `accesskit_android`, iOS [V].
- Caveat from the page: adapters "don't yet support all types of UI elements or all of the properties in the schema" [V].
- C API is generated with cbindgen, Python via PyO3 [V].
- AccessKit does **not** speak. It exposes a tree. A screen reader must be running.

### SDL3
- I found no SDL3 speech, TTS or accessibility API [V, negative search].
- The only design discussion I found is "Outputting text to accessibility tools" (2014–2015, SDL Discourse). It proposed SAPI and Speech Dispatcher as first backends. The discussion did not produce an API [V, snippet; the thread was not fetched in full].
- A 2012 mailing-list reply said SDL does not handle accessibility and suggested a separate library [V, snippet].
- Implication: a C++ SDL3 game must write its own speech layer, or use a library such as Tolk, NVDA controller client, speech-dispatcher or AccessKit, alongside SDL's window handle.

### Godot 4.5
- Official release page: "Thanks to AccessKit, we added screen reader support to Control nodes." The integration is "still in its experimental phase." Editor support is incomplete [V].
- Android: a forum user reports it does not work with TalkBack, and another user switched to an Android TTS plugin [V, forum reports, not official].

### Unreal Engine
- Three plugins: Text To Speech (English only), Screen Reader and Slate Screen Reader [V].
- Third-party readers supported on Windows and VoiceOver on iOS. Enable with `Accessibility.Enable=1`. Supported widgets include Text Block, Editable Text Box, Slider, Button and Checkbox [V].
- Custom Slate widgets: override `SWidget::CreateAccessibleWidget()` and return an `FSlateAccessibleWidget` subclass. Disabling `bCompileWithAccessibilitySupport` removes it [V].
- Labelled **Experimental** [V]. The docs do not say whether the Windows path uses UIA. UIA is likely but UNVERIFIED.

### Unity
- UAP (mikrima/UnityAccessibilityPlugin): Windows (SAPI, NVDA), Android, iOS, Mac, WebGL [V]. Bundles NVDA controller client under LGPL-2.1 [V].
- UnityAccessibilityLib (NuGet): .NET library, uses active reader or SAPI fallback, names NVDA and JAWS [V].

### Windows SAPI and WinRT
- WinRT `SpeechSynthesizer` is for voices installed on the system, Microsoft-signed only. It synthesizes to a stream and plays via MediaElement [V]. This is awkward from a plain Win32 C++ game. SAPI 5 COM (`ISpVoice`) is the simpler path for Win32 [UNVERIFIED; the fetch returned the wrong page].

### macOS
- NSSpeechSynthesizer: obsoleted in macOS 14.0; use AVSpeechSynthesizer [V, API catalog snippet].
- AVSpeechSynthesizer: available macOS 10.14+ [V, Lazarus wiki snippet]. A Mozilla bug reports `AVSpeechSynthesisVoice(language:)` can return the wrong default voice [V, bug snippet, unconfirmed].
- NSAccessibility and VoiceOver: I could not read Apple's page. Apple's post-notification API is UNVERIFIED.

## 4. Per-game implementation notes

Each note is labelled with its source. **Memory-only** items are UNVERIFIED.

### Hearthstone (HearthstoneAccess mod)
- Third-party mod by Guide Dev that makes Hearthstone accessible to blind players [V, hearthstoneaccess.com fetch].
- "Runs custom code inside the game"; players launch normally and use hotkeys [V, search snippet].
- Says most popular screen readers should work; developed and tested with NVDA. Falls back to Microsoft's Speech API if the reader is not detected [V, search snippet of project description]. Tolk is **not** mentioned in any source I found; whether the mod uses Tolk is UNVERIFIED.
- Windows only; requires English Hearthstone, preferably American English [V, snippet].
- Original developer stopped; community took over; Blizzard said it was improving native accessibility [V, snippet and fetch].
- Mercenaries mode has no planned accessibility work; tutorial, practice and solo adventures covered [V, hearthstoneaccess.com fetch].

### Stardew Valley (Stardew Access mod)
- Requires SMAPI. Required dependencies, per the setup guide, include Kokoro and Project Fluent [V, search snippet; the README defers to a setup page].
- README: makes the game playable for blind users on Windows, Linux and macOS. Narrates menus, dialogue and the world; includes an object tracker, tile viewer and grid movement [V].
- Speech backends named in the README: **Tolk** on Windows, `libspeechdwrapper` on Linux, `libspeak` on macOS [V]. This answers the Tolk question for this game: yes, it uses Tolk on Windows.
- Licence: GPL-3.0, plus LGPL-2.1 and LGPL-3.0 files; LICENSE-NVDA and LICENSE-TOLK are included [V].
- ~121 open issues, 1,406 commits on the dev branch, translations as separate mods [V].
- Hooks SMAPI: UNVERIFIED (I did not read the source).

### Minecraft (Java and Bedrock)
- Java narrator arrived in snapshot 17w13a (1.12). Mode options came in 17w15a. Later: hovered buttons (1.14), command suggestions (1.15), Narrator Hotkey option (1.20.2), volume tied to the Voice category (1.21.5) [V, Minecraft Wiki fetch].
- Modes: Off, All, Chat, System [V].
- Toggle: Ctrl+B (Cmd+B on Mac). Can be disabled in accessibility settings [V].
- Language is set by the host OS; Windows users can force a language via the registry [V].
- Narrator reads "chat messages and other text elements in the user interface, such as titles and selected buttons" [V].
- Bedrock: called "Text to speech"; added in Preview 1.21.100.23 [V, wiki fetch].
- How it reads UI internally: UNVERIFIED (Java source not checked). The wiki does not say.

### Civilization VI
- No official Firaxis screen reader update found in 2025 searches [V, negative].
- Community NVDA-based accessibility mod discussed on AppleVis in Sept 2026. The poster's own account says it was ChatGPT-written, so treat it as unverified [V, search snippet]. Claims coverage of base game, Rise and Fall, Gathering Storm and scenarios.
- Civ V has a community layer (Civ-V-Access on GitHub) [V, snippet].
- Civ VII: CivFanatics thread asks for accessibility; no official answer [V, snippet].

### The Last of Us Part II
- Official PlayStation accessibility page: on-screen text, including menus, can be voiced by Text-To-Speech. Turns on automatically with English and console TTS enabled [V, search snippet].
- Over 60 accessibility settings. Three presets for vision, hearing and motor [V].
- Vision preset enables TTS, High Contrast Display and Enhanced Listen Mode [V, snippet].
- Enhanced Listen Mode works like sonar for enemies [V, NPR/KUOW snippet].
- Design lead Emilia Schatz framed it as "can a player who is blind play this game" [V, snippet; exact wording not verified in full].

### Forza Horizon 5
- In-game screen reader: Settings > Accessibility > Screen Reader, Default/On/Off. Xbox system Narrator is separate [V, Forza support page snippet].
- Reads text, buttons and elements; separate Voice Chat TTS/STT [V, Xbox announcement snippet].
- CanIPlayThat rated it 9/10 for accessibility but described screen reader support as "iffy" [V, snippet].
- Voice, speed and verbosity controls: not found.

### Diablo IV
- Built-in screen reader, works with JAWS and NVDA; volume, speed and voice type adjustable [V, CanIPlayThat snippet].
- Reader speaks in-game menus per mmorpg.gg [V, snippet].
- Audio cues for hovered items and ground loot, configurable by rarity [V, snippet].
- Blind reviewer (rossminor.com) said gear, notifications and menus still need TTS and dungeon navigation help [V, snippet].
- Sources date from 2023 launch; not checked against current patches.

### Mortal Kombat 1
- Screen reader covers menu items and any on-screen text. Developers reportedly sequenced reading order per screen [V, snippet; "reportedly" in source].
- Set in Accessibility tab; volume adjustable; first launch prompt [V, snippet of support article].
- Reported bug: stays active even when set to Off. Not verified as fixed [V, one site report].

### Sea of Thieves
- Uses Xbox Narrator for menus. "Let Games Read to Me" extended to cosmetic chests, Shop UI, Pirate Log, Trading Company menus in Season 3 [V, CanIPlayThat snippet].
- Update 2.0.15.1 added "Narrate Toast Messages" and "Toast Message Duration" [V, snippet].
- Text-to-speech for chat is US-only per Family Gaming Database [V, third-party].

### Fortnite
- **Could not verify** a built-in screen reader or menu narration. Family Gaming Database lists subtitles and large text, not narration [V, third-party database]. Treat as unknown.

### Brogue, NetHack, DCSS
- **Brogue**: a MetaFilter commenter mentions "Brogue-SPEAK" with no detail [V, snippet]. Not verified.
- **NetHack**: a 2010 mailing-list post says it was playable blind in the 1990s DOS builds, with display problems [V, snippet]. The NetHack Guidebook is at man.archlinux.org; I did not read its accessibility options.
- **Dungeon Crawl Stone Soup**: only a 2021 Orca list post asking how to run it with a console reader [V, snippet]. The DCSS blind-play wiki page returned 404 (UNVERIFIED that it exists under that URL).
- Common ASCII-roguelike complaint: map is hard to parse. An audiogames.net post says some developers pair the map with a text description of nearby tiles [V, snippet].

### A Hero's Call (audio game)
- Developed by Out of Sight Games for blind players; turn-based RPG with quests; free version exists [V, Perkins and inviocean snippets].
- AccessWorld review by Aaron Preece exists but was not retrieved [V, reference only].
- Its UI design: UNVERIFIED (not played or read).

### Fortnite, Sea of Thieves, MK1, Diablo IV, FH5, TLOU2, Minecraft, Hearthstone, Stardew

## 5. UX rules blind players ask for (with sources)

Sources marked "search snippet" were not fetched in full. The audiogames.net forum fetches returned 403, so those posts are cited from snippets only.

1. **Menus should not wrap, or should let you choose.** Singular Sound feature request: "there should definitely be an option to disable menu wrapping." Source: https://forum.singularsound.com/t/feature-requests-for-visually-impaired-users/173 [V, fetched]. Same thread asks for "a command to jump to the top of the menu."
2. **Give the first letter or instant letter shortcuts in menus.** frastlin's audiogames.net convention list: arrow keys and Enter for menus, with letter shortcuts or first-letter navigation. Source: https://forum.audiogames.net/post/309150/ [V, search snippet; 403 on fetch].
3. **Give a volume and feedback path without sight.** AppleVis game-developer thread: suggestions include "swipe up to increase volume by 5% or 10%, with spoken confirmation" and a two-second hold gesture for the pause menu. Source: https://applevis.com/comment/53604 and siblings [V, search snippet; 403 on fetch]. Treat as a discussion, not a consensus.
4. **Audio cues should be configurable.** Diablo IV lets players choose which item rarities trigger cues [V, CanIPlayThat snippet].
5. **Speech should not be interrupted by other apps.** Red Hat docs describe speech-dispatcher's role in stopping other apps from interrupting Orca messages [V, snippet]. Applies to any game speech queue.
6. **Reading order must be designed per screen.** MK1 developers reportedly "programmed the order in which text is read aloud" for every screen [V, snippet; "reportedly"]. Treat this as a requirement for self-voicing too.
7. **Menu narration must be toggleable and have a separate volume.** Minecraft: modes Off/All/Chat/System, plus a hotkey that can be disabled [V]. MK1: Screen Reader volume in Accessibility [V, snippet]. FH5: Default/On/Off [V].
8. **Tutorials and onboarding must be readable.** AppleVis commenter: a pause-menu gesture "is somehow being explained at some point", perhaps in help or a tutorial [V, snippet].
9. **Make the TTS language follow the OS**, not the game. Minecraft does this and forces a registry edit to override [V].
10. **Chat and notifications need their own toggles.** Sea of Thieves: "Narrate Toast Messages" and a "Toast Message Duration" setting [V, snippet].
11. **Ask for feedback and report bugs to the vendor.** Blizzard asks for accessibility feedback at accessibility@blizzard.com [V, rossminor.com snippet].
12. **"Where am I" and verbosity keys**: **Could not verify** from a source this session. The audiogames.net post that might cover it returned 403.

## 6. Could not verify

- SDL3 GitHub issues about speech or TTS. The issue search returned three unrelated accessibility issues (#14450 mobile input boxes, opened 2025-11-12, open; #8944 keyboard hook for JAWS, opened 2024-01-28, open; #7463 screen reader feature request, closed 2023-11-08). None was a TTS API proposal [V, fetched page; status from page wording].
- Apple NSAccessibility and AVSpeechSynthesizer official pages (fetch returned 404 or title only). NSSpeechSynthesizer deprecation version: confirmed only through an API catalog snippet.
- Microsoft SAPI 5 overview (fetch returned an unrelated page). WinRT is verified; SAPI details are UNVERIFIED.
- JAWS developer/scripting API reference. Only third-party vendor pages were found.
- Speech-dispatcher on Steam Deck (SteamOS) and inside Flatpak.
- Unreal Slate Screen Reader: whether the Windows path uses UIA.
- Whether Tolk has forks maintained after the "not currently being developed" notice.
- Whether AccessKit has a release in 2026 (no date visible on the repo page).
- Godot AccessKit on macOS and Windows beyond what the release notes say.
- Civilization VI community mod: name, repo, licence, and status (only an AppleVis snippet; the poster said it was ChatGPT-written).
- Fortnite menu narration.
- Brogue-SPEAK details, DCSS blind-play wiki contents, NetHack accessibility options.
- The Last of Us Part II Emilia Schatz quote, exact wording.
- Minecraft Java internal mechanism for reading UI.
- Stardew Access's SMAPI hooking method (source not read).
- Hearthstone Access: whether Tolk is used (only a Microsoft Speech API fallback is mentioned).
- Audiogames.net opinions on "where am I" keys, verbosity levels and speech interruption. The relevant audiogames.net posts returned 403 or were not reached.
- AT-SPI server implementation details for SDL specifically. No SDL-specific guidance found.
- Any claim about screen reader support in Sea of Thieves beyond the snippets.

## 7. Recommended approach for a C++ SDL3 game (Linux, macOS, Windows)

Assumptions: the game is a single-player SDL3 app with a custom UI (not GTK/Qt/Win32 controls), so the OS does not see its widgets. Pick the lowest-cost option that gives a blind player the UI.

**Phase 1: self-voicing through a small speech layer.** Works with or without a screen reader.

- Define one interface, for example `speech::say( text, interrupt )`, with one backend per OS, selected at compile time or at runtime.
- **Windows**: call Tolk if the DLL is present (it covers NVDA, JAWS, SAPI fallback). Tolk is LGPL-3.0, so link dynamically and ship the DLL under its licence. Tolk is unmaintained, so budget to fork or replace it. Alternatively, call NVDA controller client (LGPL-2.1) with SAPI 5 as fallback. Do not use WinRT from a Win32 game unless you already need it.
- **macOS**: `AVSpeechSynthesizer` via a small Objective-C++ file. Do not use NSSpeechSynthesizer (obsoleted in macOS 14). VoiceOver ignores the game's speech, so decide whether to speak only when VoiceOver is off.
- **Linux**: `libspeechd` (LGPL-2.1-or-later) with `dlopen` so the build works without it. The daemon must be running. Flatpak and SteamOS need separate testing (UNVERIFIED).
- Keep the queue and interrupt rules in the game layer: a focus change interrupts, a log message does not, with separate volume and verbosity settings (see section 5).

**Phase 2: real accessibility tree, only if the game needs screen-reader navigation.**

- AccessKit (Apache-2.0 or MIT) exposes one tree to UIA, NSAccessibility and AT-SPI. Use its C API. It is the only option in this table that covers all three desktops from one codebase.
- Costs: you must build the tree yourself from the UI layer, and the AccessKit schema does not yet cover all element types (page caveat). Godot's experience shows the integration is still young (experimental) [V].
- Do not implement UIA, NSAccessibility or AT-SPI by hand unless AccessKit is unusable.

**Do not** rely on SDL3 for any of this. It has no speech or accessibility API [V].

**Rule of thumb:** Phase 1 gives a blind player a playable UI quickly. Phase 2 is the larger investment and should wait until the self-voiced UI works. The dependency licences (LGPL/Apache/MIT) are compatible with a GPL game through dynamic linking, but check the game's own licence before shipping.

## Sources

- Tolk README: https://github.com/dkager/tolk [V, fetched]
- AccessKit site: https://accesskit.dev/ [V, fetched]
- AccessKit repo: https://github.com/AccessKit/accesskit [V, fetched]
- Godot 4.5 release notes: https://godotengine.org/releases/4.5/ [V, fetched]
- Godot 4.5 coverage (search snippet): https://itsfoss.com/news/godot-4-5-release/
- Godot Android TalkBack forum (user report): https://forum.godotengine.org/t/question-about-accessibility-on-android/122578 [V, snippet]
- NVDA Controller Client readme: https://github.com/nvaccess/nvda/blob/master/extras/controllerClient/readme.md [V, fetched]
- Speech Dispatcher repo: https://github.com/brailcom/speechd [V, fetched]
- Orca (search snippet): https://github.com/GNOME/orca [V, snippet]
- Microsoft WinRT SpeechSynthesizer: https://learn.microsoft.com/en-us/uwp/api/windows.media.speechsynthesis.speechsynthesizer [V, fetched]
- SDL forum thread (2014–2015): https://discourse.libsdl.org/t/outputting-text-to-accessibility-tools/21113 [V, snippet]
- SDL forum 2012 reply (search snippet): https://discourse.libsdl.org/t/compatibility-with-screen-readers/34831 [V, snippet]
- SDL issue list: https://github.com/libsdl-org/SDL/issues?q=accessibility%20screen%20reader [V, fetched]
- NSSpeechSynthesizer obsoletion (API catalog snippet): https://catalog.apireview.net/catalog/3959fbb0ddcec599e727e0c72d29bb4a [V, snippet]
- AVSpeechSynthesizer availability (Lazarus wiki snippet): https://wiki.lazarus.freepascal.org/macOS_Text-To-Speech [V, snippet]
- Unreal screen reader docs: https://dev.epicgames.com/documentation/en-us/unreal-engine/supporting-screen-readers-in-unreal-engine [V, fetched]
- Unreal accessibility overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/blind-accessibility-features-overview-in-unreal-engine [V, snippet]
- Unity UAP: https://github.com/mikrima/UnityAccessibilityPlugin [V, snippet]
- AT-SPI2 wiki: https://wiki.freedesktop.org/www/Accessibility/AT-SPI2/ [V, snippet]
- Ubuntu accessibility stack: https://ubuntu.com/desktop/docs/en/24.04/explanation/accessibility-stack/ [V, snippet]
- Red Hat accessibility docs: https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/10/html/using_the_gnome_desktop_environment/enabling-accessibility-for-visually-impaired-users [V, snippet]
- Arch Linux Orca bug report: https://bugs.archlinux.org/task/76335 [V, snippet]
- JAWS scripting (TPGi): https://www.tpgi.com/accessibility-solutions/jaws-scripting [V, snippet]
- HearthstoneAccess site: https://www.hearthstoneaccess.com/ [V, fetched]
- HearthstoneAccess announcement (search snippet): https://caniplaythat.com/2022/11/12/hearthstone-accessibility-to-be-improved-as-mod-support-ends/ [V, snippet]
- Stardew Access README: https://github.com/stardew-access/stardew-access [V, fetched]
- Accessible Stardew Setup: https://github.com/ParadoxiKat/AccessibleStardewSetup [V, snippet]
- Stardew forum (early beta): https://forums.stardewvalley.net/threads/mod-for-screen-reader-accessibility.8793 [V, snippet]
- Minecraft Wiki Narrator: https://minecraft.wiki/w/Narrator [V, fetched]
- Civilization 6 AppleVis thread: https://www.applevis.com/forum/windows/civilization-6-pc-fully-accessible [V, snippet; poster said ChatGPT-written]
- Civ-V-Access (search snippet): see Civ VI search results [V, snippet]
- The Last of Us Part II PlayStation accessibility: https://www.playstation.com/en-vn/games/the-last-of-us-part-ii/accessibility/ [V, snippet]
- The Last of Us Part II NPR: https://www.npr.org/transcripts/889820512 [V, snippet]
- Forza Horizon 5 screen reader support: https://support.forza.net/hc/en-us/articles/11380343742227-Screen-Reader-Narrator [V, snippet]
- Forza Horizon 5 CanIPlayThat review: https://caniplaythat.com/2021/11/04/forza-horizon-5-accessibility-review-can-i-play-that/ [V, snippet]
- Diablo IV CanIPlayThat: https://caniplaythat.com/2023/07/18/diablo-iv-accessibility-review/ [V, snippet]
- Diablo IV blind review: https://rossminor.com/2023/06/08/diablo-iv-blind-accessibility-review/ [V, snippet]
- Diablo IV mmorpg.gg: https://mmorpg.gg/blizzard-entertainment-announces-enhanced-accessibility-features-for-diablo-iv/ [V, snippet]
- Mortal Kombat 1 support: https://mortalkombatgamessupport.wbgames.com/hc/articles/19435787154963 [V, snippet]
- Mortal Kombat 1 CanIPlayThat: https://caniplaythat.com/2023/09/13/mortal-kombat-1-accessibility-details-released/ [V, snippet]
- Sea of Thieves CanIPlayThat (Season 3): https://caniplaythat.com/2021/06/22/sea-of-thieves-season-3s-first-accessibility-features-revealed/ [V, snippet]
- Sea of Thieves update 2.0.15.1: https://seaofthieves.wiki.gg/wiki/2.0.15.1 [V, snippet]
- Family Gaming Database (Fortnite): https://www.familygamingdatabase.com/accessibility/Fortnite [V, snippet, third-party]
- Singular Sound forum feature requests: https://forum.singularsound.com/t/feature-requests-for-visually-impaired-users/173 [V, fetched]
- audiogames.net convention post: https://forum.audiogames.net/post/309150/ [V, snippet; 403 on fetch]
- AppleVis game-developer thread: https://applevis.com/comment/53604 [V, snippet; 403 on fetch]
- Perkins audio games list: https://www.perkins.org/resource/audio-games-for-blind-low-vision-gamers/ [V, snippet]
- A Hero's Call (inviocean): https://inviocean.com/?p=975 [V, snippet]
- AccessWorld (A Hero's Call review reference): https://afb.org/aw/22/12/17800 [V, snippet, not read]
