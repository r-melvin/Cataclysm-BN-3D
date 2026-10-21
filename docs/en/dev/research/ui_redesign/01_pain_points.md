# 01. Pain points, ranked

Sourced complaints about Cataclysm (DDA and BN) menus and about the UIs of comparable games,
ranked by how many distinct sources the research actually read. Verbatim quotes with links;
the full lists are in `research/a_cdda_bn_pain_points.md` and the per-game files.

## How much to trust this

- **Reddit, YouTube comments and the Steam discussion board were not reachable** from the
  research tools (blocked, age-gated or 403). The brief asked for r/cataclysmbn, r/cataclysmdda,
  r/roguelikes, r/gamedesign and accessibility subreddits; none were mined. Nothing below comes
  from Reddit. This is the largest gap in the research.
- CDDA/BN evidence comes from GitHub issues (read via the API), the CDDA Discourse forum (read
  via its JSON endpoint) and the 151 most recent Steam reviews of CDDA (via the reviews API).
  Counts are "distinct items read", not a population survey; a count of 16 means sixteen
  things, not sixteen percent.
- Comparable-game quotes were fetched through a summarising fetch tool; the agents flagged
  that wording may not be character-exact. Items marked [snippet] were never opened.
- Many "UI audit" GitHub issues are developer-filed. They are counted separately where it
  matters.

## Cataclysm (DDA + BN) pain points

| Rank | Pain point | Distinct sources | Where |
|---|---|---|---|
| 1 | Menus overloaded, too many options, nested menus, every action throws the whole list at you | 16 | Steam, GitHub, Discourse |
| 2 | Keybindings inconsistent and hard to learn; rebinding breaks menus; default conflicts | 15 | Steam, GitHub |
| 3 | ImGui and curses-ImGui defects in upstream CDDA (blurry fonts, no wrap, mouse offset, crashes) | 15 | GitHub, Discourse |
| 4 | Text overflow, truncation, layout not fitting the screen or font size | 14 | GitHub, Steam |
| 5 | Pocket / nested-container mental model confuses players (CDDA; BN does not have pockets but has the same inventory UI lineage) | 10 | Steam, Discourse, GitHub |
| 6 | Mouse support missing or inconsistent | 6 | GitHub, Steam |
| 7 | Filter/sort/search gaps in inventory-like screens | 5 | GitHub (BN mostly) |
| 8 | Hidden actions and features; "double menu" from multi-purpose keys | 5 | GitHub, Steam |
| 9 | Selection or tab state resets on reopen | 3 | GitHub (BN) |
| 10 | Slow menus with many items | 2 | GitHub |

### 1. Menus overloaded

> "The menus in this game are excessively complicated. There are way too many options, and I don't want to read a novel just to open and drink a stupid bottle of water." ... "Instead of coding proper situational menus, they just throw the entire list at you every time."
> Steam review, negative, 2026-07-20. https://steamcommunity.com/profiles/76561197981255486/recommended/2330750/

> "This game is fire but good lord the inventory is so bad"
> Steam review, positive, 2025-05-21. https://steamcommunity.com/profiles/76561198170937002/recommended/2330750/

> "Many UIs are clunky to use, and require the player to "get used to" them."
> @olanti-p, BN #3602, 2023-11-09. https://github.com/cataclysmbn/Cataclysm-BN/issues/3602

> "The construction UI has not been updated since ages, yet the crafting UI has gotten a lot of features which the construction UI would benefit from."
> @NetSysFire, CDDA #70909, 2024-01-14. https://github.com/CleverRaven/Cataclysm-DDA/issues/70909

> "There are so many inventory-like interfaces"
> Gadgetpatch, Discourse 23753, 2020-05-25 (read via JSON). https://discourse.cataclysmdda.org/t/general-quality-of-life-ui-improvement-discussion/23753

Design consequence: one inventory screen instead of six inventory-like screens; one
contextual interaction verb instead of a verb per key; the "situational menu" the reviewer
asks for is the interaction prompt.

