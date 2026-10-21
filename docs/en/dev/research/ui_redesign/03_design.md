# 03. The new UI: design

Scope: the whole player-facing interface of Cataclysm: Bright Nights 3D, with the 3D renderer
as the primary presentation. The classic 2D tiles mode keeps its sidebar and gets every new
screen unchanged. The curses build is retired (user decision 2026-10-21; see
`06_frontend_options.md` §4).

Nothing here changes a game rule. Where a rule change would simplify the UI it is listed under
"Feature cuts (last resort)" with the alternatives that were rejected first.

Evidence references are to `01_pain_points.md` (P1-P10), `02_patterns.md` and the files in
`research/`.

## 1. Principles

1. **One screen per job.** Every piece of state and every verb has exactly one home. If two
   screens can do the same thing, one of them is deleted. (P1, P5, P8.)
2. **One interaction verb.** Look at a thing, see the prompt, press the key. Dedicated verb keys
   (open, close, smash, pick up, examine, talk, board) become optional bindings, not the way
   in. (P1: "throw the entire list at you every time"; P8.)
3. **The world shows, the text tells.** In-world presentation (sky, weather, badges, arrows,
   combat text) may replace sidebar text, but every item of state stays available as text on
   demand: the F4 status report, the character screen, the log. No state is image-only.
4. **Never colour alone.** Every state has a shape or icon, a word, and a colour, in that order
   of importance. Colour is the third channel. (research g, 4.4.)
5. **Nothing hides without a switch.** Contextual elements (badges, stamina, prompts) each have
   an "always show" position in one option. (The Long Dark split; CDDA #27367 "Please allow
   this to be an option".)
6. **Consistent navigation everywhere.** One menu-navigation context for every list, grid and
   tab in the game: up/down/left/right, confirm, back, next/prev tab, page, home/end, search,
   info. A player who can use one screen can use all of them. (P2.)
7. **Controller-first navigation, mouse-first pointing.** Every screen is operable with a D-pad
   and two buttons. Every list and button is also clickable, with correct hit-testing. No
   virtual cursor. (P6; research f.)
8. **Budget everything.** Screens, menu items, options and keybindings all have a number, and
   the number is held (section 2).
9. **Text scales; layout reflows.** Nothing is laid out in fixed character cells in the new
   layer. 75-200 % UI scale is a first-class state, tested by screenshot. (P4.)
10. **The accessibility tree is the UI tree.** Screen-reader narration, controller focus and
    mouse hit-testing all read the same widget tree; there is no second description of a screen.

## 2. Budgets: before and after

### 2.1 Screens and menus

"Before" counts come from `research/l_codebase_ui_audit.md` (one row per distinct screen,
player-facing, excluding debug/Lua/loading/computer terminal content).

| Group | Before (screens) | After (screens) |
|---|---|---|
| Out of game | Main menu, New character (9 tabs), Load, World/mods (worldfactory), Credits, MOTD = 6 | Main menu, New game (one screen, 5 steps), Load, Settings, Credits = 5 |
| Always-on | Sidebar (4 layouts + panel manager), pixel minimap, 3D HUD = 3 | HUD (3D) / sidebar (classic) = 1 |
| Items | Inventory, Advanced inventory, Pickup, Wear, Wield, Take off, Read, Reload, Gun mods, Sort armour, Item info, Examine item menu, Item action menu, Drop, Unload = 15 | Inventory (modes: manage, loot, trade, pick) + Item info card = 2 |
| Making | Crafting, Construction, Zones manager, Salvage, Vehicle interaction = 5 | Craft, Build (tabs: Construct, Zones), Vehicle = 3 |
| Character | Character sheet (7 tabs), Mutations, Bionics, Perks, Morale, Spells = 6 | Character (tabs: Status, Skills, Traits, Bionics, Spells*) = 1 |
| World | Overmap, Missions, Factions, Messages log, Diary, Scores, Notes manager, List items/monsters (look-around filters) = 8 | Map, Journal (tabs: Log, Missions, Lessons, Notes, Records), Cursor mode (look/target/list) = 3 |
| People | Dialogue, Trade = 2 | Dialogue, Trade (an Inventory mode) = 1 + 0 |
| Settings | Options (6 pages + font page), Keybindings, Colour manager, Safe mode, Auto-pickup, Auto-notes, Panel manager, Distraction manager = 8 | Settings (tabs: Game, Display, Audio, Controls, Accessibility, Rules), Controls editor (also the command palette) = 2 |
| Help | Help, Onboarding checklist, Status report popup, Action menu = 4 | Reference ('?'), Status report (F4, text) = 2 |
| Popups | query_popup, string_input_popup, string editor, uilist (generic) = 4 | Confirm, Text entry, Choice list = 3 |
| **Total** | **61** | **23** |

\* Spells tab appears only when the character has spells; Bionics only with bionics.

{{svg:screens_before_after}}

### 2.2 Items per screen

Budget: a **menu** (a list of verbs or options) holds at most **12 items** and nests at most
**two levels** (screen > tab > item). **Data lists** (inventory rows, recipes, log lines) are
not menus; they are bounded by filters, search and scrolling instead. Every screen below states
its menu count.

| Screen | Before (menu items) | After |
|---|---|---|
| Main menu | 8 top + New Game 4 + Settings 5 + World 4 = 21 | 5 + New game steps 5 = 10 |
| Pause menu | n/a (main-menu ESC with 8) | 5 |
| Interaction prompt | Examine menu: up to 10 verbs plus nested uilists | 1 primary verb + up to 5 secondary (hold key) |
| Action wheel | Action menu (`Enter`): ~60 entries in nested uilist | 8 |
| Quick-slot wheel / hotbar | none (hotbar did not exist) | 8 |
| Inventory | 24 bound actions in INVENTORY_ITEM, 36 in ADVANCED_INVENTORY, 7 in INVENTORY | 6 tabs, 8 verbs on the item card, 12 bound actions total |
| Craft | 15 bound actions, 9 category tabs + subcategories | 4 filters, 6 category tabs, 6 verbs |
| Build | 4 bound actions + 10 construction categories | 6 category tabs, 4 verbs, 1 Zones tab with 6 verbs |
| Character | 7 tabs + 5 separate screens | 5 tabs, 0 verbs except on Bionics/Spells (activate) |
| Map | 21 bound actions | 3 layers, 8 verbs |
| Journal | P/M/Home/)/kill list = 5 screens | 5 tabs, 4 verbs |
| Settings | 252 options over 7 pages (+43 Android) | 39 options over 6 tabs (see 2.3) |
| Controls | 585 actions in 61 contexts, no search | same actions, 12 contexts, one searchable list |

