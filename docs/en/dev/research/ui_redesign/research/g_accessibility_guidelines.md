# Game accessibility guidelines and exemplary implementations

Research note for the UI redesign. Sources were fetched on 2026-10-21. Items marked **UNVERIFIED** come from memory or from a source I could not open. Items under "Could not verify" were not confirmed.

## 1. Checklist table

Guideline wording is verbatim from gameaccessibilityguidelines.com (GAG) unless stated. Level and category come from the page where the item appears. Some items appear under several categories on the source site; they are listed once per primary category.

| # | Item | Category | Level (GAG) | Source URL |
|---|---|---|---|---|
| 1 | Include an option to adjust the game speed | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 2 | Include toggle/slider for any haptics | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 3 | Ensure interactive elements / virtual controls are large and well spaced, particularly on small or touch screens | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 4 | Include an option to adjust the sensitivity of controls | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 5 | Ensure that all areas of the user interface can be accessed using the same input method as the gameplay | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 6 | Ensure controls are as simple as possible, or provide a simpler alternative | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 7 | Allow controls to be remapped / reconfigured | motor | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 8 | Avoid flickering images and repetitive patterns | cognitive | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 9 | Allow players to progress through text prompts at their own pace | cognitive | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 10 | Include interactive tutorials | cognitive | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 11 | Use simple clear language | cognitive | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 12 | Allow the game to be started without the need to navigate through multiple levels of menus | cognitive | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 13 | Use simple clear text formatting | cognitive / vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 14 | Use an easily readable default font size | cognitive / vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 15 | Provide high contrast between text/UI and background | vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 16 | Avoid VR simulation sickness triggers | vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 17 | If the game uses field of view (3D engine only), set an appropriate default for the expected viewing environment | vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 18 | Ensure no essential information is conveyed by a fixed colour alone | vision | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 19 | If any subtitles / captions are used, present them in a clear, easy to read way | hearing | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 20 | Ensure no essential information is conveyed by sounds alone | hearing | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 21 | Provide separate volume controls or mutes for effects, speech and background / music | hearing | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 22 | Provide subtitles for all important speech | hearing | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 23 | Ensure that speech input is not required, and included only as a supplementary / alternative input method | speech | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 24 | Solicit accessibility feedback | general | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 25 | Ensure that all settings are saved/remembered | general | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 26 | Provide details of accessibility features in-game | general | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 27 | Provide details of accessibility features on packaging and/or website | general | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 28 | Offer a wide choice of difficulty levels | general | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 29 | Avoid / provide alternatives to requiring buttons to be held down | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 30 | Provide a macro system | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 31 | Allow interfaces to be resized | motor / vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 32 | Allow interfaces to be rearranged | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 33 | If producing a PC game, support windowed mode for compatibility with overlaid virtual keyboards | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 34 | Avoid repeated inputs (button-mashing/quick time events) | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 35 | Make interactive elements that require accuracy (eg. cursor/touch controlled menu options) stationary | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 36 | Support more than one input device | motor | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 37 | Highlight important words | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 38 | Provide a choice of text colour, low/high contrast choice as a minimum | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 39 | Ensure no essential information (especially instructions) is conveyed by text alone, reinforce with visuals and/or speech | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 40 | Include a means of practicing without failure, such as a practice level or sandbox mode | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 41 | Indicate / allow reminder of controls during gameplay | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 42 | Indicate / allow reminder of current objectives during gameplay | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 43 | Include contextual in-game help / guidance / tips | cognitive | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 44 | Give a clear indication that interactive elements are interactive | cognitive / vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 45 | Provide an option to turn off / hide background movement | cognitive / vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 46 | Provide an option to adjust contrast | vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 47 | Provide a choice of cursor / crosshair colours / designs | vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 48 | Ensure sound / music choices for each key objects / events are distinct from each other | cognitive / vision | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 49 | Allow subtitle/caption presentation to be customised | hearing | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 50 | Provide a visual indication of who is currently speaking | hearing | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 51 | Provide captions or visuals for significant background sounds | hearing | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 52 | Ensure that all important supplementary information (eg. the direction you are being shot from) conveyed by audio is... (truncated on source) | hearing | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 53 | Provide a stereo/mono toggle | hearing | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 54 | Support text chat as well as voice for multiplayer | hearing / speech | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 55 | Base speech recognition on individual words from a small vocabulary (eg. 'yes' 'no' 'open') instead of long phrases... (truncated on source) | speech | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 56 | Include assist modes such as auto-aim and assisted steering | general | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 57 | Provide an autosave feature | general | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 58 | Offer a means to bypass gameplay elements that aren't part of the core mechanic, via settings or in-game skip option | general | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 59 | Allow difficulty level to be altered during gameplay, either through settings or adaptive difficulty | general | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 60 | Include some people with impairments amongst play-testing participants | general | Intermediate | https://gameaccessibilityguidelines.com/intermediate/ |
| 61 | Provide very simple control schemes that are compatible with assistive technology devices, such as switch or eye tracking | motor | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 62 | Include a cool-down period (post acceptance delay) of 0.5 seconds between inputs | motor | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 63 | Do not make precise timing essential to gameplay, offer alternatives (truncated on source) | motor | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 64 | Allow play in both landscape and portrait | motor | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 65 | Provide an option to turn off / hide all non interactive elements | cognitive | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 66 | Allow all narrative and instructions to be replayed | cognitive | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 67 | Avoid any sudden unexpected movement or events | cognitive | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 68 | Provide an option to disable blood and gore | cognitive | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 69 | Provide pre-recorded voiceovers for all text, including menus and installers | cognitive / vision | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 70 | Ensure screen reader support, including menus & installers | vision | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 71 | Allow the font size to be adjusted | vision | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 72 | Provide an audio description track | vision | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 73 | Provide signing | hearing | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 74 | Ensure that subtitles/captions are cut down to and presented at an appropriate words-per-minute for the target age-group | hearing | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 75 | Realtime text <-> speech transcription | general / speech | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 76 | Allow settings to be saved to different profiles, at either game or platform level | general | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 77 | Base speech recognition on hitting a volume threshold (eg. 50%) instead of words | speech | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 78 | Use symbol-based chat (smileys etc) | speech / hearing | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 79 | Include every relevant category of impairment (motor, cognitive etc) amongst play-testing participants, in representative proportions (truncated on source) | general | Advanced | https://gameaccessibilityguidelines.com/advanced/ |
| 80 | Xbox XAG 118: Photosensitivity (guideline page exists; body not fetched) | visual | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xbox-accessibility-guidelines |
| 81 | Xbox XAG 101: Text display (goal: minimum default sizes, spacing, configurable style and colour) | visual | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xag-deep-dives/xag-101-text-display.md |
| 82 | Xbox XAG 102: Contrast | visual | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xag-deep-dives/xag-102-contrast.md |
| 83 | Xbox XAG 103 Additional cues; 104 Subtitles and captions; 105 Audio accessibility; 106 Screen narration; 111 Audio description | audio | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xbox-accessibility-guidelines |
| 84 | Xbox XAG 107 Input; 110 Haptic feedback; 109 Object clarity; 112 UI navigation; 113 UI focus handling; 114 UI context; 117 Visual distractions and motion | input / visual | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xbox-accessibility-guidelines |
| 85 | Xbox XAG 108 Difficulty options; 115 Error messages; 116 Time limits; 119 Chat (STT/TTS); 120 Communication; 121 Accessible feature documentation; 122 Accessible customer support; 123 Mental health best practices | difficulty / communication / docs | XAG | https://devdocs.xbox.com/build/game-principles/accessibility/xbox-accessibility-guidelines |
| 86 | PlayStation Accessibility Tags: more than 50 tags across six categories (Visual, Audio, Subtitles and captions, Controls, Gameplay, Online communication) | platform tag | Sony | https://stevivor.com/news/accessibility-tags-added-to-playstation-store (secondary; primary: https://blog.playstation.com/?p=377775) |
| 87 | Accessible Games Initiative tag example: "Playable without Touch Controls" ("Lets you play without using touchpads or touchscreens"), category Input Features | platform tag | AGI | https://accessiblegames.com/ |
| 88 | Can I Play That: reviews cover settings-menu features, subtitle legibility, screen reader support; criteria-specific reviews are dormant | review criteria | CIPT | https://caniplaythat.com/?p=1742 |
| 89 | Ensure no essential information is conveyed by a fixed colour alone (GAG Basic; mirrors item 18) | colour | Basic | https://gameaccessibilityguidelines.com/basic/ |
| 90 | Colour-vision redundancy rule: pair colour with shape, icon, label, pattern or position (secondary guidance, not a GAG quote) | colour | secondary | https://caniplaythat.com/?p=8897 |
| 91 | Photosensitivity: no more than three flashes in any one-second period (WCAG 2.2 SC 2.3.1) | photosensitivity | WCAG | https://www.w3.org/WAI/WCAG22/Understanding/three-flashes-or-below-threshold.html |
| 92 | Text contrast: 4.5:1 normal text, 3:1 large text (18pt / 14pt bold) (WCAG 2.2 SC 1.4.3) | contrast | WCAG | https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html |
| 93 | UI components and graphical objects: 3:1 against adjacent colours (WCAG 2.2 SC 1.4.11) | contrast | WCAG | https://www.w3.org/WAI/WCAG22/Understanding/non-text-contrast.html |

