# 02. Patterns from comparable games

What other games do, whether it fits Bright Nights 3D, and the evidence grade.
Raw findings with URLs are in `research/`. Grades: **V** = the research agent fetched the
page and I spot-checked it or the claim is corroborated by two sources; **S** = search
snippet only; **U** = unverified or from memory. Do not cite S/U items as researched facts.

Fit column: **Adopt**, **Adapt** (fits with a stated change), **Reject** (does not fit, with
the reason). "Turn-based" matters: BN has no real time, so any pattern that relies on
timing (fading HUDs, hold-progress rings tied to seconds) has to be re-based on turns or
on wall-clock while waiting for input.

## HUD and in-world presentation

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Contextual HUD with a persistent-mode option | The Long Dark (V: gamedeveloper.com 2021; Steam thread 2016-2023) | Need icons appear only when something needs attention; health is the one persistent element. Players split; the asked-for fix was a sensitivity/on-off option, not the old HUD. | Adapt | Default to contextual badges (already shipped as status badges) but ship the "always show vitals" switch on day one. Research e, p. |
| Icon-first status, numbers on demand | The Long Dark (V), Project Zomboid moodles (V: Dec 2024 Steam thread) | Bars/icons answer "high or low"; numbers live in a detail view. Zomboid's B42 icons were called "basically unreadable at first glance" (Alabeo, 22 Dec 2024). | Adapt | Icons need a word next to them on first sight and a tooltip always. We cannot rely on hover (controller). Every badge gets a short label; the number is in the character sheet and the F4 report. |
| Trend arrow next to a need | The Long Dark critique (V), DayZ 2018 thread (V) | Players asked for rising/falling cues to avoid "surprise deaths". | Adopt | Turn-based: compute the trend from the last N turns of the value. Cheap, high value, pure text-renderable (▲ ▼ =). |
| Red for damage, yellow for warning, size for priority | The Long Dark (V) | Colour and size encode urgency. | Adapt | Keep as a secondary channel only. Primary channel is icon shape plus label (colour-blind rule). |
| Stamina shown as risk, only when relevant | Dying Light 2 dev blog #9 (V) | Bar appears when draining or low; running out is costly. | Adopt | BN stamina already gates running/melee. Show the bar when below max or when a stamina-using action is queued. |
| HUD off but prompts and subtitles kept | RE4 (V), STALKER 2 (V), Days Gone mod complaint (S) | Full-HUD-off modes that also hide prompts were a problem. | Adopt | "HUD: full / minimal / off" where "off" still shows the interaction prompt and the heard-sound subtitle line. |
| Directional damage indicator | Generic (U), already shipped in r3d_hud.cpp | Edge glow toward the attacker. | Adopt (exists) | Add a non-colour form: a chevron shape at the edge, not just a glow, and a log line. |
| Compass tape with markers | Skyrim-style, Valheim, CDDA fork prior art (V in o) | One strip for direction: cardinal points, sun/moon, wind, mission target, notes. | Adopt (exists) | Threats stay off the tape (prior-art decision kept): edge arrows do threats. |
| Generic ping plus a few specific commands | Deep Rock Galactic (V: 2021 Steam thread, "The pointer works BECAUSE it's simple") | One contextual ping that means the obvious thing. | Adapt | Single-player; the equivalent is the one contextual interaction prompt. Supports the "one verb" interaction design. |
| Diegetic wrist/watch UI | The Division (V 2013), Metro Exodus (S) | Information in the world rather than framing it. | Reject as a replacement | Prior art (research o, L124-128) and gamedeveloper 2010 note diegetic devices are slower to read than a number. Keep sky/weather/wind diegetic (already shipped); keep vitals as HUD. |
| Hit markers | PC Gamer (headline V, argument S) | Can become the only cue players watch. | Adopt existing combat text; no extra marker | We already float damage numbers. |
| Fading HUD with no player control | Far Cry 2 critique (V, gamedeveloper 2010: "a complete lack of control") | Auto-hide that the player cannot stop. | Reject | Everything that hides must have an "always show" setting. |