### 2.3 Options

Before: 252 registered options (`src/options.cpp`): interface 57, graphics 74 (of which 20 are
`R3D_*`), world defaults 47, debug 24, general 5, font 3, plus 43 Android. After: **39
in-game** on six tabs; everything else moves to the advanced config file (section 9) or to
the world-creation screen. The in-game list:

**Game (7):** Language (`USE_LANG`), Units (Metric / Imperial; replaces the five unit
options), Autosave (Off / 5 / 15 / 30 min; merges autosave on/off and interval), Safe mode
(Off / On; distance in Rules), Auto-pickup (Off / On; rules in Rules), Confirmations (Few /
Normal / All; merges `QUERY_DISASSEMBLE`, `QUERY_KEYBIND_REMOVAL`, `FORCE_CAPITAL_YN`),
Guided hints (existing).

**Display (9):** Renderer (Classic 2D / 3D; `USE_TILES`+`RENDERER_3D`), Tileset (`TILES`),
Window (Windowed / Fullscreen / Borderless), HUD (Full / Minimal / Off), Minimap (Off / On),
Camera (Orbit / First person; `R3D_CAMERA_MODE`), Field of view (`R3D_FP_FOV`), Weather and sky
effects (`R3D_WEATHER`), Cutaway around the player (`R3D_CUTAWAY`).

**Audio (4):** Master, Music, Effects, Ambient (existing `*_VOLUME` plus `SOUND_ENABLED` folded
into Master = 0).

**Controls (5):** Keyboard and mouse bindings (opens editor), Gamepad bindings (opens editor),
Movement keys preset (Vi / WASD / Numpad / Arrows; sets a binding group), Gamepad (Off / Auto),
Rumble (Off / On).

**Accessibility (11):** Preset (None / Vision / Hearing / Motor), UI scale (75-200 %), Colour
vision (Normal / Deuteranopia / Protanopia / Tritanopia / High contrast), Narration (Off / Menus
/ Menus and log), Narration rate, Narration volume, Reduce motion and flashing (Off / On), Sound
cues as text (Off / On), Hold or toggle (Hold / Toggle; global default for holdable actions),
Confirmation holds (Hold / Press twice), Readable font (Off / On).

**Rules (3 list editors):** Safe mode rules, Auto-pickup rules, Auto-notes rules.

Options that move to the file include: every `ANIMATION_*`, `BLINK_SPEED`, `MESSAGE_*`,
`SIDEBAR_*`, `PIXEL_MINIMAP_*` (9 options collapse into the single Minimap toggle in-game),
`R3D_FP_*` tuning, `R3D_VIEW_DEPTH`, `R3D_LEVEL_HEIGHT`, `R3D_THIN_WALLS`, `R3D_WALL_TEXTURES`,
`R3D_VOXEL_ITEMS`, `TEXTURE_STREAMING`, `RENDER_BATCHING`, `FRAMEBUFFER_ACCEL`, `VSYNC`,
`SCALING_FACTOR`, `COMPUTE_ACCELERATION`, `TERMINAL_X/Y`, all font options, `AIM_*`,
`ACCURACY_DISPLAY`, `MORALE_STYLE`, `HEALTH_STYLE`, `LOG_FLOW`, `WIKI_DOC_URL`, `HHG_URL`,
all `debug` options, and all `ANDROID_*`. World defaults stay per world (`worldoptions.json`)
with 8 shown in the New game > World step and the rest file-edited.

The cut is justified per option in `05_plan.md`, milestone M2.

### 2.4 Keybindings

Before: 585 actions, 61 contexts. After: the same actions exist (mods and scripts bind them)
but default bindings are reduced: **play context ≤ 32 default-bound actions on keyboard, ≤ 14
on gamepad**, and the generic `menu` context replaces 40+ per-screen navigation duplicates.
Contexts: `global`, `play`, `cursor`, `menu`, `text`, `inventory`, `craft`, `build`, `map`,
`vehicle`, `dialogue`, `debug` = 12. Per-screen extras live in those; nothing else registers
its own context.

## 3. Information architecture

Three tiers. An item of state is in exactly one tier; the F4 status report and the character
screen are the text fallbacks for tiers 1 and 2.

{{svg:tiers}}

| Tier | What | Where |
|---|---|---|
| **Always visible** (HUD: Full) | Vitals strip (body figure, stamina when relevant, move mode), safe-mode state, compass tape, minimap (if on), hotbar, message ticker (3 lines), interaction prompt, mission target on compass | HUD |
| **Shown when abnormal** | Needs badges with trend (hungry, thirsty, tired, cold, hot, pain, wet, overloaded, noisy), status-effect badges (bleeding, infected, poisoned, drunk, ...), stat-reduction badges (STR -2), morale badge (Miserable / Elated), power/mana rows, vehicle cluster while driving, edge arrows (hostile, sound), hit direction, "Entering: place", pinned recipe shortfall | HUD |
| **On demand** | Exact numbers for everything above, HP per limb, speed and move cost breakdown, temperature per body part, weather forecast text, time and date, weight/volume, morale reasons, effects with durations, skills, traits, bionics, armour, encumbrance | Character screen (`@`), F4 report (text), Inventory header |
| **Text only** | Everything that the HUD shows, as sentences | F4 status report (one paragraph; spoken by the narrator), message log |

HUD modes: **Full** (all of tier 1 and 2), **Minimal** (tier 2 badges, prompt, ticker; vitals
only when abnormal), **Off** (prompt and sound subtitles only). "Always show vitals" is
implied by Full.

### 3.1 Sidebar replacement (3D mode)

The current sidebar (`src/panels.cpp`, four layouts, `labels` by default) is removed in 3D
mode. Its readability problems: two-letter limb labels in six colours, numbers packed to the
cell, panel borders that eat a third of the width, state encoded by colour alone (stat
buckets, limb colours), a message panel that is cut to 3-6 lines by layout, and no scaling
(cell grid at a restart-only font size). Every panel's information is reassigned below.