Counts from the GAG full-list page: 123 entries including repeats, about 106 unique guidelines. The fetched basic list gives 28 items; this table takes the lists as the fetch tool returned them. Some GAG wording was truncated by the fetch tool (marked "..."). Check the source page for full text before quoting.

## 2. Platform and reviewer criteria

### Game Accessibility Guidelines (GAG)
- Three levels: Basic ("Easy to implement, wide reaching and apply to almost all game mechanics"), Intermediate ("Require some planning and effort but still just good general game design"), Advanced ("Complex adaptations for profound impairments and specific niche mechanics"). Source: https://gameaccessibilityguidelines.com/
- The GAG homepage does not list items by category. Items are on /basic/, /intermediate/, /advanced/ and /full-list/.

### Xbox Accessibility Guidelines (XAGs)
- Version 3.2, published 8 June 2023. Each XAG has Goal, Overview, Scoping questions, Key areas to target, Implementation guidelines, Example content, Potential player impact, Resources. Source: https://learn.microsoft.com/en-us/gaming/accessibility/guidelines (also mirrored at https://devdocs.xbox.com/build/game-principles/accessibility/xbox-accessibility-guidelines).
- The XAGs are a "checklist to validate the accessibility of their titles" for test teams. The source says they are not a compliance checklist.
- Numbering (from devdocs index): XAG 101 to 123 (see checklist items 80 to 85). Titles of XAG 101 and 102 were verified from their deep-dive pages; other titles are from the index only.