### 2. Keybindings

> "Its a great game that is unfortunately ruined by an ancient and awkward control scheme that is in desperate need of an overhaul."
> Steam review, negative, 2025-01-25. https://steamcommunity.com/profiles/76561198040659710/recommended/2330750/

> "the lack of consistent keys (ie press B to back out of menu or 2 or whatever) is very frustrating."
> Steam review, negative, 2023-12-20. https://steamcommunity.com/profiles/76561198011358609/recommended/2330750/

> "This renders the menu unusable, as in teh carat will not move when the newly bound key is pressed."
> @JBloodthorn, CDDA #45927 (changing movement keys breaks menu navigation), 2020-12-09. https://github.com/CleverRaven/Cataclysm-DDA/issues/45927

> "#86193 added batch size increase and batch size decrease bindings, setting their defaults to to `+` and `-`, but prioritize and deprioritize already had those as their default keys"
> @esotericist, CDDA #86331, 2026-04-07. https://github.com/CleverRaven/Cataclysm-DDA/issues/86331

> "The complexity of the giant amount of screens each with its own keybinds is another barrier, that is not fun to master."
> @thedyze, BN #5442, 2024-09-27. https://github.com/cataclysmbn/Cataclysm-BN/issues/5442

Codebase fact (verified): `data/raw/keybindings/keybindings.json` holds **585 actions in 61
contexts** with 674 keyboard, 28 gamepad and 16 mouse bindings; the default-mode context alone
has 132 actions. Per-screen contexts with their own keys are the structural cause.

### 3. Upstream ImGui migration defects (CDDA only; BN has no ImGui)

> "Brain immediately complains that the fonts look.. blurry? They definitely look off and not as crisp as the menu just before."
> @NetSysFire, CDDA #73120, 2024-04-19. https://github.com/CleverRaven/Cataclysm-DDA/issues/73120

> "imGUI still has several major problems in the curses build that makes menus difficult to use."
> @SandwichPie, CDDA #82161, 2025-07-31. https://github.com/CleverRaven/Cataclysm-DDA/issues/82161

> "Currently, the `input_context` turns all mouse clicks into the SELECT action, no matter where the user actually clicked."
> @db48x, CDDA #75288, 2024-07-28. https://github.com/CleverRaven/Cataclysm-DDA/issues/75288

> "On Windows with display scaling set to 200%, mouse clicks on the ImGui-rendered main menu land on the wrong entry."
> @YellowBee123, CDDA #88651, 2026-09-13. https://github.com/CleverRaven/Cataclysm-DDA/issues/88651

> "Uh... I just started playing experimental version of cdda and I don't like new "graphical" ui of some windows." ... "Like, escape menu is now not ascii-like and I can't find setting to disable it."
> @AnimeAss, Discourse 29530, 2024-11-22; reply @Profugo_Barbatus: "Nope, the imgui migration is not optional." https://discourse.cataclysmdda.org/t/changing-the-menu-interface/29530

The "looks like a dev tool" phrase from the brief was **not found** verbatim anywhere. Praise
was thin: one reviewer comment, "Overall the UI works really well" (@db48x on PR #73134,
2024-05-03). Lesson: a migration that ships half-done, with a second text renderer and two
mouse models, generates a long tail of bugs and a "can I turn it off" reaction. Whatever BN
builds must own one text path and one mouse model, and must not expose a half-migrated
state for years.

### 4. Text overflow and layout

> "I cannot read parts of quest descriptions, because they trail off the right edge of the quest menu."
> @anoobindisguise, CDDA #84564, 2026-01-09. https://github.com/CleverRaven/Cataclysm-DDA/issues/84564

> "It stretches the name box so far that the information about the spell on the right is illegible."
> @Keltas, CDDA #77116, 2024-10-18. https://github.com/CleverRaven/Cataclysm-DDA/issues/77116

> "Scenario and Profession don't fit on the screen"
> @bullkang, BN #10427, 2026-09-29. https://github.com/cataclysmbn/Cataclysm-BN/issues/10427