| Sidebar panel (panels.cpp) | Problem today | 3D replacement | Tier |
|---|---|---|---|
| Limb HP (`draw_limb_*`, `draw_limb_health`) | 6 rows of `HD`/`TO`/`LA` with coloured bars; colour-only severity | Body figure in the vitals strip: six segments filled by HP %, hatched when broken, a drop icon when bleeding, a tooth icon when bitten; the lowest limb's percent as a number under it; label "HP" | Always; exact numbers in Character > Status and F4 |
| Stamina (`draw_stamina` inside stat/needs panels) | Thin bar, easy to miss | Bar under the body figure, shown when below max, while running, or always in HUD Full | Always (Full) |
| Needs (`draw_needs_*`) | Six coloured words | Badges only when abnormal, each with icon + word + trend arrow (▲ worsening, ▼ improving) | Abnormal; F4 |
| Stats STR/DEX/INT/PER (`draw_stat_*`) | Four numbers whose colour means "reduced" | Character > Status; a badge "STR -2" appears while a stat is reduced | On demand; abnormal |
| Speed / moves / move mode (`draw_char_*`, `move_mode_string`) | "Spd 100 Mv 50" | Move mode icon + word in the vitals strip (Walk / Run / Crouch); speed with breakdown in Character > Status | Always (move mode); on demand (speed) |
| Time / date / moon (`draw_time*`, `draw_moon_*`) | Clock only with a watch; moon glyph | Clock on the compass tape right end when the character has a watch; sun/moon sprites in the sky (shipped); date and approximate time in F4 | Always (clock, gated by watch); text |
| Weather / temperature / wind / lighting (`draw_env_compact`, `draw_weather_classic`, `temp_stat`, `draw_wind`, `draw_lighting_classic`) | Text lines; temperature arrows in colour | Sky, precipitation, fog, wind chevron on the compass (shipped). Temperature badge (Cold / Hot, with body-part detail in F4) gated by feeling; exact degrees in F4 when a thermometer is carried; lighting as a sun/moon brightness glyph on the compass | Diegetic + abnormal badge; text |
| Location (`draw_loc_*`, `draw_location_classic`) | Place name | "Entering: place" notice (shipped) and the minimap label; always in F4 and Map | Event + on demand |
| Safe mode (`safe_color`) | "SAFE" coloured word | Shield icon + "SAFE" / "SAFE OFF" top-left; interrupt popup names the creature | Always |
| Messages (`draw_messages*`) | 3-6 lines clipped by layout | Three-line ticker above the hotbar, newest at the bottom, older lines dim (no fade under Reduce motion); scroll wheel / `L2` over it opens Journal > Log | Always; full log on demand |
| Minimap (`draw_mminimap`, `draw_minimap`, pixel minimap) | Two minimaps (overmap chunk and pixel) | One minimap, top-right: heading-up in first person (shipped), north-up in orbit; local pixel minimap dropped | Always if on |
| Compass (`draw_compass`, `draw_simple_compass`) | Creature letters per direction | Compass tape (shipped) + edge arrows for hostiles (shipped) | Always |
| Vehicle (`draw_veh_*`) | Speed, fuel, cruise in one line | Vehicle cluster replaces the stamina row while in a vehicle: speed and cruise target, fuel bar per tank type, engine state word, lights/doors icons | While driving; F4 says "Driving at 30 mph, fuel 40 %" |
| Weapon / style (`draw_weapon_*`) | Name + style | Hotbar slot 1 shows the wielded item name and ammo count; style name in Character > Skills | Always (slot) |
| Weight / volume (`draw_weightvolume_*`) | Two numbers | Inventory header; badge "Overloaded" when over limit | On demand; abnormal |
| Armour (`draw_armor*`) | Per-limb armour values | Character > Status (body figure hover/focus shows armour per limb) | On demand |
| Power / mana (`power_stat`, `mana_*`) | Numbers | Extra row in the vitals strip only when the character has bionics / spells | Always when applicable |
| Morale face (`morale_emotion`) | ASCII face | Badge when mood is outside the neutral band; reasons list in Character > Status | Abnormal; on demand |
| Sound / stealth (`draw_sound_*`, `draw_stealth`) | "Sound: 12" | Badge "Noisy" when own noise exceeds a threshold; exact in F4 | Abnormal; text |
| AI goal (`draw_ai_goal`) | Debug text | Dropped from the player HUD; stays in the debug menu | Dropped |
| Lua sidebar widgets (`lua_sidebar_widgets`) | Mod-defined text lines | Rendered as text badges in the badge row with the widget's colour; full text in F4. **Decision needed** (open question 6 in `05_plan.md`) | Abnormal / text |
| Hint panel (`draw_hint`) | Onboarding "Next" step | Lesson line at the top of the message ticker, dismissable | Always while a lesson is active |

### 3.2 Map area without the sidebar

Today `game::create_or_get_main_ui_adaptor` sizes the terrain window as `TERMX -
(sidebar_left + sidebar_right)` (`src/game.cpp:4692-4729`) and `game::draw_panels`
(`game.cpp:4946`) draws every panel of the current layout beside it. In 3D mode with the
sidebar removed:

- `panel_manager::get_width_left/right()` return 0 and `draw_panels` returns early when
  `RENDERER_3D` is active (one predicate, `ui::sidebar_hidden()`).
- `TERRAIN_WINDOW_WIDTH = TERMX`, `w_terrain` at `point_zero`; the 3D draw rectangle in
  `src/sdltiles.cpp:1684` already follows `w_terrain`, so the picture fills the window.
- Screens that position themselves relative to the sidebar must be re-anchored: look-around
  info window (`game.cpp:10082-10107`, uses `get_width_left()` and `LOOKAROUND_POSITION`),
  the `offsetX` computation at `game.cpp:9576`, the right-offset block at
  `game.cpp:10531-10539`, and pickup/AIM window placement (`PICKUP_POSITION`). In the new
  layer these become HUD cards anchored to a screen edge.
- The pixel minimap window (`w_pixel_minimap`) is no longer created in 3D mode.
- Curses and classic 2D are untouched: the predicate is false there.

Risks: anything that reads `TERRAIN_WINDOW_WIDTH` to compute a sidebar-relative position
(grep `get_width_left|get_width_right|SIDEBAR_POSITION` in `game.cpp`: 7 sites), the panel
manager key (`}`) must become a no-op with a message in 3D, saved panel layouts must still
load for classic mode, and the option toggle at runtime must mark the main ui_adaptor for
resize.

## 4. Screen by screen

Wireframes are at 100 % scale on a 16:9 window. Letters in brackets are default keyboard
keys; gamepad glyphs replace them when a gamepad was the last device used.

### 4.1 HUD (3D, orbit and first person)

{{svg:hud_layout}}

