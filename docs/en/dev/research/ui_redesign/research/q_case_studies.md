# Case studies: moving a C/C++ game or simulation onto a general-purpose engine

Research date: 2026-10-21. Labels: **VERIFIED** (page fetched and quoted, or primary source text read), **SNIPPET** (only seen in search-result text, not fetched), **NOT VERIFIED** (no source found in this session, or claim comes from memory).

**Session limit:** the WebSearch budget ran out partway through (shared 200-call session limit). Sections below are therefore incomplete. Remaining gaps are listed under "Could not verify".

---

## 1. Open-source / indie projects embedding an existing C/C++ core in Godot (or Unity/Unreal)

- **Cataclysm: DDA / Cataclysm-BN as a Godot frontend.** No Godot frontend found. A search returned only unrelated Godot projects. **NOT FOUND.** Search: https://www.google.com (no URL; WebSearch result list only).
- **Godot GDExtension for a custom simulation (racing sim).** GodotCon Amsterdam 2026 talk "Godot at the limit of grip!" (Marcel Offermans): a C++ GDExtension runs a custom physics engine (Project Chrono) at 1000 Hz, with GDScript for the dashboard. The abstract promises a lessons-learned section; the content was not read. **SNIPPET.** https://talks.godotengine.org/godotcon-ams-2026/talk/GAWZDR.ics
- **Godot's own guidance on GDExtension.** The docs say GDExtension does not require compiling the engine, that godot-cpp only requires compiling your own library, and that GDExtension is experimental and has less access than static modules. **SNIPPET** (search result text from docs.godotengine.org translations; the English page was not fetched). https://docs.godotengine.org/fr/4.x/_sources/tutorials/scripting/cpp/about_godot_cpp.rst.txt
- **Dwarf Fortress (Steam, 2022).** Not an engine product in the sense of Unity/Godot. The GamingOnLinux article reports the game was upgraded "from the ancient SDL to SDL 2, along with many optimisations" and that SDL 2 is a "stepping stone to ports". **VERIFIED.** https://www.gamingonlinux.com/2023/06/dwarf-fortress-getting-close-to-a-linux-release-on-steam/ . **NOT VERIFIED:** that it is "its own engine, not Unity" (the article does not mention Unity or the engine architecture).
- **Caves of Qud (Freehold Games).** Brian Bucklew said in an RPG Site interview that he built a component-based roguelike engine in C# ("let me try to build one of these component-based engines in this new language C#"). **SNIPPET.** https://www.rpgsite.net/interview/20000-brian-bucklew-caves-of-qud-interview-expansions-switch-port-unity-controller-support-coffee . Unity Systems-driven design article exists (unity.com/resources/systems-driven-design-in-caves-of-qud): **SNIPPET**, content not read. The claim "Unity only for the UI" comes from a secondhand GOG forum post: **NOT VERIFIED.**
- **Caves of Qud Godot port test (2023).** Article: Bucklew "decided to see just how much work would be involved to port Caves of Qud from Unity to Godot". The article reports he "logged about 16 hours total to get Caves of Qud booting under Godot", but that figure is the author's paraphrase, not a Bucklew quote. The quote for the motive: "Unity's pricing scheme spits in the face of a community that has invested a truly unfathomable amount of time". **VERIFIED** (quotes and paraphrase distinguished). https://fullcleared.com/news/caves-of-qud-developer-live-tweets-porting-game-to-godot . Note: this is a booting test, not a finished migration. The "full port likely around 1.0" line is the article's claim, not confirmed.
- **Vintage Story, Factorio, RimWorld, Zomboid, Songs of Syx, Stoneshard, Vintage Story:** engine claims NOT VERIFIED (no sources fetched).

## 2. Native-plugin ports of C++ games into Unity, and Unreal ports