## Interaction and prompts

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| One contextual interact verb with a prompt | Common in FPS/survival (U as a general pattern; prompt text verified in STALKER 2 complaint "the F menu dissapears", V) | Look at a thing, see "E: Open", press E. | Adopt | Replaces most of BN's dedicated verb keys (open, close, smash, examine, pick up, talk, board). The prompt shows the key glyph for the active device. |
| Hold-to-confirm with one radial fill | Unity docs (dated), general (S) | Long-press for destructive or slow actions. | Adapt | Only for a few actions (disassemble, drop all, butcher). Must be a toggle-able behaviour (hold vs press-twice) for motor accessibility. Turn-based means no real-time pressure; the hold is a confirmation, not a timer. |
| Menu that stays open while placing | Subnautica 2 (V, 2026 Substack), Subnautica 2018 complaint (V) | Build menu does not close after each placement. | Adopt | Construction: pick a piece once, place it repeatedly, Esc leaves. |
| Pick a thing from the world as the active build | Satisfactory (V: 2022 thread, "exactly, what I was looking for") | Point at a placed object to select its recipe. | Adopt | Construction: "copy this" on an existing furniture/terrain piece. |
| Double-click / single key moves item to pack | DayZ (V: 2019 thread) | Fast transfer from vicinity. | Adopt | Inventory: one key moves between panes; drag-and-drop is additive, never required. |
| Items silently routed to "hands" | DayZ (V: complaint) | Items land in hands instead of the pack. | Reject | Destination is always explicit: the focused pane. |