```
+------------------------------------------------------------------------------+
| [shield] SAFE        W . . N . . E        wind>  (sun)          +---------+   |
|                        ^mission  ^note                          | minimap |   |
|                                                                 |  12:40  |   |
|                                                                 +---------+   |
|                                                                               |
|                                  (3D scene)                  hostile > [edge] |
|                                                                               |
|   [cold] Cold v    [bed] Tired    [bolt] Pain ^           Entering: Evac shelter
|                                                                               |
|  +------+                                                                     |
|  | body | HP 62 %         > You hear a gunshot to the north, far away.        |
|  +------+ Stamina ||||||||..                                                  |
|  [walk] Walk               > The zombie bites your left arm!                  |
|                            > You feel cold.                                   |
|  [1 pistol 7/15] [2 bandage x3] [3] [4] [5] [6] [7] [8]       [E] Open door   |
+------------------------------------------------------------------------------+
```

- Compass tape top-centre (shipped), clock at its right end only when a watch is carried.
- Edge arrows (shipped) get a chevron shape with a one-letter class glyph (Z zombie, A animal,
  H human, ? unknown, ♪ sound) so hostile vs sound is not colour-only.
- Badge row above the vitals: icon + word + trend. Order is fixed (needs, then effects, then
  stats) so a returning glance finds things in the same place.
- Vitals strip bottom-left: body figure (six segments), stamina bar, move mode. Rows for
  power and mana appear below when applicable; the vehicle cluster replaces stamina while
  driving.
- Message ticker: three lines, left-aligned above the hotbar; combat text in-world stays.
- Hotbar: eight slots; slot 1 is always the wielded item (name, ammo). Slots are items or
  actions.
- Interaction prompt bottom-right: glyph + verb + target name.
- Hit direction: edge chevron plus glow (shipped glow; chevron added for colour-blind use).
- All HUD text is drawn by the new text layer at UI scale, not the map font.

Menu items: 0. Options involved: HUD mode, Minimap, UI scale.

### 4.2 Interaction prompt and secondary verbs

Replaces: examine (`e`), open/close (`o`/`c`), smash (`s`), pick up (`g`, single tile), talk
(`C`), board/alight, use furniture, drive, and the examine item menu.

Target selection: in first person the crosshair tile; in orbit the tile in the facing
direction, or the tile under the mouse when the pointer is over the scene, or the adjacent
tile chosen by holding the interact key and pressing a direction.

```
[E] Open door                      (primary verb: one press)
[hold E] ⌄ Close | Lock | Examine  (secondary verbs: hold to reveal, release on one)
```

Rules: one primary verb computed by priority (talk > drive/board > open/close > pick up
(single item) > loot (several items, opens Inventory in loot mode) > examine). Up to five
secondary verbs in a hold list; more than that is a design error. Smash is a secondary verb
on terrain and furniture. The prompt always shows the target's name and, for creatures,
their one-word attitude (hostile / neutral / friendly) with an icon.

Gamepad: `A` primary, hold `A` for the list, stick to pick, release to run. Keyboard: `E` and
hold `E`. Mouse: left click = primary, right click = the list as a context menu at the
pointer. Hold-vs-toggle: "Confirmation holds" and the global Hold/Toggle apply.

### 4.3 Action wheel and quick-slot wheel (hotbar)

Two radial menus of eight. Hold to open, direction to pick, release to run. Keyboard: hold
`Tab` (actions), hold `Q` (quick slots), or press `1`-`8` directly for quick slots. Mouse:
hover a slice. Gamepad: hold `LB` (actions), hold `RB` (quick slots); D-pad left/right cycles
the hotbar without the wheel.

Action wheel, fixed order: Inventory, Craft, Build, Character, Map, Journal, Wait/rest, Look.
These are also the eight screen keys (`i`, `&`, `*`, `@`, `m`, `j`, `|`, `x`). The wheel exists
so a gamepad reaches every screen in one gesture and so the keys are discoverable.

Quick slots: items (wield, wear, eat, use, read) or actions (fire, reload, throw, sleep,
toggle light). Binding: in Inventory, "Assign to slot" on the item card; in the Controls
editor for actions. Slots never reorder themselves (BG3 complaint).

### 4.4 Inventory (one screen, four modes)

Replaces 15 screens (table 2.1). Modes differ only in which panes are shown and which verb
is the default:

- **Manage** (`i`): Nearby | Worn and wielded | Pack. Default verb: use.
- **Loot** (from the prompt or `g`): Nearby (the looked-at tiles, expandable to 3x3) | Pack.
  Default verb: take.
- **Trade** (from Dialogue): Their goods | Your goods, with a balance row.
- **Pick** (any "choose an item" request: wear, wield, read, reload ammo, eat, gun mod, drop,
  unload, salvage, component choice): a single filtered pane with the requested verb as
  default; `Esc` cancels. This is the `game_menus::inv::*` family rendered by the same screen.

```
+-- Inventory ------------------------------------------- 23.4 / 40 kg   18 / 36 L --+
| [All] [Weapons] [Clothing] [Food] [Tools] [Other]      search [        ]  sort: Name v |
|-------------------------------------+------------------------------------------------|
| NEARBY  (3 tiles)                   | Survivor jacket                     worn: torso |
|  > wooden plank x4           2.4 kg | Warmth 20   Coverage 90 %   Encumbrance 12      |
|    rock x2                   1.0 kg | vs hoodie (worn):  Warmth +12 ^  Encumb +4 ^    |
|    first aid kit             0.5 kg |                                                 |
| WORN / WIELDED                      | Sturdy. Keeps rain out. Pockets: 2.              |
|  wielded  pipe wrench        1.1 kg |                                                 |
|  torso    survivor jacket    1.8 kg | [E] Use  [W] Wear  [D] Drop  [T] Take           |
|  legs     jeans              0.6 kg | [A] Assign slot  [I] Full info  [C] Compare     |
| PACK                                |                                                 |
|  water bottle (2)            1.0 kg |                                                 |
|  can of beans                0.4 kg |                                                 |
|-------------------------------------+------------------------------------------------|
| Tab pane  Enter move  Space mark  Ctrl+A all  Esc close            Marked: 2 (3.4 kg) |
+---------------------------------------------------------------------------------------+
```

- Left pane: three sections in one scroll (not three panes), so Tab cycles sections and the
  list never loses position (P9). Section headers show weight/volume totals (StarUI).