### Sony / PlayStation Accessibility Tags
- More than 50 tags across six categories: Visual, Audio, Subtitles and captions, Controls, Gameplay, Online communication. Shown on PS Store game hubs (Triangle button on PS5). Rollout started April 2023. Sources: https://stevivor.com/news/accessibility-tags-added-to-playstation-store, https://www.gematsu.com/2023/04/playstation-store-for-ps5-adds-accessibility-tags. Current tag list not fetched (PlayStation store pages were not opened).

### Accessible Games Initiative tags (accessiblegames.com)
- One example tag verified: "Playable without Touch Controls" ("Lets you play without using touchpads or touchscreens"), in the Input Features category. The full tag list at https://accessiblegames.com/accessibility-tags/ did not load its content in the fetch (dynamic list). The site says the initiative was first proposed by Sony Interactive Entertainment among others.

### Can I Play That (CIPT)
- Current model is broad "Accessibility Reviews" that mix professional and lived experience. Criteria-specific reviews are dormant. No published numeric rubric found. Source: https://caniplaythat.com/?p=1742
- A 2026 DiGRA paper maps CIPT reviews (2019 to 2025) onto GAG: reviews foreground basic hearing, motor and visual provisions (subtitles, remapping, legible text); advanced guidelines largely absent. Source: https://dl.digra.org/index.php/dl/article/view/2833

### Game Accessibility Nexus
- Search did not find an organisation or database under this name. The closest result was Ubisoft's Assassin's Creed Nexus VR accessibility spotlight (motor, visual, auditory groupings, comfort presets). Not treated as the same resource. Could not verify the intended resource.