> "making the whole menu practically unreadable."
> @midgardormr, BN #10675, 2026-10-21. https://github.com/cataclysmbn/Cataclysm-BN/issues/10675

Codebase fact (verified): every screen is laid out in fixed character cells on an 80x24-minimum
grid; font size changes require a restart (`FONT_SIZE`, `MAP_FONT_SIZE`, `OVERMAP_FONT_SIZE`
in `src/options.cpp:2313-2320`). There is no wrapping layout engine.

### 5. Inventory mental model

> "it's all in one, long, painful line on the left side."
> @PoeSalesman, Discourse 12592, 2016-09-27. https://discourse.cataclysmdda.org/t/12592

> "a ton of people have been confused by this but i have no idea how."
> @KorGgenT (developer), Discourse 24456, 2020-08-15. https://discourse.cataclysmdda.org/t/24456

> "This gives a really awkward structure where how to equip an NPC is in two completely separate menus"
> @Daenyth, BN #3525, 2023-10-30. https://github.com/cataclysmbn/Cataclysm-BN/issues/3525

> "Right now, Unloading stuff can be a pain."
> @Kaylakaze, BN #2308, 2023-01-26. https://github.com/cataclysmbn/Cataclysm-BN/issues/2308

### 6. Mouse

> "There is no full mouse support. Partial mouse support is present, but for many cases it's impossible to interact with the UI using mouse."
> @olanti-p, BN #3602. https://github.com/cataclysmbn/Cataclysm-BN/issues/3602

Codebase fact (verified): `src/ui.cpp` (uilist), `inventory_ui.cpp`, `crafting_gui.cpp`,
`advanced_inv.cpp`, `newcharacter.cpp` and `main_menu.cpp` contain **zero** mouse or
coordinate handling; a left click in a uilist activates the highlighted row without hit-testing
the pointer. Only look-around, targeting and `query_popup` register `COORDINATE`.

### 7. Filter, sort, search

> "However, if the item is placed inside a container (for example, apple juice inside a bottle), it cannot be found."
> @jeremy7986, BN #7280, 2025-09-24. https://github.com/cataclysmbn/Cataclysm-BN/issues/7280

> "Everyone I've seen use these options (myself and two or three YouTubers) gets confused."
> @jbytheway, CDDA #26311 (crafting search prefixes like `c:`), 2018-10-18. https://github.com/CleverRaven/Cataclysm-DDA/issues/26311

### 8. Discoverability

> "so that newbies don't have to remember that '(' is disassemble (for example)."
> @HeatherSoron, CDDA #1368 (asking for an action menu), 2013-06-04. https://github.com/CleverRaven/Cataclysm-DDA/issues/1368

> "This multi-purpose nature frequently leads to a frustrating "double-menu" (nested menu) issue."
> @cosing, BN #9805 (examine also picks up), 2026-07-05. https://github.com/cataclysmbn/Cataclysm-BN/issues/9805

> "if I read a book, but don't want to read until I gain a skill level, I have to press either 4 keys for each time I read the book"
> @spacenookie, CDDA #28611, 2019-03-10. https://github.com/CleverRaven/Cataclysm-DDA/issues/28611

### 9 and 10. State resets, slowness

> "opening the construction menu will always move one tab counterclockwise from where it was before."
> @sato2077, BN #8299, 2026-03-10. https://github.com/cataclysmbn/Cataclysm-BN/issues/8299

> "Opening the construction menu in an area with a lot of items takes a while to open, seemingly longer the more items there are"
> @thaelina, CDDA #88904, 2026-10-04. https://github.com/CleverRaven/Cataclysm-DDA/issues/88904

### Sidebar (prior art from the CDDA fork research, GitHub issue #27367 and PR #71109)

> "Nearly everything on the current sidebar is important in nearly all situations"
> unnamed commenter, CDDA #27367. https://github.com/CleverRaven/Cataclysm-DDA/issues/27367