- Column sort by header click or `S`; secondary key is name; the active key is shown.
- Category tabs across the top; `LB/RB` or `[`/`]` switch; search filters all sections and
  looks inside containers (BN #7280). The search box has no prefix syntax (CDDA #26311);
  instead, three toggle chips: "can use", "needed for pinned recipe", "junk".
- Item card right: 6-line summary, comparison against the worn item for the same body part
  when `C` is held (toggle on gamepad), deltas as signed numbers with ▲▼ glyphs, full info on
  `I` (scrollable page). The card updates synchronously with focus; no hover delay.
- Move: `Enter` moves the focused or marked items to the other section (Nearby ⇄ Pack); for
  Worn, `Enter` is wear/take off. Drag-and-drop works with the mouse but is never required.
- Worn order (armour layers) is edited by `Shift+Up/Down` in the Worn section; the separate
  sort-armour screen is gone.
- Reload: "Pick" mode lists compatible ammo with a "best" row first.

Menu count: 6 tabs, 8 verbs, 12 bound actions in the `inventory` context.

### 4.5 Craft

```
+-- Craft --------------------------------------------------------------------------+
| [Can craft] [Missing one] [All known] [Favourites]     search [       ]   Tools near: 4 |
| [Food] [Tools] [Weapons] [Armour] [Chemistry] [Other]                                   |
|-----------------------------------+------------------------------------------------------|
| > Makeshift knife           2 min | Makeshift knife                      skill: fabric. 1 |
|   Wooden spear              5 min | Needs: 1 scrap metal (have 3), 1 rag or duct tape (2) |
|   Bandage x3               10 min |        tool: hammer or rock (hammer nearby)           |
|   Water, clean              8 min | Result: 1 makeshift knife.  Batch of 3: 5 min.        |
|   ...                             |                                                       |
|                                   | [Enter] Craft 1  [B] Batch...  [P] Pin  [F] Favourite |
|                                   | [I] Item info  [R] Who can teach / book               |
|-----------------------------------+------------------------------------------------------|
| 42 recipes shown, 310 known, 6 unknown in this category (needs: Fabrication 3 or a book)  |
+------------------------------------------------------------------------------------------+
```

- Four filters, six category tabs, no subcategory tabs (search replaces them).
- Component alternatives render as one row with the chosen alternative and `Left/Right` to
  cycle (Vintage Story).
- "Pin" puts the recipe's shortfall on the HUD badge row and tags matching items "needed" in
  inventory and loot views.
- Unknown recipes are counted, never named (Satisfactory thread); the count's hint says how
  to learn them (skill level or book type).
- Crafting from nearby containers is existing BN behaviour; the header states the radius
  ("Tools near: 4").

Menu count: 4 filters, 6 tabs, 6 verbs.

### 4.6 Build (Construct + Zones)

Construct tab mirrors Craft (filters: Can build / Missing one / All; six categories; search;
detail card with stages and materials). Differences: choosing a construction enters placement
mode over the scene; the menu stays open (Subnautica 2 rule); `E` places, `R` rotates where
applicable, `Esc` returns to the list. "Copy this" (`C` in cursor mode on existing
furniture/terrain) selects the matching construction (Satisfactory F+middle-click). The list
remembers its tab and selection (BN #8299, #8154).

Zones tab: a list of zones for the current area with type, name, size; verbs: Add (draw a
rectangle in cursor mode), Edit, Move, Enable/disable, Delete, Show on map. Replaces the
zones manager screen.

Menu count: 6 tabs, 4 verbs (Construct); 6 verbs (Zones).

### 4.7 Cursor mode (look, target, list)

`x` (look), `f` (fire), `t` (throw) and the "list items/monsters" screens become one mode: a
cursor on the scene with a side card. The card describes the tile under the cursor (terrain,
furniture, items, creature with attitude, remembered vs seen), and has filter chips "Items"
"Creatures" "Terrain" that turn the card into a sorted list of visible things; selecting a
row jumps the cursor. Verbs: Fire/Throw (when entered from `f`/`t`, with the aim confidence
shown as a bar with a word), Travel to, Note, Copy construction, Peek. The narrator reads the
card on every cursor move (section 7.3).

Menu count: 3 chips, 6 verbs.

### 4.8 Vehicle

Opened by interacting with a vehicle part. Replaces `veh_interact` (3270 lines, 24 bound
actions) with a screen of three areas: a top-down part grid (reuses `vehicle_preview`), a
part card (state, fuel, controls), and verbs: Install, Repair, Remove, Refill/Siphon, Rename,
Change tyre, Assign crew/animal; "Examine contents" opens Inventory in loot mode. Driving
controls (lights, cruise, horn, engine) live on the HUD vehicle cluster and the action wheel
while seated, not in this screen.

Menu count: 8 verbs. This is the hardest screen and is last in the plan.

### 4.9 Character

```
+-- Character: Sam Reyes ------------------ [Status] [Skills] [Traits] [Bionics] [Spells] --+
|  +--------+  Head      100 %        Hunger   Normal        Mood   -12  Miserable          |
|  |  body  |  Torso      62 % bitten Thirst   Thirsty v      - Cold (-8)                   |
|  | figure |  L arm      40 % broken Fatigue  Tired v        - Saw a friend die (-10)      |
|  |        |  R arm      90 %        Pain     Moderate ^     + Fine meal (+6)              |
|  +--------+  L leg      85 %        Temp     Cold (torso)                                 |
|              R leg      85 %        Weight   23.4/40 kg                                   |
|  STR 10  DEX 9 (-1 pain)  INT 11  PER 8        Speed 92 (-8 pain)   Moves 100            |
|  Effects: Bleeding (left arm, 4 min), Cold, Infected (torso, severe)                      |
|  Armour (focus a limb): Torso bash 6 cut 8 ballistic 2   Encumbrance 12                   |
+-------------------------------------------------------------------------------------------+
```

Tabs: Status (as above, including the morale reasons list and effects with durations),
Skills (level, progress, rust, style), Traits (mutations and perks, with the perk picker's
verbs), Bionics (activate / deactivate / reassign slot), Spells (cast, assign slot; only when
present). Replaces six screens. Every number the HUD summarises is here in full.

Menu count: 5 tabs; verbs only on Bionics/Spells (3 each).

### 4.10 Map

The overmap with three layers toggled by chips: Notes, Missions, Zones (local zones when
zoomed to the current overmap tile). Verbs: Travel here, Add note, Edit/delete note, Set as
destination, Centre on me, Filter notes (search), Delete notes in view (with count), Zoom.
The notes manager list is the Journal > Notes tab, not a map sub-screen. Missions are listed
in the Journal; the map shows the active target. Replaces overmap, notes manager and the
zone overlay.

Menu count: 3 layers, 8 verbs.

### 4.11 Journal

Tabs: **Log** (the full message log, filter chips: Combat, Needs, Danger, Info, Speech;
search), **Missions** (active / completed, each with its target and "show on map"),
**Lessons** (the guided-onboarding checklist; re-readable), **Notes** (diary entries and
overmap notes in one list, add/edit), **Records** (kills, scores, achievements, play time).
Replaces messages, missions, diary, scores, kill list and the onboarding checklist popup.

Menu count: 5 tabs, 4 verbs.

### 4.12 Dialogue and trade

Dialogue keeps its shape: the NPC's line, then a numbered list of responses (data list, not a
menu). Added: the NPC's attitude word and icon in the header, a "repeat last line" key for the
narrator, and consistent `menu` navigation. Trade opens Inventory in Trade mode. The dozen
NPC-management actions that today live in dialogue (follow, guard, rules, equipment) stay as
dialogue responses; "Manage equipment" opens Inventory with the NPC's panes (BN #3525 asked
for this to be one place).