## 3. Per-game implementation notes

Each entry cites the source it came from. Where a feature was only in a secondary source, that is stated.

### The Last of Us Part II (PlayStation / Naughty Dog)
Source: https://www.playstation.com/en-us/games/the-last-of-us-part-ii/accessibility/ (fetched)
- **Presets:** Vision, Hearing, Motor. Each switches on a bundle; individual options can be changed after applying one.
- **Text-to-speech:** "Enables narration of on-screen text." Separate volume slider. Source as above.
- **High contrast:** listed as "High contrast display" under Magnification and visual aids: "Mutes environment colors and adds distinct contrast coloring to allies, enemies, items, and interactive objects."
- **Lock-on:** "Lock-on aim" (Assistance): "Automatically lock-on to enemy targets when aiming." Also Lock-on strength and Arc-throw lock-on.
- **Hold to toggle:** no option with that exact name. Hold/Toggle choice exists for Aiming mode, Listen mode, Sprinting, Crafting, Backpack weapon swap and Hold breath mode.
- **Navigation and traversal:** two options. "Navigation assistance" (pressing L3 faces the camera in the direction of story progression) and "Traversal assistance" ("Enables simplified traversal input.").
- **Audio cues:** Traversal cues, Combat audio cues, Combat vibration cues, Guitar vibration cues; Accessibility audio cues volume; Audio cue glossary.
- **Colour options:** HUD colorblind mode is listed under Magnification and visual aids; the page lists it and HUD flashing, HUD scale, HUD background, HUD colour.
- **Other:** Subtitles (size, background, names, name colours, direction, colour); Camera shake, Motion blur, Dolly zoom, Field of view, Persistent center dot under Motion sickness.
- Secondary (2020 Naughty Dog / PS blog text quoted in search results, NOT opened): the Vision preset includes TTS, High Contrast Display, HUD scale Large, Lock-On Auto-Target, Traversal and Combat Audio Cues. Naughty Dog blog returned a DNS error in this session. https://blog.playstation.com/2020/06/09/the-last-of-us-part-ii-accessibility-features-detailed

### Forza Horizon 5 (Playground Games / Xbox)
Sources: https://support.forza.net/hc/en-us/articles/4409280058259-Forza-Horizon-5-Accessibility-Support (returned HTTP 403, not read); https://news.xbox.com/en-us/2021/11/04/gaming-for-everyone-the-accessibility-features-of-forza-horizon-5/ (not opened); search summaries only.
- **Sign language:** ASL and BSL interpreter for cinematics, announced at launch as picture-in-picture. Shipping status **not verified** from the support page (blocked). Source: https://www.neowin.net/news/forza-horizon-5-is-gaining-an-on-screen-sign-language-interpreter-for-cutscenes/ (search summary).
- **Text size:** subtitle font size, background opacity, keyword highlighting; menu and gameplay font size customisation (per announcement summary). Source: search summary of the Xbox news post.
- **Narration:** screen narrator reads text, buttons and elements (announcement summary).