- **Unity native plugin model.** Native plug-ins are unmanaged libraries called from C#; IL2CPP generates P/Invoke wrappers in C++. **SNIPPET.** https://docs.unity.com/en-us/engine/6000.7/manual/programming-environment/plug-ins/native.md ; https://unity.com/de/blog/engine-platform/il2cpp-internals-pinvoke-wrappers
- **Reported pain points (Unity C++ plugins).** Jackson Dunstan: C++ call stacks do not show in the Unity profiler; C++ crashes can take down the editor; C# glue code and per-platform setup needed. **SNIPPET** (search text). https://www.jacksondunstan.com/articles/3938
- **Exceptions must not cross managed/unmanaged boundary; native plug-ins are not unloaded, which hinders iteration.** Attributed to an Audiokinetic talk. **SNIPPET.** Source URL not fetched.
- **Marshalling structs/arrays and 64-bit requirement for the Editor.** From a minimal GitHub example (cschladetsch/MinimalUnity3dCppPlugin). **SNIPPET.** https://github.com/cschladetsch/MinimalUnity3dCppPlugin
- **OpenRA, Wesnoth Unity ports.** No source found. **NOT FOUND.**
- **Unreal ports of C++ codebases, pain points.** Not searched in depth. **NOT VERIFIED.**
- **Third-party TTS plugins for Unreal/Unity (ReadSpeaker).** Vendor marketing claim that it adds screen narration. **SNIPPET, vendor claim, not verified in shipped games.** https://www.readspeaker.com/resources/unity/

## 3. Roguelikes and turn-based games with screen-reader support

- **Caves of Qud.** No screen reader found. Official-looking devlog sidebar tags: "Configurable controls" and "High-contrast". **VERIFIED** (tags only; the Feature Friday posts for July 2024 mention no screen reader). https://freeholdgames.itch.io/cavesofqud/devlog/764265/feature-friday-july-12-2024 . Steam page fetch gave no accessibility feature list. **VERIFIED** (absence in fetched text only).
- **Shattered Pixel Dungeon (libGDX).** Itch.io accessibility tag "Configurable controls" in search result text only. No screen-reader support found. Engine libGDX from a linuxphoneapps listing. **SNIPPET.** https://linuxphoneapps.org/apps/com.shatteredpixeldungeon/ . The official site fetch failed (DNS error), so the official site was not checked.
- **Dwarf Fortress.** No documented screen reader support found. A Steam community thread asks whether it is playable blind; a commenter says the UI is accessible (anecdote). **SNIPPET / NOT VERIFIED** as a feature.
- **Cogmind.** No information found. **NOT FOUND.**
- **Jupiter Hell, Brogue.** Not searched. **NOT VERIFIED.**
- **Academic approach for ASCII roguelikes.** A 2020 gamnlp paper (aclanthology 2020.gamnlp-1.9) generates text descriptions of game events for screen readers to read aloud. **SNIPPET.** https://aclanthology.org/2020.gamnlp-1.9

## 4. Effort numbers and reasons for abandoning migrations

- **Caves of Qud Godot booting test: about 16 hours** (author paraphrase, see section 1). Not a full migration. **VERIFIED as reported; NOT VERIFIED as a precise figure.**
- **RollerCoaster Tycoon Classic:** a forum post quoting the original developer says porting from machine code to C++ took much longer than the original writing. **SNIPPET, NOT VERIFIED** (no fetched source).
- **Reasons for leaving an engine:** the only reason found is Unity's pricing change, cited by Bucklew (see section 1). **VERIFIED.**
- **Other reasons, timelines for Unity/Godot migrations:** NOT FOUND in this session. No postmortem with weeks/months of effort was located.
- **Cyberix3D rewrite (C++ with Ogre):** a shipped rewrite whose main constraint was backward compatibility of user-made content. **SNIPPET.** https://gamedev.net/news/game-highlight-cyberix3d-r5113/

## 5. Accessibility outcomes: did engine choice deliver screen-reader support?

**Engine-level facts:**
- **Godot 4.5** adds screen-reader support through AccessKit to Control nodes; experimental; editor support limited to Project Manager, standard UI nodes and the inspector; follow-ups promised. **VERIFIED.** https://godotengine.org/releases/4.5/
- **Unreal Engine** ships Text To Speech, Screen Reader and Slate Screen Reader plugins; the TTS plugin supports English only on all platforms. **VERIFIED.** https://dev.epicgames.com/documentation/en-us/unreal-engine/blind-accessibility-features-overview-in-unreal-engine
- **Unreal and Godot before 4.3 "do not do this out of the box" (claim about Unity too).** Snippet from a search result; no fetched support. **NOT VERIFIED.**
- **Godot NVDA Integration GDExtension** exists in the Godot asset library (sends text to NVDA, falls back to TTS). **SNIPPET.** https://godotengine.org/asset-library/asset/edit/10637 (the link is an edit page, not the asset page).