### 4.13 Settings and Controls

Settings: six tabs (2.3), 39 options, each a single row with its value and a one-line
description under the focused row. Accessibility is the fifth tab and is also reachable from
the main menu and the pause menu directly ("Accessibility" entry) so it is found before any
other setting is needed.

Controls editor: one searchable list of every action, grouped by context, three columns
(Keyboard, Mouse, Gamepad). `Enter` on a column captures the next input (keys with modifiers,
mouse buttons including middle, gamepad buttons with an optional held `LT` modifier);
conflicts within the context and its parents are shown inline with a one-key "swap" fix.
Per-action "hold / toggle" and "assign to quick slot" live in the row. Presets: Vi, WASD,
Numpad, Arrows, Left-handed, One-handed (section 7.9). The same list, filtered to runnable
actions, is the **command palette** (`Ctrl+Space` / `Start+Y`): type a verb, run it.

### 4.14 Main menu, new game, pause

Main menu (5): Continue, New game, Load, Settings, Quit (Credits is under Settings > Game as
a row "About and credits": the 40th item is a link, not an option). The MOTD becomes a line
under the title.

New game: one screen, five steps down the left (World, Scenario, Profession, Character,
Skills and traits), detail on the right, points in the header, "Quick start" (random of
everything valid) and "Guided play" buttons at the top. The World step shows name, seed,
mods (opens the mod list as a Pick list), and 8 world options (city size, spawn density,
NPC density, season length, monster evolution, vehicle spawn rate, item spawn rate, starting
season); the other 39 world options are file-edited (`worldoptions.json`, documented).
Character creation today is 4799 lines over nine tabs; the "Skills and traits" step merges
Stats, Traits, Skills, Bionics into one scrolling form with the points shown per group.

Pause (`Esc`, 5): Resume, Save, Settings, Reference, Quit to menu.

### 4.15 Reference ('?')

One searchable screen: Actions (every action with its binding, runnable), Lessons, Items and
recipes (the in-game handbook: item pages with "used in" and "made from"), Game concepts (the
existing help texts). Replaces help, keybinding help, action menu and the onboarding
checklist popup. The F2 "explain this screen" lesson stays and opens here.

## 5. Input model

### 5.1 Actions and contexts

The unit stays the action id (`data/raw/keybindings/*.json`). Contexts become twelve
(section 2.4) and nest: `global` < `play` | `menu` | `cursor` | `text`; `inventory`, `craft`,
`build`, `map`, `vehicle`, `dialogue` extend `menu`; `debug` extends `global`. A screen
registers only its own verbs; navigation comes from `menu`. This fixes "rebinding movement
breaks menus" (CDDA #45927): menus do not use movement actions.

The `menu` context: up, down, left, right, confirm, back, next tab, prev tab, page up, page
down, home, end, search, info, help, mark, mark all, sort, secondary (context menu). 18
actions, bound once.

{{svg:input_contexts}}

### 5.2 Devices

- **Keyboard:** key + modifiers (Ctrl, Shift, Alt) captured as a unit. Today modifiers are not
  capturable in the editor (research m, A). Movement presets set the eight directions, wait,
  and stairs.
- **Mouse:** left, right, middle, wheel, with modifiers; movement by click-to-travel on the
  scene (existing auto-travel); right-click context menu = the hold-list of the prompt;
  middle drag stays the camera (shipped). Every list row and button is hit-tested.
- **Gamepad:** replace `SDL_Joystick` (hat-only, buttons sent as keyboard events, no sticks:
  `src/sdltiles.cpp:1871-1935`, `3499-3503`) with `SDL_Gamepad`: hot-plug, named buttons,
  sticks with dead zones, rumble, `SDL_GetGamepadButtonLabel` for glyphs (available in the
  system SDL 3.4.18 header). One optional modifier: hold `LT`. Default layout:

| Input | Play | Menu |
|---|---|---|
| Left stick / D-pad | move (8-way; D-pad 4-way + LT for diagonals) | navigate |
| Right stick | camera (orbit) / look (first person) | scroll detail card |
| A | interact (hold: verb list) | confirm |
| B | cancel / back / stop activity | back |
| X | use quick slot (current) | secondary verb / context menu |
| Y | inventory | info |
| LB hold | action wheel | prev tab |
| RB hold | quick-slot wheel | next tab |
| LT | modifier (diagonals, hold-list) / aim mode (hold or toggle) | page up |
| RT | fire / attack confirm | page down |
| D-pad L/R (play) | cycle hotbar | |
| D-pad Up / Down (play) | wait one turn / crouch toggle | |
| Start | pause | |
| Back/View | map | search |
| LS click / RS click | zoom / toggle first person | home / end |

14 play actions, all content reachable (Steam Deck rule 8). Text entry opens the on-screen
keyboard (`SDL_StartTextInput` reaches the Deck keyboard; a built-in grid keyboard for other
pads).

### 5.3 Keyboard defaults (play context, 32)

Move x8 (preset), wait, stairs up/down, interact `E`, inventory `i`, craft `&`, build `*`,
character `@`, map `m`, journal `j`, look `x`, fire `f`, throw `t`, reload `r`, action wheel
hold `Tab`, quick-slot wheel hold `Q`, quick slots `1`-`8` (one action with an index), run
toggle `Shift`/`"`, crouch, sleep `$`, safe mode `!`, status report `F4`, lessons `F1`,
explain screen `F2`, reference `?`, command palette `Ctrl+Space`, pause `Esc`, zoom `z`/`Z`,
first person `Ctrl+F`, turn toward threat `Ctrl+G`. Every other action keeps its id and can
be bound; most legacy keys stay bound by default in a "Legacy" binding group that the player
can turn off in one row of the Controls tab (so veterans lose nothing on day one and new
players see a short list in the Reference).