### Celeste (Maddy Makes Games)
Source: https://gameaccessibilityguidelines.com/celeste-assist-mode (fetched); developer name spelling conflicts across sources (noted in search results).
- **Assist Mode options (as named by GAG page):** Game speed, Infinite stamina, Air dashes, Invincibility. Dash assist is named in one secondary review (search summary, UNVERIFIED against the game).
- GAG links the example to: "Allow gameplay to be fine-tuned by exposing as many variables as possible"; "Include an option to adjust the game speed"; "Include assist modes such as auto-aim and assisted steering".
- Secondary review (newnormative.com, https://newnormative.com/2018/01/25/celestes-assist-mode-brings-welcome-accessibility-options) describes game speed going down to 50% in 10% steps. Not opened.

### Hades (Supergiant Games)
- **Could not verify** the official option list. One database (familygamingdatabase, https://www.familygamingdatabase.com/accessibility/Hades) reportedly says subtitles are on by default and their size cannot be adjusted. Conflicting page in search results. Not opened.
- Community mods (PC only): Erumi's Blind Accessibility Options (May 2021), audio cues for doors, lava damage removal. Source: https://caniplaythat.com/?p=5182 (search summary).

### God of War Ragnarok (Santa Monica Studio / Sony)
Source: https://www.playstation.com/en-us/games/god-of-war-ragnarok/accessibility/ (fetched).
- **UI:** "Icon size" (Small, Default, Large, X-Large); "UI Text Size" (Default, Large, X-Large, XX-Large); "Icon, subtitle and caption size" also present in some summaries (not on the fetched page).
- **High contrast:** "High Contrast HUD", "High Contrast Cinematics", "High Contrast Display Preset" (Off, General, Red-Green, Green-Red, Blue-Yellow, Collector, Danger, Lighting). Also "Visualization contrast" (Light, Medium, Dark). Also individual colour selectors for ten categories (Kratos, Atreus, Boss/Miniboss, Enemy, NPC, Target, Interact, Hazard, Traversal markings, Background).
- **Colour-blind modes:** "UI color correction" (Default, Set 1, Set 2, Set 3); "Color filter" (Off, Red-Green, Green-Red, Blue-Yellow), described on the page as Daltonization. Search summaries said no colour-blind mode found; the fetched page contradicts that, so the search summary is treated as wrong.
- **Audio cues:** "Audio Cues" and "Screen Reader for select key menus (English only)"; "Audio Cue glossary"; Accessibility audio cues volume.
- **Subtitles:** size (Default, Large, X-Large), background (None, Light, Dark), speaker names, speaker colour, direction indicators.
- **Presets:** Vision, Hearing, Motion reduction, Motor (each Off/some/full).

### Spider-Man 2 (Insomniac Games / Sony)
Source: Insomniac support article https://support.insomniac.games/hc/en-us/articles/46730041467027-What-Accessibility-options-does-Marvel-s-Spider-Man-2-feature (returned via search only; not opened, the page is the authoritative list). Summaries from search results (UNVERIFIED against the page):
- Game speed (30%, 50%, 70% of real time), adjustable enemy health/damage, stealth detection speed.
- Chase Assist; solid-colour shaders for throwables and collectibles; high-contrast background and Spider-Sense; high-contrast Perfect Dodge effect.
- Frequency filters for audio (high, low or custom range); screen reader for on-screen menu text; on-screen captions and audio descriptions for cinematics.
- Button-hold alternatives for aiming, swinging and parkour; Web-Shooter Burst; remapping and axis inversion.
- Family Gaming Database counts 41 features. Source: https://www.familygamingdatabase.com/accessibility/Marvels+Spider-Man+2

### Sea of Thieves (Rare / Xbox)
Sources: community databases and patch-note coverage (search results only):
- **Menu narration:** supports Xbox screen reader when navigating menus; "Let Games Read to Me" option extended in Season 3 to cosmetic chests, Shop menus, Pirate Log Season and Reputation sections, Trading Company menus.
- **Toast narration:** "Narrate Toast Messages" option in update 2.0.15.1 (https://www.gamewatcher.com/sea-of-thieves-update-2-0-15-1-patch-notes).
- **Toast duration** setting and opaque menu backgrounds (Season 3).
- **TTS/STT for chat:** platform-tied, reported as limited to US-configured systems at review time (caveat: may be out of date).
- Source: https://caniplaythat.com/?p=3000 and https://www.familygamingdatabase.com/accessibility/Sea+of+Thieves

### Diablo IV (Blizzard)
Sources: https://caniplaythat.com/2023/05/23/diablo-iv-accessibility-detailed-by-blizzard/ and https://news.blizzard.com/diablo4/23954932 (search summaries; not opened).
- Built-in screen reader that reads in-game menu text; works with JAWS/NVDA (per one source). Adjustable volume, speed and voice.
- Speech-to-text for chat (microphone input).
- Adjustable text and cursor sizes; gear audio cues tied to rarity.
- Blizzard claimed over 50 accessibility features at 2023 launch.
- Status as of 2026 **UNVERIFIED**.

### Overwatch 2 (Blizzard)
Sources: https://www.familygamingdatabase.com/accessibility/Overwatch+2 (27 features, count only); https://www.slashgear.com/overwatch-gets-updated-colorblind-feature-with-nine-color-options-22546973 and Blizzard forum threads (search summaries).
- **Colour-blind options:** nine colour-blind-friendly colours for nameplates, HUD, health bars, hero outlines; friendly and enemy colours separate; under Video > Color Blind Options. A forum user reported ability effects (Tracer's red blink) not covered.
- **Subtitles:** text scalability, portrait icons, speaker name, text colour, background colour, preview.
- **Cursor size** (PC) and custom Group/Alert colours.
- **Narration:** none found in search results. Status **UNVERIFIED**.

## 4. Colour-blindness

### 4.1 Prevalence (congenital CVD)
Source: 2025 systematic review and meta-analysis of children and adolescents, 56 studies, Ophthalmology, via https://pubmed.ncbi.nlm.nih.gov/40769301/ (search summary; abstract figures quoted from the search result, primary page not opened).

| Type | Males | Females |
|---|---|---|
| Deutan | 3.66% (95% CI 3.02 to 4.44) | 0.46% (95% CI 0.30 to 0.69) |
| Protan | 1.54% (95% CI 1.30 to 1.82) | 0.30% (95% CI 0.21 to 0.43) |
| Tritan | 0.67% (95% CI 0.20 to 2.22) | not reported in the summary |

- Global congenital CVD estimate: about 2.59%.
- The review notes reliance on the Ishihara test may underestimate tritan prevalence.
- Older reference figure for tritan (search summary, source not identified): 1 in 15,000 to 1 in 50,000. Treat as **UNVERIFIED**.
- Population figures for adults in the general population were not gathered. Commonly quoted figures (about 8% of men, 0.5% of women, for red-green) are **UNVERIFIED** memory-only claims.

### 4.2 Daltonisation vs designed palettes
- Simulation (Machado 2009, Viénot 1999, Brettel 1997) is a model of the colour view. It is not a fix. Source: https://pypi.org/project/daltonlens/0.1.2/ (tool lists the models).
- Daltonisation filters shift colours to separate confusable hues. They distort colours, can introduce false distinctions, and may not improve performance. Simon-Liedtke, Farup and Laeng state it "remains unclear" whether these methods improve performance (summary from search result; primary paper not opened). Trade-off between contrast and naturalness: source as search result on daltonisation.
- Practical conclusion: ship designed palettes plus redundancy (section 4.4), and let the player choose colours for the important elements. GAG lists "Provide a choice of cursor / crosshair colours / designs" (item 47). The Overwatch and God of War entries show palette presets; GoW's Daltonization is a filter.
- A QA guide notes that a single alternative palette often covers only the red-green case; test against each deficiency type. Source: https://bugnet.io/blog/qa-testing-for-colorblind-modes (search summary).

### 4.3 Safe palettes with hex values

**Okabe-Ito (8 colours)** - Okabe and Ito, Color Universal Design, 2008; popularised by Bang Wong, "Points of view: Color blindness", Nature Methods 8(6):441, 2011, doi:10.1038/nmeth.1618 (citation from red-bean.com BibTeX entry, search result).

| Name | Hex |
|---|---|
| Orange | #E69F00 |
| Sky Blue | #56B4E9 |
| Bluish Green | #009E73 |
| Yellow | #F0E442 |
| Blue | #0072B2 |
| Vermillion | #D55E00 |
| Reddish Purple | #CC79A7 |
| Black | #000000 |

Source for hex values: https://conceptviz.app/blog/okabe-ito-palette-hex-codes-complete-reference (secondary). The primary page https://jfly.uni-koeln.de/color/ returned HTTP 403, so these hex values are **not confirmed against the primary source**. Check the primary page before citing.

**Paul Tol (SRON colour schemes page)** - source https://sronpersonalpages.nl/~pault/ (fetched).

| Scheme | Hex codes | Page note |
|---|---|---|
| Bright | #4477AA #EE6677 #228833 #CCBB44 #66CCEE #AA3377 #BBBBBB | "colour-blind safe" |
| Vibrant | #EE7733 #0077BB #33BBEE #EE3377 #CC3311 #009988 #BBBBBB | "equally colour-blind safe" (alt. to Bright) |
| High-contrast | #004488 #DDAA33 #BB5566 | "colour-blind safe and optimized for contrast" |
| Muted | #CC6677 #332288 #DDCC77 #117733 #88CCEE #882255 #44AA99 #999933 #AA4499 (bad data #DDDDDD) | "equally colour-blind safe with more colours, but lacking a clear red or medium blue" |
| Medium-contrast | #6699CC #004488 #EECC66 #994455 #997700 #EE99AA | "colour-blind safe with more colours" |
| Dark | #4477BB #CC4466 #228833 #2255AA #993366 #006644 #117788 #882288 #BB4488 #775500 #444444 | "only some cost for people who are colour blind" |
| Light | #77AADD #EE8866 #EEDD88 #FFAABB #99DDFF #44BB99 #BBCC33 #AAAA00 #DDDDDD | "reasonably distinct in both normal and colour-blind vision" |
| Pale | #AACCEE #CCEEFF #BBDDBB #EEEEBB #FFBBCC #EEBBDD #DDDDDD | "not very distinct in either normal or colour-blind vision" |
| Sunset (diverging) | #364B9A #4A7BB7 #6EA6CD #98CAE1 #C2E4EF #EAECCC #FEDA8B #FDB366 #F67E4B #DD3D2D #A50026 (bad data #FFFFFF) | "also works in colour-blind vision" |
| Nightfall (diverging) | #125A56 #00767B #238F9D #42A7C6 #60BCE9 #9DCCEF #C6DBED #DEE6E7 #ECEADA #F0E6B2 #F9D576 #FFB954 #FD9A44 #F57634 #E94C1F #D11807 #A01813 (bad data #FFFFFF) | "also works in colour-blind vision" |

Tol's discrete rainbow (23-colour variant) and smooth rainbow exist on the same page; the page says the discrete scheme "should not be interpolated" to stay colour-blind safe.

**IBM Carbon data-visualisation categorical palette** - https://carbondesignsystem.com/data-visualization/color-palettes/ (fetched). Order is curated for neighbour contrast; the page does not claim colour-blind safety.

| Order | Name | Hex |
|---|---|---|
| 1 | Purple 70 | #6929c4 |
| 2 | Cyan 50 | #1192e8 |
| 3 | Teal 70 | #005d5d |
| 4 | Magenta 70 | #9f1853 |
| 5 | Red 50 | #fa4d56 |
| 6 | Red 90 | #570408 |
| 7 | Green 60 | #198038 |
| 8 | Blue 80 | #002d9c |
| 9 | Magenta 50 | #ee538b |
| 10 | Yellow 50 | #b28600 |
| 11 | Teal 50 | #009d9a |
| 12 | Cyan 90 | #012749 |
| 13 | Orange 70 | #8a3800 |
| 14 | Purple 50 | #a56eff |

**Wong 2011** - the palette is the Okabe-Ito set above (Wong's article is the citation). Primary text not fetched.

### 4.4 Redundancy rules
- Never use colour alone for essential information (GAG Basic item 18; XAG 102 contrast and related XAGs). Add an icon, a label, a pattern, or a position.
- Double-coding (colour plus shape/label) is the standard recommendation. Source: search summary of gamedev and CIPT colour guidance (https://caniplaythat.com/?p=8897; https://gamedev.net/blogs/entry/2260691-the-problem-with-color).
- Common failures named in sources: status effects shown only by tint; team colours without another marker; map markers separated only by hue; damage numbers distinguished only by colour.
- Avoid red/green, blue/green and yellow/purple pairs for critical differences (university guide, search summary: https://sc.edu/about/offices_and_divisions/digital-accessibility/toolbox/best_practices/color_usage/).

## 5. Contrast and tools

- **Text:** WCAG 2.2 SC 1.4.3 requires 4.5:1 for normal text and 3:1 for large text. Large text is at least 18pt, or 14pt bold (about 24px and 18.5px). Ratios are not rounded: 4.499:1 fails. Exceptions: incidental text, pure decoration, logotypes, and text in inactive UI components. Source: https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html
- **UI components and graphics:** WCAG 2.2 SC 1.4.11 requires 3:1 against adjacent colours. Source: https://www.w3.org/WAI/WCAG22/Understanding/non-text-contrast.html
- **Tools:** the search did not verify specific checker tools. Commonly used checkers (WebAIM Contrast Checker, Colour Contrast Analyser by TPGi, Figma/Stark plug-ins) are **UNVERIFIED** memory-only mentions. Carbon's accessibility guidance says the system meets WCAG AA contrast in its themes (search summary, https://carbondesignsystem.com/guidelines/accessibility/color).
- Carbon also notes tokens apply across themes while hex values are theme-specific.

## 6. Photosensitivity and reduced motion

### Flash thresholds (WCAG 2.2 SC 2.3.1, Harding/ITC basis)
Source: https://www.w3.org/WAI/WCAG22/Understanding/three-flashes-or-below-threshold.html (fetched)
- Pass by having no more than **three flashes in any one-second period** (general or red flashes).
- Or pass by having a flashing area that stays within the **.006 steradians in any 10-degree visual field** (25% of a 10-degree field). The glossary gives a reference of 341 x 256 px at 1024 x 768, which the Intent text converts to **87,296 CSS pixels**.
- **General flash:** a pair of opposing luminance changes of 10% or more of maximum relative luminance (1.0), where the darker image is below 0.80 relative luminance.
- **Red flash:** a pair of opposing transitions involving saturated red (R/(R+G+B) at least 0.8, and a chromaticity difference above 0.2 in CIE 1976 UCS).
- Fine, balanced patterns (e.g. checkerboards with squares under 0.1 degree) are exempt.
- Intent note: industry definition for non-sRGB or HDR uses a change of 20 cd/m2 or more where the darker image is below 160 cd/m2 (ITU-R BT.1702).

### Harding FPA
- Harding FPA is a UK-developed system that analyses video frame by frame for flashes, patterns and fast cuts (search summary of news and secondary sources).
- The 3-per-second and 25%-area figures are cited from an Epilepsy Society summary and an Ofcom ruling summary (search result: https://epilepsysociety.org.uk/node/4424). The current Ofcom Broadcasting Code text was **not** retrieved; treat these as UNVERIFIED for broadcast regulation.

### Reduced motion (guidance)
- GAG Basic items 8 and 45 and Advanced items 66 and 67 cover flicker, background movement and sudden events.
- PlayStation presets (TLOU2, God of War, Spider-Man 2) include camera shake, motion blur, camera sway, persistent centre dot, cinematic smoothing, and film grain options (see per-game notes).
- VR comfort (GAG Basic item 16): Ubisoft's AC Nexus lists dynamic vignettes and a comfort preset set (search summary, news.ubisoft.com).

## Could not verify

- **Game Accessibility Nexus:** no resource of that name found in search.
- **Full XAG 101 to 123 titles:** only 101, 102, 109, 112, 113, 114, 117, 118 (index), 103 to 106, 107, 108, 110, 111, 115, 116, 119 to 123 from the devdocs index. Full guideline text not read.
- **Sony tag list (current):** not fetched. The rollout details are from 2023 news.
- **Accessible Games Initiative full tag list:** the page content did not load. Only one tag is verified.
- **Can I Play That scoring rubric:** none published in the sources found.
- **Hades official options:** conflicting; not confirmed. Colour-blind options not confirmed.
- **Forza Horizon 5 sign language shipping status and option names:** the support page returned 403; only announcement summaries were available.
- **Spider-Man 2 option names:** from search summaries only; the Insomniac support page was not opened.
- **Sea of Thieves, Diablo IV, Overwatch 2 current options:** from 2021 to 2023 summaries; not checked against current patches.
- **Naughty Dog TLOU2 blog:** DNS failure; Vision preset details taken from search summary only.
- **Okabe-Ito hex values from primary source:** primary page returned 403. Hex values come from a secondary blog.
- **Wong 2011 article text:** not read.
- **Daltonisation evidence (Simon-Liedtke, Farup, Laeng; Jefferson and Harvey 2006):** only summary-level evidence; Jefferson and Harvey not found.
- **Adult CVD prevalence (general population):** not gathered with a source. Memory figures are UNVERIFIED.
- **Accessibility checker tools list:** only Carbon is linked; others UNVERIFIED.
- **Ofcom Broadcasting Code flash rules:** not retrieved.
- **GAG item wording:** several items were truncated by the fetch tool; full text unverified for items 52, 55, 63, 79.