**Shipped narrated games (verified where stated):**

| Game | Engine | Built-in narration / screen reader | Status | Source |
|---|---|---|---|---|
| The Last of Us Part II | NOT VERIFIED (not identified in session) | Menus and on-screen text voiced via Text-To-Speech; auto-enabled when language is English and console TTS is on; available in all languages except Thai | **VERIFIED** (PlayStation page) | https://playstation.com/en-th/games/the-last-of-us-part-ii/accessibility |
| Diablo IV | NOT VERIFIED (custom engine claim not checked) | Built-in screen reader, supports JAWS and NVDA; reads in-game menu text; voice type, speed, volume configurable | **VERIFIED** (Blizzard news) | https://news.blizzard.com/diablo4/23954932 |
| Sea of Thieves | Unreal Engine 4 | "Let Games Read to Me" narrates cosmetic chests, Shop menus, Pirate Log, barrel contents, etc.; Xbox screen reader for menus; toast narration | Narration **VERIFIED** (caniplaythat, 2021); engine **SNIPPET** (Wikipedia/search text) | https://caniplaythat.com/2021/06/22/sea-of-thieves-season-3s-first-accessibility-features-revealed/ ; https://en.wikipedia.org/wiki/Sea_of_Thieves |
| Mortal Kombat 1 | Unreal Engine 4 (customized, per NetherRealm/Ed Boon via press) | Screen reader narrates menus; per Inverse, reads any on-screen text; Descriptive Audio | Engine **SNIPPET**; narration **SNIPPET** (support page returned HTTP 403, so not fetched) | https://www.inverse.com/gaming/mortal-kombat-1-accessibility ; https://multiplayer.it/notizie/mortal-kombat-1-non-gira-unreal-engine-5-ed-boon-non-esclude-injustice-3-in-futuro.html |
| Fortnite | Unreal (NOT VERIFIED in session) | Epic help page: chat text-to-speech ("Read Messages Aloud"), English only. No full menu screen reader found | Chat TTS **SNIPPET**; full screen reader **NOT VERIFIED / not found** | https://www.epicgames.com/help/epic-games-store-c-202300000001639/launcher-support-c-202300000001735/how-do-i-enable-text-to-speech-tts-a202300000083792 |
| Hearthstone, Stardew Valley, Minecraft | NOT VERIFIED | NOT VERIFIED | NOT VERIFIED | none |
| Any shipped Godot game with narration | Godot | none found | **NOT FOUND** (Steam search for "Godot + narrated menus" not completed) | none |
| Caves of Qud, Dwarf Fortress, Shattered Pixel Dungeon, Cogmind | C#/custom, SDL, libGDX, NOT VERIFIED | No screen reader found | **NOT FOUND** (see section 3) | see section 3 |

**Conclusion (verified part only):** the three shipped titles with confirmed narration are on custom or Unreal stacks, not Godot. Engine choice did not by itself deliver narration: the narration in Sea of Thieves and MK1 is per-game work (each screen's reading order, per the Inverse article for MK1), and Godot's own support is experimental and Control-node only.

---

## Could not verify

- Cataclysm-DDA / Cataclysm-BN Godot frontend: none found.
- Caves of Qud accessibility roadmap and any screen reader: no source; Freehold devlogs checked only.
- Shattered Pixel Dungeon and Cogmind accessibility details; official sites not reached (DNS error for shattered-pixel.com).
- Unity and Unreal ports of named C++ games (OpenRA, Wesnoth): not found.
- Effort numbers (weeks/months) for engine migrations of shipped games: not found beyond the 16-hour Caves of Qud booting test (author paraphrase).
- Engines and narration for Hearthstone, Stardew, Minecraft, Forza, Fortnite menus, Factorio, RimWorld, Vintage Story, Songs of Syx, Stoneshard, Zomboid, Brogue, Jupiter Hell: not searched or not found.
- Mortal Kombat 1 support page content (HTTP 403 on fetch).
- Dwarf Fortress "own engine, not Unity" claim: the fetched article does not address it.
- Godot 4.3+ NVDA/Braille/third-party screen reader assets: only a snippet.
- The GodotCon 2026 talk lessons-learned content: abstract only, recording not found.
- All SNIPPET-labelled items above need a fetch before being cited as fact.