### 5.4 Hold vs toggle

Holdable actions: run, crouch, aim mode, action wheel, quick-slot wheel, interact hold-list,
camera glance, map pan. Each has a per-action setting (Hold / Toggle / Default) in the
Controls editor; the global default is in Accessibility. Confirmation holds (long-press to
confirm destructive verbs) can be "press twice" instead.

## 6. Options architecture: in-game versus file

Rule for placing an option: it stays in-game if (a) a player changes it in a normal session
(language, volume, scale, HUD mode, controls, accessibility), or (b) it is an accessibility
need. Everything else is in the advanced file. Options that are "tuning" of a feature the
player already turned on (particle density, fade times, minimap pixel ratio, FOV tween) are
file-only. Debug and developer options are file-only. Duplicate signals are merged (five unit
options to one; nine minimap options to one toggle; three autosave options to one).

The advanced file is specified in section 9.

## 7. Accessibility

All items are in the Accessibility tab (11 options) plus the Controls editor. Presets set
several of them at once and are documented as such; a preset is never the only way to reach
an option.

### 7.1 Remapping

Every action, every device, in one editor (4.13). Modifiers and middle mouse capturable.
Conflicts shown and fixable inline. Presets include Left-handed (mirrored movement and
verbs) and One-handed (7.9). Mods that add actions appear in the same list. Bindings are
stored per device so a keyboard rebind never touches the gamepad.

### 7.2 Colour vision and non-colour redundancy

Rules first: every state carries (1) an icon shape or glyph, (2) a word, (3) a colour. Lists
that use colour for rarity/condition also show the word. Deltas show sign and arrow. Hostile
vs neutral vs friendly creatures show an icon on the prompt and on edge arrows.

Palettes: UI semantic tokens use the Okabe-Ito set, designed to stay distinct under
deutan, protan and tritan vision (hex from a secondary source; verify against
jfly.uni-koeln.de before shipping; see research g):

| Token | Use | Normal | Note |
|---|---|---|---|
| `danger` | hostile, damage, broken limb, critical need | #D55E00 vermillion | not pure red |
| `warn` | abnormal need, low stamina, warning | #E69F00 orange | |
| `good` | healthy, improving, can craft | #009E73 bluish green | |
| `info` | neutral facts, sound markers, notes | #56B4E9 sky blue | |
| `accent` | focus ring, selected tab, mission target | #F0E442 yellow | focus ring also has a 2 px dark outline |
| `special` | magic, bionic power | #CC79A7 reddish purple | |
| `muted` | remembered/unseen, disabled | #999999 | |

Colour vision modes (Deuteranopia / Protanopia / Tritanopia) remap the game's 16-entry
terminal palette (`data/raw/colors.json` → `windowsPalette`, used by every curses-drawn
glyph and by `cata_tiles.cpp:3406`) to a mode-specific table, and also remap the 3D palette
tints (hostile outline, remembered sepia, hit glow). The table is data (`data/raw/
colors_<mode>.json`) and is validated in CI by simulating each mode (daltonlens or the
Machado 2009 matrices) and asserting the pairwise colour distance of the semantic tokens. "High
contrast" is a fourth mode: UI on black with the token set at full saturation, and in the 3D
scene hostiles, items and interactables tinted with tokens while the rest desaturates (TLOU2
style).

Contrast: text ≥ 4.5:1 against its background, UI components ≥ 3:1 (WCAG 2.2 SC 1.4.3 and
1.4.11), checked by a Catch2 test over the theme tokens.

### 7.3 Screen reader and self-voicing

Architecture (detail in `04_architecture.md` §5): the widget tree carries, per node, a role,
a name, a value, a state and an optional description. A **narrator** walks it:

- Screen open: speak title, then a one-line hint ("Inventory. 3 sections. Tab to switch.").
- Focus change: speak "name, value, state, position" ("Survivor jacket, worn on torso, 1.8
  kilograms, 7 of 24"). Verbosity Low drops position and state.
- Value change on the focused control: speak the new value.
- HUD events (narration = Menus and log): new log lines, badge appear/disappear ("Cold"),
  "Entering: place", interrupt popups. Combat text is not spoken separately; the log line is.
- Cursor mode: speak the tile card on every move ("Door, closed. Zombie, hostile, 3 tiles
  north-east."). This is the blind-navigation path for the map.
- Keys: F4 speaks the status report (already text); `Ctrl+R` repeats the last utterance;
  `Ctrl+Up/Down` reads the previous/next line of the current card without moving focus.
- Queue rules: focus and screen changes interrupt; log lines queue; a volume and rate slider.

Speech backends (research h): Windows: NVDA controller client (LGPL-2.1, dynamically loaded)
when NVDA is running, else SAPI 5 via COM; Tolk is not used (its README says it is not being
developed). macOS: `AVSpeechSynthesizer` through a small Objective-C++ file. Linux:
`libspeechd` loaded with `dlopen` (present on this machine, 0.12.1; Flatpak/SteamOS access to
the daemon is unverified). SDL3 has no speech API. This is self-voicing; it works with or
without a screen reader. A second phase exposes the same tree through AccessKit's C API
(Apache-2.0 or MIT; `accesskit-c` repository exists) so JAWS/NVDA/VoiceOver/Orca can navigate
the UI natively; this is deferred until the self-voiced UI is complete, because the
accessibility tree it needs is the same tree.

Trade-off stated plainly: self-voicing gives blind players a playable game soonest and does
not depend on the still-experimental AccessKit adapters, at the cost of a second speech
stream when a screen reader is also running (mitigated: when NVDA/JAWS/Orca is detected the
narrator routes through it instead of SAPI/speechd).

### 7.4 Text and UI scale

One slider, 75-200 % in 5 % steps, applied live (no restart) to every new-layer screen and the
HUD. Floor is computed so the smallest glyph is ≥ 9 px at 1280x800 (Steam Deck rule), target
12 px. The map/tile zoom stays a separate in-play key. Fonts: default UI font plus "Readable
font" (Atkinson Hyperlegible; dyslexia-specific fonts are not shipped, the evidence for them
is weak; research i §8), CJK/emoji fallbacks via `TTF_AddFallbackFont`. Every screen reflows:
cards wrap, lists scroll, nothing truncates (P4). Screenshot tests at 100 % and 200 %.

### 7.5 Reduced motion and flashing

One switch: camera easing off, HUD fades replaced by instant show/hide, hit-direction pulse
replaced by a static chevron for one turn, lightning flash removed (sky brightens without the
frame flash), screen shake (if any) off, precipitation particle count halved. The game's
baseline must already pass WCAG 2.3.1 (≤ 3 flashes per second); the lightning flash and the
hit glow are the two effects to measure (research g §6).

### 7.6 Audio cues as text

"Sound cues as text" shows a subtitle line for every heard sound in the message ticker with
direction and distance words ("You hear glass breaking, east, close"), speaks it when the
narrator is on, and keeps the in-world sound markers (shipped). NPC speech is already text.
Music and ambient cues carry no information in BN, so nothing else is needed.

### 7.7 Cognitive load

HUD Minimal, Guided hints, the Reference's "explain this screen" (F2), consistent navigation,
no timed popups, one verb per interaction, lessons re-readable in the Journal. "Confirmations:
All" adds a confirm step to every irreversible verb.

### 7.8 Hold vs toggle

Section 5.4. Also: no action requires two simultaneous inputs; the `LT` gamepad modifier and
keyboard modifiers can be set to "sticky" (press, then press the key) in the Controls tab
(this is one row: Sticky modifiers Off/On, counted within the Controls editor, not the 39).

### 7.9 One-handed and switch-friendly play

- **One-handed presets** (left and right): movement on one hand's cluster, verbs on the same
  cluster, radials for everything else; nothing on the far side of the keyboard.
- **Mouse-only play** is possible: click-to-travel, right-click verb list, hotbar and wheels
  clickable, every screen clickable, scroll wheel for lists. BN is turn-based, so speed is not
  an issue.
- **Single-switch scanning** is not built in (it is a large, separate feature); instead the
  command palette guarantees every action is reachable by typing, which works with external
  switch-to-keyboard tools, and the Reference lists every action. Stated as a limitation.

## 8. Theming

Two shipped themes: Dark (default) and High contrast; the Modern menu border option is
subsumed. Theme = a JSON token file (colours, spacing scale, corner radius, border width,
font family) in `data/raw/ui/themes/`; the classic look is a third theme file (plain
borders, no padding) so "make it look like before" is a theme choice, not an option. Colour
vision modes layer over any theme.

## 9. The advanced config file

- **File:** `config/advanced.json` (next to the existing `config/options.json`).
- **Format:** one flat JSON object. Keys are option ids in lower case; values are the typed
  value. A `"//key"` entry before each option carries its description and allowed values, so
  the file is self-documenting without a comment syntax:

```json
{
  "//message_ttl": "Turns a message stays in the ticker. integer 1-1000. default 10",
  "message_ttl": 10,
  "//r3d_fp_tween_ms": "First-person step easing, milliseconds. 0 disables. default 90",
  "r3d_fp_tween_ms": 90
}
```

- **Generation:** on first run the game writes the file with every advanced option at its
  default and its description. `cataclysm-bn-tiles --dump-advanced-options` prints the same
  list as Markdown for `docs/en/user/advanced_options.md`, so the doc is generated, not
  maintained.
- **Reload:** read at launch, when the Settings screen closes, and when returning to the main
  menu. Options marked "restart" in their description apply at the next launch; the Settings
  screen shows a one-line "advanced.json changed; N settings need a restart" notice.
- **Validation:** unknown key → warning in the log, ignored; wrong type or out of range → default
  used, warning in the log; the file is never rewritten by the game after creation except by an
  explicit "Reset advanced settings" row under Settings > Game (that row is the 40th item and
  is a verb, not an option).
- **Migration:** existing `options.json` values for moved options are copied into
  `advanced.json` once, then ignored.

## 10. Cuts and merges (UI only, no rule changes)

Merged: 15 item screens → Inventory; 6 character screens → Character; 8 world/log screens →
Map + Journal + Cursor mode; 8 settings screens → Settings + Controls; 4 help screens →
Reference; 4 sidebar layouts + panel manager + pixel minimap → HUD (3D only).

Cut outright (UI): the action menu uilist (replaced by the wheel and the palette), the
examine item menu (verbs are on the item card), `}` panel manager in 3D, the advanced
inventory's 36-action context, per-screen navigation keys, the `LOOKAROUND_POSITION` /
`PICKUP_POSITION` / `SIDEBAR_POSITION` options, the two minimaps in favour of one, the
separate "list items" and "list monsters" screens, crafting search prefixes, construction
subcategories, `MORALE_STYLE` and `HEALTH_STYLE` (one style), `ITEM_SYMBOLS` (always on in
the new layer), the colour manager screen (replaced by theme files), the distraction manager
(its toggles become Rules > Safe mode rules rows).

Kept on the legacy cell-drawn path until M11, unchanged: the debug menu, Lua console,
computer terminals, loading screen, the in-game minigames, Defense mode screens (see below),
and the classic 2D sidebar. (The terminal build itself is retired; these screens draw through
the in-process cell canvas, `cursesport.cpp`, until they are ported or dropped.)

## 11. Feature cuts (last resort)

Each lists the alternatives rejected before proposing the cut. None is required for the
design; they reduce the long tail of legacy screens.

1. **Defense mode (and its Caravan shop).** Two bespoke curses screens (`DEFENSE_SETUP`,
   `CARAVAN` contexts) for a game mode outside the survival loop. Rejected alternatives:
   port to the new layer (a full screen's cost for a mode with no evidence of use); keep on
   the legacy path (cheap, but it keeps the old uilist stack alive and inconsistent with
   principle 6). Recommendation: keep on the legacy path, mark "Legacy" in the mode list, and
   cut only when the legacy stack is retired.
2. **Item minigames (Sokoban, Minesweeper, Lights on).** Three keybinding contexts and three
   screens for console items. Rejected alternatives: port (cost for no gain); replace the
   items' use with a morale effect only (a rule change for a trivial feature). Recommendation:
   same as 1.
3. **"Examine also picks up" (BN #9805).** Not a feature cut: the interaction prompt makes
   examine and pick up distinct verbs on the same target, which answers the complaint without
   changing what the player can do.

No other game-rule cut is proposed. The brief's permission to cut was not needed for the
design: the complexity players complain about is in the screens, not in the rules.