## Inventory

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Three-zone layout: Vicinity / Hands+Worn / Pack | DayZ wiki (V) | One screen for everything near and carried. | Adopt | Merges BN inventory, advanced inventory, pickup, drop, wear/wield/take off into one screen. |
| Sort by visible column, with a secondary key | StarUI (V, MP1st 2023), Fallout 76 sort bug post (V) | Click a column header; add a tiebreak key so equal rows do not reshuffle. | Adopt | Sort key shown in the header; stable secondary sort by name. |
| Filters on the main screen, not behind a button | BG3 complaint (S: "needs 2 mouse clicks") | Category tabs visible. | Adopt | Tabs across the top; bumpers switch them. |
| Category totals (combined mass per category) | StarUI (V) | Light stacks that add up become visible. | Adopt | Header shows weight and volume per category. |
| Junk tagging and "mark the amount you need" | Fallout 4 threads (V 2015) | Mark items as junk; target quantities for materials. | Adapt | Junk tag feeds auto-pickup/auto-drop rules. Target quantities become "pinned recipe" shortfall list (see crafting). |
| Favourites with numbered hotkey groups | SkyUI (S) | Several items per key. | Adapt | Hotbar of 8 slots bound to items or actions; no groups (budget). |
| Pinnable floating panels | Project Zomboid (V: pzfans) | Panels stay open when pinned. | Reject | Floating windows are mouse-only and fight controller focus. One fixed layout per screen. |
| Grid "tetris" inventory | DayZ (V), RE4 (S), Zomboid mods (S) | Spatial grid per container. | Reject | BN has volume/weight and nested pockets; a spatial grid is a game-rule change and a large UI. List with icons instead. |
| Tooltip latency | BG3 (V: Gfinity 2025 "tooltips take just long enough to load") | Slow tooltips slow the whole screen. | Adopt as a rule | Item info is built once per selection and cached; no hover delay on controller (the focused row's info pane is always up to date). |
| Compact list mode | StarUI/ArmadaUI (S) | Dense view for many items. | Adopt | Row density follows text scale; no separate "compact" option (budget). |

## Crafting and building

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Can-craft / missing / favourites / recent filters | Valheim AAA Build Menu mod (V) | First-class filters. | Adopt | Default filter is "can craft now"; "all known" and "missing one thing" are the others. |
| Pin a recipe; HUD shows the shortfall | Valheim Recipe Pinner mod (V), JEI Crafting Insight (S) | Track missing materials in real time. | Adopt | One pinned recipe shown as a small HUD line; the shortfall also feeds the "needed" tag in loot views. |
| Pull ingredients from nearby containers with a shown radius | Valheim HammerCraftingMaterials (V), Enshrouded magic chest gap (V) | Crafting sees nearby storage. | Adopt (exists in BN) | BN already crafts from adjacent tiles; show the radius and which tiles count. |
| Search must not reveal unknown recipes | Satisfactory thread (V) | Typed search returns only what you know. | Adopt | Unknown results appear as "unknown recipe (learn from books)" counts, not names. |
| Show the unlock action next to a locked entry | Valheim/Green Hell/Subnautica complaints (V/S) | Players could not tell why a piece was missing. | Adopt | Each hidden recipe category shows "needs: skill X 3, or book". |
| In-game handbook on one key | Vintage Story (V: wiki) | Reference from anywhere. | Adapt | BN's help ('?') becomes a searchable reference with recipe and item pages; also where the lessons live. |
| Recipe variants scrolled in place | Vintage Story (V) | "Any plank" shown once. | Adopt | BN's component alternatives render as one row with a cycle control. |
| Grid snap with an off switch | Subnautica 2 (V) | Snap by default. | Reject | BN builds per tile; there is no free placement. |
| Craft queue for controller users | Valheim thread (V: Rhapsody) | Queue repeated crafts. | Adopt (exists) | BN batch crafting already exists; expose "craft N / max" on the recipe row. |

## Controller, radial menus, Steam Deck

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Hold a bumper to open a radial; stick direction selects; release confirms | Factorio Switch quick panel (V: wiki), Project Zomboid (V: "V" key) | Fast access without menus. | Adopt | Two wheels of 8 (actions, quick slots). Keyboard: hold Tab / Q; mouse: hover slice. |
| Cap radials at 4 or 8 slices; page with the D-pad | Wikipedia pie menu (V), Factorio (V), The Long Dark 8-slot cap (S) | More than 8 fails. | Adopt | 8 slices, no nesting. Overflow goes to the full screen. |
| Two-stage trigger: light pull peeks, full pull opens | BG3 (V: PlayStation Blog 2023) | Peek at status. | Reject | SDL gamepads expose analog triggers but a two-stage gesture is fiddly and unmappable to keyboard. Use hold/release instead. |
| Stable radial layout | BG3 complaint (S: slots reorder themselves) | Slots never move on their own. | Adopt | Slots are explicit bindings; nothing auto-populates. |
| No virtual cursor on grids | Push Square 2019 (V) | D-pad focus movement, not a mouse emulator. | Adopt | Focus model with spatial navigation; a cursor mode exists only as a fallback for mod screens. |
| Default controller config reaches everything; glyphs match the device | Steamworks Deck compat (V) | Hard requirement for Deck Verified. | Adopt | `SDL_GetGamepadButtonLabel` (available in SDL 3.4.18 on this machine, header checked) drives glyphs; Steam Input is optional. |
| On-screen keyboard for text entry | Steamworks (V) | Deck review requirement. | Adopt | `SDL_StartTextInput` triggers the Deck keyboard; the game ships its own grid keyboard for non-Steam gamepads. |
| 9 px minimum, 12 px target glyph height at 1280x800 | Steamworks (V) | Legibility floor. | Adopt | Text-scale slider floor is computed from this. |

## Tooltips and information density

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Compare on demand against the same-slot equipped item | PoE (V), Diablo 4 (V: Den of Geek 2023) | Hold a key to compare. | Adopt | Inventory info pane shows deltas versus the worn item for the same body part when a key is held; on controller a toggle. Delta shown as sign plus arrow glyph, never colour alone. |
| Short tooltip, hold for detail, pin for long text | PoE 2 (S), NN/g progressive disclosure (V) | Summary first. | Adopt | Item info has a 6-line summary and an expandable full page. |
| Group stats by category, drop pointless headers | Factorio FFF-318 (V) | Root category without a name. | Adopt | Item info sections only when they contain more than one line. |
| Keep density, fix navigation | Dwarf Fortress Steam (V: PCGamesN 2020) | Scrollbars and tabs instead of cutting content. | Adopt | Dense screens get a scrollbar, a search field and tabs; no content is dropped for density alone. |
| Test at high UI scale | Factorio cropping (S) | Tooltips crop at 2x. | Adopt as a test rule | Screenshot tests at 100 % and 200 %. |

## Needs, health, morale, journal, map

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Reason list for mood | RimWorld (V: 2017 thread), CDDA Discourse (V: "I like to know why my toon is happy/sad") | Sum by source. | Adopt (exists) | BN's morale screen already lists sources; it becomes a tab of the character screen, no separate key. |
| Limb state as function loss with the threshold stated | Tarkov (V: Shacknews 2024), Kenshi (V) | "Blacked out" limb loses function. | Adapt | Body figure with per-limb fill; a broken limb shows the word "broken" and the effect ("cannot wield two-handed"). |
| Personal vs shared map markers | Valheim (V paraphrase) | Pin privacy. | Reject | Single-player; not needed. |
| Clear-all and a visible count for markers | Elden Ring (S) | Marker limit frustrations. | Adopt | Overmap notes: count shown, filter, delete-all-in-view. |
| Journal that gates actions | The Witcher 1 (V, 2007) | Knowledge unlocks contracts. | Reject | Game-rule change. Journal is a record only. |
| Challenge list instead of tutorial | 7DTD 1.0 (S) | Tracked goals. | Adapt | Guided onboarding lessons (shipped) are shown as a checklist in the journal; nothing else. |

## Onboarding and discoverability

| Pattern | Game (grade) | What it does | Fit | Why |
|---|---|---|---|---|
| Just-in-time hints that stay re-readable, auto-marked learned | RimWorld Learning helper (V: wiki), BG3 complaint about vanishing popups (S) | Hints when the mechanic is met; never timed out. | Adopt (exists) | Guided onboarding already does this; it moves into the journal's "Lessons" tab. |
| Searchable help on one key | DCSS '?' (S), Valheim Compendium (V) | One place to look things up. | Adopt | '?' opens the reference (actions, lessons, recipes, items). |
| Command palette (search for an action by name) | VS Code (U, not a game source) | Type what you want to do. | Adopt | One screen that lists every action with its current bindings and runs it. It is also the keybinding editor's search and the single-switch/one-handed path. |
| Skippable tutorial with a re-entry point | Deep Rock (S), Caves of Qud (V: GamingOnLinux 2024) | Short, optional, revisitable. | Adopt (exists) | Guided Play entry. |
| Surface required early features | Subnautica 2017 forum (V: "ready to quit") | Do not hide a required tool. | Adopt | The first lesson covers the interact prompt, the inventory screen and the F4 report. |

## Accessibility implementations

| Pattern | Game (grade) | What it does | Fit |
|---|---|---|---|
| Presets (vision / hearing / motor) that set many options at once | The Last of Us Part II (V: PlayStation page) | One choice on first launch. | Adopt: three presets in the Accessibility tab; each is a documented set of the same options. |
| Built-in narrator with Off / All / Chat / System modes and a hotkey | Minecraft (V: wiki) | Self-voicing with verbosity modes. | Adopt: Off / Menus / Menus and log, Ctrl+B-style hotkey, separate volume and rate. |
| Reading order designed per screen | Mortal Kombat 1 (S) | Screens declare what is read first. | Adopt: the widget tree order is the reading order; screens are reviewed for it. |
| High-contrast mode that recolours the world | TLOU2, God of War Ragnarok (V) | Hostiles, items, interactables in flat distinct colours. | Adapt: 3D high-contrast mode tints hostiles/items/interactables with the Okabe-Ito set and dims the rest. |
| Hold vs toggle per action | TLOU2 and GAG (V) | Any held input can be a toggle. | Adopt. |
| Narrate toast messages, with duration | Sea of Thieves (S) | Notifications spoken. | Adopt: "Entering: place" and badges are spoken when the narrator is on. |

## Patterns rejected outright

- **Floating/pinnable windows** (Zomboid): mouse-first, breaks focus navigation.
- **Spatial grid inventory** (DayZ, RE4): game-rule change, large UI, poor with text scaling.
- **Fully diegetic HUD** (Metro): slower to read, not screen-reader friendly, and BN is turn-based so "glance at your watch" has no cost anyway.
- **Marker-following navigation** (Skyrim-style objective arrow to every target): prior art and this research agree it replaces looking. Mission target on the compass only.
- **Timed popups** (BG3 complaint): nothing informational disappears on a timer; only the "Entering: place" notice fades, and it is also logged.
- **Two-stage analog trigger gestures** (BG3): not mappable to keyboard, hard to make accessible.