> "Sidebar take too much space ... The overall feeling is messy"
> nsklaus, CDDA #27367.

> "if it's so dense you can't understand it, it's too dense"
> kevingranade, CDDA #27367.

> "Please allow this to be an option"
> CoroNaut, CDDA #27367 (on hiding information by context).

These four quotes bracket the sidebar problem: players want all the state, find the sidebar
messy, and distrust anything that hides state without a switch. The user's instruction for
this fork (remove the sidebar in 3D mode) is consistent with the first three if, and only if,
every removed item is reachable as text on demand (F4 report, character sheet) and the "show
always" switch exists for the vitals strip.

## Comparable-game pain points that transfer

| Pain | Game, source | Transfer to BN |
|---|---|---|
| Hidden survival state in a contextual HUD | The Long Dark, Steam 2016-2023 (V): "let the Hud Open plz or put a On/Off switch...that informations are SO important in a survival game." (ZzozZ, 2017) | Contextual badges need an "always show" option. |
| Status icons unreadable at a glance | Project Zomboid B42, Steam Dec 2024 (V): "These moodlets are basically unreadable at first glance and you have to constantly double check what they mean." (Alabeo) | Icons carry a word; no icon-only state. |
| Icons that do not scale with the UI | Project Zomboid (V): "The old ones could not be scaled well with UI apparently." (Silamon, player account) | Vector/SDF glyphs or a per-scale icon set; test at 200 %. |
| Sorting hidden behind a button, filters unreliable | BG3 (V: Gfinity 2025; S: Steam) | Sort/filter in the header, one press. |
| Slow tooltips slow the whole screen | BG3 (V: Gfinity 2025) | Info pane updates synchronously with focus. |
| Equal items reshuffle when sorted | Fallout 76 bug post (V) | Secondary sort key. |
| Radial slots that reorder themselves | BG3 (S) | Slots are explicit bindings. |
| Controller: no hotbar, clunky | Project Zomboid (V: "with a controller it's extremely clunky and you also get no hot bar", 2022) | Hotbar and radials are controller-first. |
| Items hidden behind unlocks with no explanation | Valheim, Subnautica, Green Hell (V/S) | Show the unlock condition in the list. |
| Build tab too cluttered | Valheim (V: "The building tab is too cluttered. Split into Masonry and Carpentry tabs") | Construction categories plus search and a can-build filter. |
| Blueprint menu closes after each placement | Subnautica 2018 (V), fixed in Subnautica 2 | Build menu stays open. |
| No controller remap in-game | Valheim (S) | Remap everything in-game. |
| Tutorial popups vanish | BG3 (S) | Hints stay until dismissed (already how guided onboarding works). |
| No way to see why mood changed | RimWorld 2017 (V), CDDA Discourse 2019 (V: "I like to know why my toon is happy/sad.") | Morale reasons list stays, inside the character screen. |

## Positive signals (what not to break)

- CDDA advanced inventory called "actually incredibly intuitive to use" by one 2025 Steam
  reviewer (not attributed to any UI framework). The two-pane move model has fans; the new
  inventory keeps a two-pane move mode.
- The ZenFS thick sidebar was praised ("I've been using it all day on my ultra wide monitor
  and it's lovely", PR #71109). Some players want more on screen, not less. The classic build
  keeps its sidebar untouched; 3D mode offers the "always show vitals" switch and the
  character screen.
- Dwarf Fortress Steam kept its density and fixed navigation (scrollbars, tabs) and was called
  "a large step up" (PCGamesN 2020, V). Density is not the enemy; unnavigable density is.

## What the research could not establish

- Any Reddit thread on any topic in the brief.
- BN-specific Steam reviews (BN is not on Steam as far as the agents found).
- Player complaints about CDDA character creation (only developer audit issues exist).
- Player complaints about CDDA's item info text walls (none found).
- Whether CDDA's ImGui menus are slow (no attributed report).
- Any accessibility review of CDDA or BN by Can I Play That or similar (none found).
