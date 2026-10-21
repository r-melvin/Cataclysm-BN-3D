# First-person mode: review, research and roadmap (2026-10)

> Plan only. Nothing here is implemented. Outstanding work is tracked in
> [BACKLOG.md](../../../../BACKLOG.md); when an item below is picked up, add it there and link
> back to its `FP-nn` id. Implementation must follow [AGENTS.md](../../../../AGENTS.md).

Evidence tags used throughout:

- **[repo]** verified in source on `main` at `a478a0e761` (2026-10-21). Line numbers are
  approximate (`≈`); search for the named function instead of trusting the number.
- **[src]** taken from an external source listed in [Sources](#sources). Where the researcher
  only saw a search snippet, the entry says "snippet".
- **[spec]** my own inference or proposal, not verified anywhere.

---

## 1. Inventory: what first-person mode is on `main`

### 1.1 Enabling and persistence [repo]

| Thing | Where | Behaviour |
| --- | --- | --- |
| Camera mode option | `src/options.cpp` `R3D_CAMERA_MODE` (`orbit` / `first_person`, **default `first_person`**) | `r3d::sync_camera_mode()` (`src/r3d.cpp` ≈820) switches `camera::mode`, zeroes `look_pitch`, glance, `exact_fp_yaw`. |
| Toggle key | `data/raw/keybindings/keybindings.json` `r3d_first_person` = **Ctrl+F** → `ACTION_R3D_FIRST_PERSON` → `r3d::toggle_first_person()` | Flips the option and saves options. |
| Renderer forced on | `r3d::draw()` ≈2365: `forced_3d_on_start` sets `RENDERER_3D=true` on first draw | Quirk: the 2D option value is overridden every launch. Not FP-specific but affects "first person is default" perception. |
| Saved heading | `uistate.r3d_fp_heading` (`src/uistate.h`, `src/savegame_json.cpp` ≈5004/5066), `sync_saved_heading()` / `remember_heading()` in `r3d.cpp` | Heading survives save/load; `-1` when never set. |
| Other FP options | `R3D_FP_FOV` 60–110 (90), `R3D_FP_EYE_HEIGHT` 50–95 % (80), `R3D_FP_TWEEN_MS` 0–250 (0), `R3D_FP_COMPASS` off/heading/threats (threats), `R3D_FP_EDGE_ALERT` (on), `R3D_FP_CROSSHAIR` (on), `R3D_FP_USE_HEADING` (on), `R3D_FP_MINIMAP_ROTATE` (on), `R3D_FP_OCCLUDE_UNSEEN` (on), `R3D_WASD` (on) | All `COPT_CURSES_HIDE`, prerequisite `RENDERER_3D`. |

### 1.2 Camera model [repo]

`src/r3d_camera.h/.cpp`, `struct camera`:

- `heading` 0..7 = eighth turns clockwise from north (map convention, y south). `fp_yaw()` =
  `heading * 45°` (+ `glance_yaw` + `tween_yaw`), **or** `*exact_fp_yaw` + glance + tween when
  `exact_fp_yaw` is set (only while controlling a vehicle, see 1.8).
- `look_pitch` ±70° (`max_look_pitch` 1.2 rad) in 15° steps (`pitch_steps`); `tilt_pitch` ±45°
  when look mode (`u.view_offset.z`) is on another level; `glance_pitch` from mouse.
- `fovx` from option (horizontal, applied to the narrower side: `fp_fovy/fp_fovx`); near plane
  0.05, far 200; `eye_height` in stretched space on top of `surface_height()` of the tile.
- `zoom()` in FP has no distance: it toggles a ±45° "look at the level above/below" tilt.
- `rotate_steps(n)` wraps heading; `forward_eighths()` returns heading (or rounded exact yaw).
- Math helpers with pure tests: `src/r3d_first_person_math.h` (`relative_bearing`,
  `compass_offset`, `edge_alert_side`, `minimap_turn`, `slide_from_eye`), `src/r3d_hud_math.h`
  (`place_edge_mark`, `bearing_delta`, `camera_forward_bearing` — orbit only).

### 1.3 Movement and turning [repo]

- `input.cpp` `r3d_key_action()`: only in `DEFAULTMODE`, no modifiers, `R3D_WASD` on, 3D active.
  `w/s/a/d` → `UP/DOWN/LEFT/RIGHT` (two held keys → diagonal, read via `SDL_GetKeyboardState`);
  `q/e` → `r3d_rotate_left/right` **except** while `g->u.controlling_vehicle`; arrows → rotate /
  pitch. `input_context::get_direction()` then rotates the delta by `r3d::view_eighths()`
  (= `cam.heading` in FP), so W = step along facing, **A/D = strafe**, S = step back without turning,
  W+A = diagonal step. 
- One key press = one tile. Turning costs **no game time** (`r3d::rotate()` only changes the camera;
  BN has no facing). Turn step is **45°**, fixed. No quick-turn (180°) action.
- Easing: `update_fp_ease()` (`r3d.cpp` ≈878) slides position/yaw over `R3D_FP_TWEEN_MS`
  (default 0 = instant). Disabled while an activity/auto-move runs and while driving.
- `Ctrl+G` (`r3d_fp_face_threat`): faces the nearest visible hostile
  (`handle_action.cpp` ≈3296, `u.get_hostile_creatures( MAX_VIEW_DISTANCE )`), free, no-op while
  controlling a vehicle.

### 1.4 Mouse [repo]

- **No mouse capture / mouselook exists.** `SDL_SetWindowRelativeMouseMode` is only ever called
  with `false` (`sdltiles.cpp` ≈3280, 3308, 3358) as a defensive reset; `is_mouse_captured()`
  (`sdltiles.cpp` ≈4458) can only return false in practice. Capture was added in `ac54957c4c` and
  removed in `3b470163a1`. No commit message records why (both messages are one line).
- Middle-button drag = **glance**: `drag_orbit()` adds `glance_yaw` (clamped ±2.4 rad ≈ ±137°) and
  `glance_pitch`; button up → `end_glance()` snaps back. Shift+middle pan is a no-op in FP.
- Mouse motion → `r3d::pick(x, y)` → `set_hovered_tile()`; the hovered tile gets a `highlight`
  marker, and a hovered **item** tile is drawn 28 % larger, brighter, with a `cursor` sprite
  (`r3d_entities.cpp` ≈2161).
- Left click (`handle_action.cpp` ≈2057, any 3D camera): adjacent creature → move/attack action;
  self → examine; openable door/vehicle part → `ACTION_OPEN`; else `ACTION_EXAMINE`; farther →
  normal click-to-travel (only known/remembered terrain). Right click → examine/target when seen.
  No mouse actions while `veh_ctrl`.
- Picking: `pick_cell()` (`src/r3d_dda.cpp`, Amanatides–Woo through `picking_column_height`:
  terrain/furniture column, visible creatures = full height, visible item piles raise to
  surface+0.55) then **`raycast_from_player()`** (`r3d.cpp` ≈1150): walks `line_to(player, hit)`
  and returns the first tile whose column rises ≥0.45 above its surface and above the eye→target
  ray, so a pick "through" a counter or creature resolves to the obstacle. Picks resolve on the
  player's z (holes map to the crossing tile).

### 1.5 Keyboard interaction without the mouse [repo]

- `handle_action.cpp` ≈2132: for `EXAMINE/OPEN/CLOSE/PICKUP` with no mouse target, the target is
  the **hovered tile** if adjacent and `can_interact_at()` (`action.cpp` ≈918), else the **tile
  ahead** (`ACTION_MOVE_FORTH` rotated by heading), else the player's own tile.
- Direction prompts (`choose_direction`, `action.cpp` ≈1355): with `R3D_FP_USE_HEADING`, the key
  `/` (`r3d_fp_select_ahead`, categories DEFAULTMODE/LOOK/TARGET) returns the tile ahead. In look
  mode (`game.cpp` ≈10357) and aiming (`ranged.cpp` ≈3363) the same key moves the cursor to
  `r3d::crosshair()` (the tile under the screen centre).
- `game::peek()` (`game.cpp` ≈8996) is BN's own: it `setpos()`es the avatar to the peeked tile
  and opens `look_around`. **[spec]** Because `r3d::draw()` places the FP eye at
  `get_avatar().bub_pos()`, the FP picture should already move to the peeked tile during the
  look; the easing state (`fp_prev_pos`) will treat it as a step. Not verified in a session.

### 1.6 HUD [repo]

Two overlays are drawn over the GPU picture with the SDL 2D renderer:

1. `draw_hud()` (`src/r3d_hud.cpp`, option `R3D_HUD`): combat text, status badges (hunger,
   thirst, fatigue, pain), hit-direction red glow (`draw_hit_direction`, attacker **guessed** as the
   nearest visible hostile), "Entering: place" notice. **In FP it skips** `draw_edge_arrows()`
   (pointers to seen hostiles and heard sounds outside the picture) **and** `draw_compass()`
   (cardinals, **sun/moon sprites, wind chevron, mission target, auto-travel destination,
   nearby overmap notes**). `draw_compass` uses `camera_forward_bearing( cam.yaw )`, which is
   meaningless in FP; that is why it is skipped.
2. `draw_fp_hud()` (`src/r3d.cpp` ≈2150, FP only): compass strip spanning half a turn (8 direction
   labels; yellow ticks for sounds heard this turn whose post is out of the picture or occluded,
   from `s.heard_ticks`; red ticks for visible hostiles on the same z, height by distance, when
   `R3D_FP_COMPASS=threats`); edge alert bars (left/right/bottom) only for hostiles within
   **1 tile** (`get_hostile_creatures( 1 )`); "Looking: N level up/down"; a 4×4 crosshair dot.
   **The crosshair label was deliberately removed** in `e7fef314b1` (2026-10-20). The option text
   of `R3D_FP_CROSSHAIR` ("with the name of what is under it"), `docs/en/dev/guides/first_person.md`
   ("crosshair labels") and the backlog entry R3D-12 still describe the label.
   `r3d::crosshair_description()` (`src/r3d_knowledge.cpp`) is now dead code with a live test
   (`tests/r3d_knowledge_test.cpp`).
- Heard-sound posts: in FP, `sounds::get_footstep_markers()` positions become depth-tested marker
  posts (`entity_view::marker::heard`), and those not in view / occluded become compass ticks.
  Only positions are used; `sounds::sound_at()` descriptions are not shown.
- Minimap: `src/pixel_minimap.cpp` ≈483 rotates BN's pixel minimap disc heading-up in FP
  (`R3D_FP_MINIMAP_ROTATE`). No facing wedge is drawn; the beacon is the usual player dot.

### 1.7 Weapon and entities [repo]

- `draw_first_person_weapon()` (`r3d.cpp` ≈2301): the wielded item's **2D tileset sprite** is
  blitted bottom-right, mirrored, rotated −8°, with a bob derived from position and yaw. Hidden
  while unarmed or controlling a vehicle. Despite commit `a6a7ac6ebe`'s title ("3d weapon prism"),
  there is no 3D weapon geometry on `main` (`7292e32abd` replaced it with this HUD sprite).
- `build_entity_mesh()` (`src/r3d_entities.cpp`): in FP the avatar is not drawn; creatures, items
  and tall furniture are upright cylindrical billboards; sprites nearer than `min_depth` slide
  away from the eye (`slide_from_eye`). Monsters have no facing (R3D-06 in the backlog).
- World mesh: ceilings are emitted in FP (`mesh config.ceilings = first_person`); never-seen
  tiles are drawn solid black when `R3D_FP_OCCLUDE_UNSEEN` (`uniforms.flags[0]`); storeys above are
  always drawn; stairs get a special case in `r3d_mesh.cpp` ≈1998. Cutaway and avatar silhouette
  are off in FP. Lighting is BN's visibility cache (`light_sampler`), so flashlights, headlights
  and darkness already show as lit/unlit tiles.

### 1.8 Vehicles [repo]

- While `vp->vehicle().player_in_control( you )` (`r3d.cpp` ≈2546): `cam.exact_fp_yaw =
  vehicle_heading_to_fp_yaw( face )` (`src/r3d_vehicle_camera.h`), `heading` snapped to the nearest
  eighth, the eye is anchored to the **seat mount rotated about the pivot** (`172e0133f3`), easing
  off, `rotate()`/`face_towards()` return early, `q/e` keep their vehicle meaning, the driver's own
  windshield glass is not emitted (`clear_windshield`, `r3d_entities.cpp` ≈927; test
  `r3d_first_person_driver_sees_through_own_windshield`), the weapon sprite is hidden, vehicle
  cutaway is off.
- Not present: mirrors, any rear view, head turning while driving (arrows do nothing),
  interpolation between vehicle substeps (backlog R3D-15), multi-level vehicles (R3D-16).

### 1.9 Tests and tooling [repo]

- `tests/r3d_first_person_test.cpp` (21 cases: eye position, heading/forward, centre ray, FOV,
  pitch clamp, pick ahead, bearings, compass placement, edge-alert sectors, minimap turn, ceilings,
  clear line, sprite slide). `tests/r3d_test.cpp` (camera, DDA, vehicle yaw).
  `tests/r3d_vehicle_rules_test.cpp` (windshield). `tests/r3d_knowledge_test.cpp` (label memory).
  `tests/input_test.cpp` (`r3d_key_action` filters). `tests/r3d_hud_math_test.cpp`,
  `tests/r3d_hud_test.cpp` (renderer-state guard).
- `tests/r3d_geometry_snapshot_test.cpp` uses the **orbit** camera (`r3d::camera{}`) for all 17
  scenes, so FP-only branches gated on `cam.first_person()` do not move the snapshot; changes to
  shared entity/vehicle paths do.
- Run: `CATA_TEST_COMPUTE_ACCELERATION=cpu ./out/build/linux-full/tests/cata_test-tiles "[r3d]"`.
  New test files need a CMake reconfigure (glob). Capture tooling: `CATA_R3D_FP_SHOTS`.

### 1.10 Known gaps (verified unless tagged)

1. Picture ≠ perception: the game keeps 360° vision; the compass/edge cues are the only bridge.
   Edge alerts cover only adjacent hostiles; far hostiles outside the picture are compass ticks only.
2. FP loses the orbit compass's sun, moon, wind, mission, destination and note markers.
3. Crosshair label removed; option text and docs are stale; `crosshair_description` is dead.
4. No quick-turn, no 90° turn option, no mouselook/capture, no head-turn while driving.
5. Sound cues are positional only (no description, no category colour, no z hint on the strip).
6. Hit direction guesses the attacker (`guess_attacker`), so a hit from an unseen attacker lights
   the whole edge.
7. Minimap has no facing wedge; the pixel minimap is small and only shows the loaded bubble.
8. No interaction prompt: the player must know which key acts on the tile ahead. The "contextual
   interaction tooltips" of `ac54957c4c` were removed in `a6a7ac6ebe`.
9. Driving: no mirrors/rear view, no interpolation, no FOV change, no speed/heading readout in the
   picture.
10. No perception/injury/mutation effects on the picture beyond BN's own lit-level dimming.
11. Docs: `first_person.md` says "Ctrl+Tab" is unencodable and lists crosshair labels; neither
    matches current code.
12. **[spec]** `game::peek()` in FP untested; `look_around` with FP tilt on other levels untested
    beyond the R3D-12 acceptance notes.

---

## 2. Research findings (condensed)

Full per-game detail is in the three research reports summarised here; every URL is in
[Sources](#sources). Headline: **no shipped game is first-person, turn-based, grid-locked and a
persistent survival sandbox.** The nearest analogues each miss one axis (The Quest: FP grid
turn-based, no sandbox; Vaporum "stop time": FP grid, optional turn-like pacing; ROGUE-FP: FP
roguelike, gridless; Barony: FP roguelike, real-time; Daggerfall Unity: FP open world, real-time).
Jupiter Hell is **not** first-person (angled top-down 3D); its FAQ says camera rotation costs no
turn and facing is "just visual fluff" [src]. No CDDA/BN 3D renderer other than a Minetest
prototype ("Voxel CDDA") was found [src].

### 2.1 Grid-based first-person RPGs [src]

- **Genre convention is 90° turns on a 4-direction grid**; the Dungeon Crawler Jam defines the
  genre that way and rejected hex/60° proposals and mid-tile strafe-peeking. No source discusses
  45°/8-direction turning. The nearest evidence is Grimrock's mouselook layered over 90° turns and
  VR snap-turn guidance that users should pick the angle (15/30/45°) and that 22.5° was used in one
  study because larger snaps felt disorienting.
- **Praised**: auto-maps that fill in as you walk and persist (Might & Magic III, Ultima
  Underworld's annotatable automap, Daggerfall Unity's persistent map with notes, 3D view and a
  classic micromap); visible roaming enemies instead of random encounters (M&M III, Etrian FOEs
  shown on the map with learnable patrol/chase behaviour); an encounter gauge that changes colour
  (Etrian); passive threat cues (a spider's shadow spilling round a corner in Grimrock; Daphne's
  heartbeat/muffled audio when enemies are near); context cursors for interaction (Stonekeep's
  eye/hand cursor, Dungeon Master's "reach in and pick it up"); optional pause/stop-time
  (Vaporum); authenticity toggles (Wizardry remake presets, Grimrock old-school mode).
- **Complained about**: disorientation from identical walls and no landmarks (SMT I), spinners and
  rotation tiles that the map does not reflect (Wizardry 6, remake), maps that reset or are
  consumables (Wizardry remake, Elminage), hidden-switch pixel hunts (Grimrock, Vaporum), the grid
  feeling restrictive (Operencia items not lining up, Grimrock 2 outdoors, no diagonal movement),
  input overshoot on turns (Vaporum analogue deadzone, unintended 180s), one-shot messages that
  vanish (Wizardry remake), no enemy health feedback (EotB), the real-time "combat waltz" exploit
  (EotB, Grimrock), one Vaporum reviewer reporting nausea from grid movement.
- **Developer notes**: FTL chose real time "to put the player under the pressure of time"; the
  Digital Antiquarian frames the grid as a hardware compromise that became a feature (lockstep
  party, graph-paper mapping, emergent tricks); Grimrock's team tried turn-based and dropped it;
  Jeff Vogel notes a square grid's limited design space.

### 2.2 First-person roguelikes and viewers [src]

- **Barony**: praised for co-op and fair deaths; complaints are instant-kill ceiling traps,
  "clunky" combat, tedious inventory, and UI/view conflicts (side inventory narrows the view,
  minimap hidden while the inventory is open, cursor recentring, shared keys). Developer concern:
  "too much visual information can make the interface harder to use for newcomers"; fix was
  toggles. Stealth is light-based: sneaking improves dark vision, light exposes you.
- **Daggerfall / Unity**: disorientation in huge dungeons is answered by the automap (persisted,
  3D view default by request, micromap, notes, compass HUD), not by movement changes.
- **Ultima Underworld**: automap with notes was the feature the team wished got more credit.
- **Jupiter Hell** (3D, not FP): auto-interrupt when an enemy comes into view, a click sound so
  "wait" is felt, remaining enemies highlighted, default zoom makes distant enemies hard to read.
- **NetHack/Angband/DCSS/DF**: no first-person frontends except Armok Vision's "walking FPS mode"
  (a viewer, compatibility unverified).

### 2.3 Survival-sim UX lessons [src]

- **Project Zomboid**: zombies have a ~60° forward cone with reduced rear detection and
  hearing ranges by surface; the player's own view is a cone and the area behind goes dark. The
  Keen Hearing trait (hear what is behind you) is called a fix for "repeated 360° turns". Vehicle
  entry via a facing/distance-dependent highlight confuses new players; an explicit prompt is the
  lesson. Loot UI baseline: container list, take-all, take-same-type; players mod in grouping and
  labels. Bleeding must be the most visible status.
- **Tarkov / ZERO Sievert**: real-time looting delays are only tolerated when the world can punish
  them; in a turn-based game the cost must be game turns and be interruptible.
- **Fortnite "Visualize Sound Effects"**: a directional ring around the reticle coloured by sound
  category; accessibility guidance says every gameplay sound needs a visual twin. A Graphics
  Interface 2021 study found bold peripheral cues beat 3D arrows and faint cues were missed in busy
  scenes.
- **Thief light gem / Metro watch / 7 Days to Die eye / STALKER mods**: a simple persistent
  "how visible am I" readout fixes confusion about first-person lighting; mixing light and noise in
  one meter confuses players; diegetic readouts need a first-use prompt.
- **The Long Dark**: status shows only when something needs attention, with severity colours and
  trend arrows; players split on it, so offer always-on and a sensitivity option.
- **Injury effects**: vignettes/blur that flicker with actions cause complaints and headaches
  (Space Marine 2); keep effects steady, subtle, off the HUD-critical regions, with an off switch.
- **Driving view**: players prefer first person on foot and third person in vehicles when there are
  no mirrors or the FOV is narrow (Arma Reforger); racing players switch view by situation.
- **Darkness**: DayZ players game the gamma when night is unplayable; give a non-light way to see.
- **Comfort**: snap turning and FOV restriction each reduce VR sickness; combining them does not
  help further; Crystal Rift's hop-movement cured a reviewer's nausea.
- **Text in world**: legibility ≠ readability; plain type, few words, generous spacing.

---

## 3. Synthesis

### 3.1 Genre tropes vs. this fork

| Trope | Done well by [src] | On `main` [repo] |
| --- | --- | --- |
| Grid-locked step movement | Grimrock, Etrian, Wizardry | Yes (W/S, strafe A/D, diagonals) |
| Fixed-angle turning | 90° everywhere; VR: user-chosen 15/30/45° | 45° only; no 90° option; no quick 180° |
| Turn costs time? | Jupiter Hell: no (facing is fluff) | No (correct for BN; keep) |
| Free look / mouselook over snap turns | Grimrock (limited), Crystal Rift (look after each hop) | Middle-drag glance only; no capture |
| Strafe | Grimrock, Vaporum | Yes |
| Peek/lean | Rejected by DC jam; PZ peeking is BN's `peek` | BN `peek` exists; FP behaviour unverified |
| Automap, persistent, annotatable | Underworld, Daggerfall Unity, M&M III | BN overmap + map memory; pixel minimap (rotating); no notes on minimap |
| Compass / facing indicator | Daggerfall, EotB, Underworld | Compass strip; minimap lacks facing wedge; sun/moon/wind lost in FP |
| Visible-threat indicators off-screen | Etrian FOE map, Dying Light cones, Fortnite ring | Compass red ticks; edge bars only adjacent |
| Sound cues with direction/category | Fortnite ring, Hunt, Daphne heartbeat | Yellow ticks + posts, no category/description |
| Light/darkness, torches | Grimrock, Barony, DayZ | BN lighting already drives the picture; no "how lit am I" readout |
| Encounter warning / auto-interrupt | Etrian gauge, Jupiter Hell interrupt | BN's own safe-mode interrupt (not FP-specific) |
| Doors/secret doors/loot interaction | Stonekeep context cursor, DM pick-up | Click-to-interact, hover-to-target, `/` ahead; no prompt text |
| Bump-attack | All crawlers | BN moves into a creature = attack; click on adjacent creature = attack |
| Attack-from-behind rules | Grimrock flank rules | BN has none; out of scope (gameplay) |
| Ranged reticle | FPS staples | Crosshair dot + `/` in aim mode; no zoom for scopes |
| Weapon in view | Daphne, FPS | 2D sprite with bob |
| Enemy health feedback | EotB lacked it (complaint) | BN sidebar/`look`; nothing at the crosshair |
| Minimap fog / memory | Daggerfall, PZ fade | Map memory + black unexplored geometry |
| Vehicle view, mirrors | Arma (wants mirrors), racing (view switch) | Exact yaw, windshield cleared; no mirrors, no view switch, no head turn |
| Status: pull not push | The Long Dark | Badges only when abnormal (similar) |

### 3.2 Ideas to port, adapted to BN

Numbered as `FP-nn`; the roadmap (3.5) gives recipes.

- **Orientation**: facing wedge on the minimap (FP-06); sun/moon/wind/mission/note markers on the FP
  compass (FP-03); quick 180° turn (FP-04); selectable turn step 45°/90° (FP-05); landmarks by
  label: crosshair label (FP-01) and hovered-tile label (FP-08).
- **Awareness**: bold edge pointers for every seen hostile and heard sound, not just adjacent
  (FP-07); sound ticks carry description and z (FP-09); hit direction from the real attacker when
  BN's event has one, else keep the guess (FP-10); a "visibility" readout from BN's light level at
  the player and recent noise (FP-15).
- **Interaction**: a prompt "[e] examine  [o] open  [g] pick up" for the tile the next key will act
  on (FP-11), using `can_interact_at()` and the real bindings; vehicle entry prompt is the same
  mechanism applied to `ACTION_CONTROL_VEHICLE`/`ACTION_OPEN` on vehicle parts.
- **Vehicles**: head-turn glance while driving (FP-12); optional orbit camera while driving
  (FP-13); driving readout (speed/heading) in the picture (FP-14).
- **Comfort**: keep instant steps as default; expose `R3D_FP_TWEEN_MS` as "comfort" with a short
  default only if the user wants (open question Q4).

### 3.3 Original ideas (bold ones marked)

- **FP-15 Visibility/noise readout** (Thief gem adapted): light at the player's tile from
  `map::ambient_light_at()`/`light_at()` and the last noise the player made. Presentation only.
- **FP-16 Vision-state overlays**: boomered (magenta tint), `effect_darkness`, high pain
  (vignette), `sight_impaired()` (grain), blindness (black). Steady, subtle, behind an option.
- **FP-17 Scope/binocular zoom in aim mode**: narrow `fovx` while aiming with a `ZOOM`-flag item.
- **BOLD FP-18 Rear-view mirror**: a second small render with yaw+180° while driving a vehicle
  that has a mirror-like part. Costly (second scene pass); deferred.
- **BOLD FP-19 Peripheral darkening by perception**: shade the picture's outer band when
  `per_cur` is low or `sight_impaired()`; must not hide HUD. Risk of "flicker" complaints [src].
- **BOLD FP-20 Diegetic driving dashboard**: speed, fuel, headlights drawn as a strip in the
  windshield region while controlling.
- **BOLD FP-21 Sound description captions**: short captions ("a crash to the north-east") stacked
  under the compass for sounds this turn, from `sounds::sound_at()`.
- **BOLD FP-22 Mouselook with snapping**: captured mouse turns the glance continuously and
  commits whole 45° steps when the glance passes 22.5°. Previously removed; see Q2.
- **BOLD FP-23 Look-mode "scout" tilt**: in look mode, the crosshair label becomes BN's
  `look_around` description line so FP look does not need the side window.
- Flashlight cone, headlights, night: already represented by BN lighting; no renderer work. The
  "non-light way to see" lesson is BN's night vision/mutations, already in the light model.

### 3.4 Where first person hurts a tile-based turn-based game (and what to reject)

- **The picture hides 2/3 of what the character perceives.** Every awareness feature above is a
  patch for that. Turning is free, so the "honest" fix (face everything) costs only key presses,
  and the compass/edge cues must stay bold. Rejected: making turning cost time, or restricting the
  character's vision to the picture. Both change BN's rules and break monster/NPC symmetry.
- **45° steps are outside genre convention and slower to reason about than 90°** [src: no source
  discusses them; VR guidance prefers user choice]. Keep 45° because BN moves in eight directions
  and a diagonal step must be "forward"; add 90° as an option, not a default (FP-05).
- **Mouselook on a grid is a trap**: a free yaw that does not match the eight headings makes
  "forward" ambiguous and was removed once already. If reintroduced, it must snap (FP-22) and stay
  off by default.
- **Real-time flourish (head bob, weapon sway, step easing) buys nothing in a turn-based game and
  can nauseate** [src]. Keep easing default 0; keep the weapon bob small or make it optional.
- **Peeking/leaning mid-tile** is rejected by grid-crawler convention and would need gameplay
  changes; BN's `peek` already covers the mechanic.
- **Third-person/over-the-shoulder** was requested in vehicle contexts only; the orbit camera
  already exists, so the cheap answer is a per-situation switch (FP-13), not a new camera.
- **Rear-view mirror / second pass** doubles scene cost on a renderer that already rebuilds entity
  and vehicle meshes every frame [repo]. Defer behind a measurement.
- **Diegetic-only HUD** is "nearly hopeless" in an FPS [src]; keep the mixed HUD.
- **Automap drawing by hand (Etrian)**: charming, but BN's map memory and overmap notes already
  exist; do not build a parallel map.

### 3.5 Prioritised roadmap

Effort: S < ½ day, M 1–2 days, L > 2 days for an agent with build access. Every item: format
touched C++ with `bash build-scripts/format-cpp.sh <files>`; build
`cmake --build --preset linux-full --target cataclysm-bn-tiles cata_test-tiles`; run
`CATA_TEST_COMPUTE_ACCELERATION=cpu ./out/build/linux-full/tests/cata_test-tiles "[r3d]"`
(plus `"[input]"` when `input.cpp` changes) and
`./out/build/linux-full/tests/cata_test-tiles r3d_geometry_matches_snapshot`; commit atomically
with a Conventional Commits subject (`feat(r3d): …`, `fix(r3d): …`, `docs(r3d): …`).
General conventions that apply to every recipe (from AGENTS.md): `auto` + trailing return types,
`const auto`, designated initialisers, options structs for >3 parameters, anonymous namespaces,
`_( "text" )` for strings, `// *NOPAD*` on reference returns, keep unused parameter names as
comments, do not touch unrelated formatting, pure math in a header with no SDL/game types so it can
be unit-tested.

#### Phase 0 — fixes and parity (all S, independent, can run in parallel)

**FP-01 Restore the crosshair label behind an option value**
- Value: landmarks and loot identification at a glance; fixes stale docs/option text; revives dead
  code. Depends on Q1 (the label was removed deliberately one day earlier).
- Files: `src/options.cpp` (`R3D_FP_CROSSHAIR`), `src/r3d.cpp` (`draw_fp_hud`),
  `docs/en/dev/guides/first_person.md`.
- Recipe:
  1. In `options.cpp` replace the bool `R3D_FP_CROSSHAIR` with a string option: values
     `{ "off", "dot", "label" }`, default `"dot"`; text: "A dot in the middle of the first-person
     view; Label also names what is under it (what you see, or what you remember)."
  2. In `draw_fp_hud` (`r3d.cpp`, the block `if( get_option<bool>( "R3D_FP_CROSSHAIR" ) )`):
     read `const auto crosshair = std::string( get_option<std::string>( "R3D_FP_CROSSHAIR" ) );`,
     draw the dot when `crosshair != "off"`, and when `crosshair == "label"` re-add the six lines
     removed by `e7fef314b1` (`git show e7fef314b1`): `crosshair()` → `crosshair_description(
     get_avatar(), here, *at )` → `text( { .x = cx, .y = cy + 8.0f, .text = label, .color = white } )`.
     `here` is `get_map()` (already bound in `draw`; add `auto &here = get_map();` inside the
     function if not in scope). Include `r3d_knowledge.h` (already included).
  3. Any other reader of `R3D_FP_CROSSHAIR` (`rg R3D_FP_CROSSHAIR src`) must switch to the string.
  4. Docs: update `first_person.md` to describe the three values.
- Verify: existing `tests/r3d_knowledge_test.cpp` passes; add a test in
  `tests/r3d_first_person_test.cpp`? Not possible (SDL); instead a private session screenshot via
  `CATA_R3D_FP_SHOTS` with the option set to `label`. Geometry snapshot unaffected.

**FP-02 Sync docs and option text with code**
- Files: `docs/en/dev/guides/first_person.md`, `src/options.cpp` (`R3D_FP_CROSSHAIR` text if FP-01
  is rejected), `BACKLOG.md` R3D-12.
- Recipe: remove "crosshair labels" unless FP-01 lands; replace the "Ctrl+Tab" sentence with the
  actual bindings (`Ctrl+F`, `Ctrl+G`, `/`); state that mouse capture does not exist; state that
  glance is middle-drag; mention `q/e`.
- Verify: `rg -n 'Ctrl\+Tab|crosshair label' docs src` returns nothing stale.

**FP-03 FP compass parity: sun, moon, wind, mission, destination, notes**
- Value: orientation outdoors (sun position is the strongest natural compass); mission/destination
  pointers are lost in FP today.
- Files: `src/r3d_hud.cpp` (`draw_compass`, `draw_hud`), `src/r3d_hud_math.h/.cpp` (new pure
  helper), `src/r3d.cpp` (`draw_fp_hud`), `tests/r3d_hud_math_test.cpp`.
- Recipe:
  1. Add to `r3d_hud_math.h`: `auto view_forward_bearing( const camera &cam ) -> float;` — **no**:
     that header must stay free of game types but `camera` is a pure r3d type, acceptable. Body:
     `return cam.first_person() ? wrap to [0, 2π) of cam.fp_yaw() : camera_forward_bearing( cam.yaw );`
     (`fp_yaw()` is already "radians clockwise from north"; wrap with `std::fmod` and add 2π if
     negative).
  2. In `draw_compass` replace `camera_forward_bearing( frame.cam.yaw )` with
     `view_forward_bearing( frame.cam )`. Also make the tape width follow the FP span: in FP use
     `half_w` so that `π` radians map to the same width `draw_fp_hud` uses
     (`strip_w = max( 120, w*0.4 )`, i.e. `half_w = strip_w * 0.5`), so markers from both functions
     land on the same x for the same bearing.
  3. In `draw_hud`, call `draw_compass` in FP too (remove the `!frame.cam.first_person()` guard
     for the compass only; keep `draw_edge_arrows` gated until FP-07).
  4. In `draw_fp_hud` stop drawing the band and the 8 direction labels when `R3D_HUD` is on (the
     orbit tape now draws them), keep the sound and threat ticks; `top` must equal the tape's `top`
     (`frame.y + 4`) and tick y must sit at the tape's bottom. When `R3D_HUD` is off, keep the
     current FP strip.
  5. Test (`tests/r3d_hud_math_test.cpp`): `view_forward_bearing` for an FP camera at each heading
     equals `heading * π/4`; for `exact_fp_yaw = -π/2` returns `3π/2`.
- Risks: two overlays drawing on the same strip; keep one owner per element. Verify in a session
  with `CATA_R3D_FP_SHOTS` at noon (sun on the tape) and with an active mission.

**FP-04 Quick turn (180°)**
- Files: `src/action.h` (enum after `ACTION_R3D_FACE_THREAT`: `ACTION_R3D_TURN_AROUND`),
  `src/action.cpp` (`PAIR( ACTION_R3D_TURN_AROUND )` in the name map ≈214; `action_ident` string
  `"r3d_fp_turn_around"` ≈541; add to the no-worldstate list ≈668; add to `register_actions`
  list ≈1267), `src/handle_action.cpp` (case → `if( r3d::first_person() ) { r3d::rotate( 4 ); }`),
  `data/raw/keybindings/keybindings.json` (entry after `r3d_fp_face_threat`, name
  "3D view (first person): turn around", category `DEFAULTMODE`, default binding `CTRL+X`? — pick an
  unused chord; verify with `rg '"key": "CTRL+X"' data/raw/keybindings/keybindings.json`),
  `tests/input_test.cpp` (binding resolves: `inp_mngr.get_first_char_for_action(
  "r3d_fp_turn_around", "DEFAULTMODE" )`), `tests/r3d_first_person_test.cpp`
  (`cam.rotate_steps( 4 )` from each heading gives `( h + 4 ) % 8` and `forward_eighths()` agrees).
- Note `r3d::rotate( dir )` calls `rotate_steps( dir )`, so passing 4 works unchanged; it is a
  no-op while controlling a vehicle (correct).
- Also add `'x'`-style single-key remap? No: `r3d_key_action` remaps only w/a/s/d/q/e/arrows;
  leave it to the binding.

**FP-05 Turn-step option (45° / 90°)**
- Files: `src/options.cpp` (new `R3D_FP_TURN_STEP`, int, values 45 or 90 via a string option
  `{ "45", "90" }`, default `"45"`, prerequisite `RENDERER_3D`), `src/r3d.cpp` (`rotate()`:
  `const auto step = get_option<std::string>( "R3D_FP_TURN_STEP" ) == "90" ? 2 : 1;`
  `s.cam.rotate_steps( dir * step );` only in FP), `src/r3d_first_person_math.h/.cpp`
  (optional pure `turn_steps( bool ninety, int dir ) -> int`), test in
  `tests/r3d_first_person_test.cpp`.
- Risk: in 90° mode a saved odd heading stays odd (NE/SE/…); acceptable, document it. Do not snap.

**FP-06 Facing wedge on the rotating minimap**
- Files: `src/pixel_minimap.cpp` (after the rotated-disc `RenderCopy`/geometry draw ≈483–520),
  no options.
- Recipe: draw a small triangle (3 `SDL_Vertex`, `SDL_RenderGeometry`) above the beacon centre
  `(cx, cy)`: apex at `(cx, cy - r*0.35)`, base at `(cx ± 3, cy - 4)`, colour white alpha 200. In
  north-up mode (`R3D_FP_MINIMAP_ROTATE` off) rotate the triangle by
  `heading_bearing( r3d::view_eighths() )` (screen y down; use `turn_on_screen` from
  `r3d_first_person_math.h`). Only when `r3d::first_person()`.
- Test: pure rotation already covered by `r3d_minimap_turns_the_heading_up`; add a case for the
  wedge apex position for heading 2 (east) in north-up mode using `turn_on_screen`.

#### Phase 1 — awareness (M, mostly independent)

**FP-07 Bold edge pointers for every seen hostile and heard sound in FP**
- Value: the biggest first-person complaint is "what is behind me"; edge bars today cover one tile.
- Files: `src/r3d_hud.cpp` (`draw_hud`: call `draw_edge_arrows` in FP too), `src/r3d.cpp`
  (`draw_fp_hud`: keep the adjacent bars, they are the "urgent" tier), `tests/r3d_hud_math_test.cpp`.
- Recipe:
  1. `draw_edge_arrows` already uses `frame.clip()` + `place_edge_mark()` which mirrors things
     behind the camera, so it works for FP without changes. Remove the FP guard in `draw_hud`.
  2. FP arrows should be bigger: in `draw_edge_arrows`, `const auto base = frame.cam.first_person()
     ? 20.0f : 14.0f;` and use `base` in the size clamp. (Bold beats faint [src].)
  3. Sounds: `draw_edge_arrows` also points at `get_footstep_markers()`; `draw_fp_hud` already
     draws compass ticks for the same. Keep both only if the tape (FP-03) and arrows do not fight;
     otherwise skip sound arrows in FP (`if( frame.cam.first_person() ) { /*sounds on the tape*/ }`).
  4. Test: `place_edge_mark` with `clip_w < 0` (behind) lands on the bottom edge? It mirrors —
     assert the existing behaviour in `r3d_edge_marks_sit_on_the_border_and_point_out` for an FP-like
     narrow margin; no new math.
- Verify: session with a hostile behind the player: a red arrow at the bottom edge plus the red
  adjacent bar when within one tile.

**FP-08 Hovered-tile label**
- Files: `src/r3d.cpp` (`draw_fp_hud_options` gains `const hud_frame &frame`; call site in `draw`
  passes the `frame` built just before `draw_hud`), `src/r3d_knowledge.cpp` (reuse
  `crosshair_description`).
- Recipe: if `s.hovered && *s.hovered != crosshair tile` and the label value of FP-01 is on,
  `frame.screen( *s.hovered, 0.6f )` → push a boxed `hud_text` 10 px above the point with
  `crosshair_description( get_avatar(), here, *s.hovered )`. Skip when empty. Depends on FP-01.
- Verify: hover a remembered but unseen tile shows the remembered name (test exists for the
  function); hover a seen zombie shows `disp_name()`.

**FP-09 Richer sound ticks**
- Files: `src/r3d.cpp` (`state::heard_ticks` → `std::vector<heard_tick>` with `pos`, `dz`,
  `description`), `draw_fp_hud` (tick colour: same z yellow, above light blue, below orange; when
  `R3D_FP_COMPASS == "threats"` and the tick is the nearest, draw its `sounds::sound_at( p )`
  description centred under the tape, boxed), `src/r3d_first_person_math.h` unchanged.
- Recipe: where `s.heard_ticks.push_back( p )` happens (`r3d.cpp` ≈2759) push
  `{ .pos = p, .dz = p.z() - player_z, .description = sounds::sound_at( p ) }`. Limit captions to
  the two nearest; truncate to 40 chars with `utf8_truncate`.
- Verify: fire a gun off-screen in a private session; a caption appears for one turn.

**FP-10 Hit direction from the real attacker**
- Files: `src/r3d_hud.cpp` (`hud_events::guess_attacker`).
- Fact [repo]: `event_type::character_takes_damage` (`src/event.h` ≈257) carries only
  `character` and `damage`; there is no attacker field and no melee event with an attacker, so the
  direction must stay a guess.
- Recipe: in `guess_attacker`, first look for a visible hostile with `rl_dist <= 1` (melee range,
  the overwhelmingly common case); only if none, fall back to the nearest visible hostile (ranged
  attacker). Current code already takes the nearest visible hostile, so this only changes the
  tie-break order; it is a tidy-up with low value. S.
- Verify: unit test is impractical (event bus + SDL); session check.

#### Phase 2 — interaction (M)

**FP-11 Interaction prompt for the tile the next key acts on**
- Value: removes the "which key" guess; replaces the removed tooltips; vehicle entry prompt
  (PZ lesson [src]).
- Files: new `src/r3d_interaction.h/.cpp` (pure-ish: takes `const map &`, `const avatar &`,
  tile → `std::vector<interaction_hint>{ action_id, label }`), `src/r3d.cpp` (`draw_fp_hud` or
  `draw_hud`: render as one line under the crosshair: `"[e] Examine  [o] Open  [g] Pick up  [^]
  Drive"`), `tests/r3d_interaction_test.cpp` (new; reconfigure CMake).
- Recipe:
  1. `auto interaction_hints( const tripoint_bub_ms &p ) -> std::vector<interaction_hint>`:
     for each of `ACTION_OPEN, ACTION_CLOSE, ACTION_EXAMINE, ACTION_PICKUP, ACTION_CONTROL_VEHICLE,
     ACTION_BUTCHER` call `can_interact_at( action, p )` (`src/action.h`; `ACTION_BUTCHER` only on
     the player's tile) and collect `{ action, label }` where `label` is
     `press_x_if_bound( action )` (`src/action.h` ≈545: returns "Press o to open" style text and
     `std::nullopt` when unbound; skip unbound actions). There is no bare `action_name()` in BN;
     if a shorter label is wanted, use `inp_mngr.get_action_name( action_ident( action ) )` after
     checking that method exists (`rg -n 'get_action_name' src/input.h`).
  2. Key character: `inp_mngr.get_first_char_for_action( action_ident( action ), "DEFAULTMODE" )`
     (pattern used in `tests/input_test.cpp`); if `R3D_WASD` is on and the char is one of
     `w a s d q e`, show the action menu hint instead (those keys are remapped by
     `r3d_key_action`).
  3. Target tile = the same resolution as `handle_action.cpp` ≈2132: hovered if adjacent and
     interactable, else the tile ahead, else own tile. Factor that resolution into
     `r3d_interaction.cpp` as `auto interaction_target( … ) -> std::optional<tripoint_bub_ms>` and
     have `handle_action.cpp` call it, so the prompt and the key agree (one source of truth).
  4. Draw: `texts.push_back( { .x = cx, .y = cy + 24, .text = line, .color = 7, .align = 0,
     .boxed = true } )`. Only when `R3D_FP_CROSSHAIR != "off"`.
  5. Test: build a small map (`clear_map()`, `here.ter_set( p, t_door_c )`), place the avatar
     adjacent, assert hints contain `ACTION_OPEN`; put an item on the floor → `ACTION_PICKUP`; spawn
     a car with controls and board → `ACTION_CONTROL_VEHICLE`.
- Risk: `can_interact_at( ACTION_CLOSE )` calls `map::close_door( p, inside, check_only = true )`
  (`src/map/map.h` ≈1444), so it does not mutate. Budget: these checks run per frame; they are
  cheap but gate on the hovered/ahead tile changing (cache by `(tile, calendar::turn)`).

**FP-12 Head-turn glance while driving**
- Files: `src/r3d.cpp` (`rotate()`): when `s.cam.first_person() && you.controlling_vehicle`,
  instead of `return`, do `s.cam.glance_yaw = std::clamp( s.cam.glance_yaw + dir * eighth, -π/2,
  π/2 )`; `end_glance()` when the vehicle moves (in `draw`, if `is_controlling` and the vehicle's
  position or `face` changed since last frame → `end_glance()`), and on `Ctrl+R`.
- Test: `tests/r3d_test.cpp` has `r3d_vehicle_heading_to_camera_yaw`; add a pure check that
  `fp_yaw()` with `exact_fp_yaw` + `glance_yaw` sums (already true by construction; one assert).
- Verify: session: press left arrow while driving, the view turns 45° left, the next
  acceleration snaps back.

**FP-13 Orbit camera while driving (option)**
- Files: `src/options.cpp` (`R3D_FP_DRIVING_CAMERA` `{ "first_person", "orbit" }`, default
  `first_person`), `src/r3d.cpp` (`sync_camera_mode`: want FP = option FP **and not**
  (controlling && driving option == orbit); must not write `R3D_CAMERA_MODE`; keep the FP heading
  when switching back: `reset()` logic already preserves `heading`).
- Verify: toggling control (`^`) swaps cameras; `uistate.r3d_fp_heading` unchanged after a drive
  (assert in a `[r3d][vehicle][tiles]` test modelled on
  `r3d_first_person_driver_sees_through_own_windshield`: set the option, board, call
  `sync_camera_mode` indirectly via `r3d::draw`? `draw` needs a GPU — so test the pure decision
  function: extract `auto wants_first_person( bool option_fp, bool controlling, bool drive_orbit )
  -> bool` and unit-test it).

**FP-14 Driving readout**
- Files: `src/r3d.cpp` (`draw_fp_hud`): when `you.controlling_vehicle`, one boxed line under the
  tape: `string_format( _( "%s  %d %s" ), heading name, speed, units )` using
  `vehicle::velocity`, `convert_velocity( …, VU_VEHICLE )`, `vehicle::face.dir()` → direction via
  `eighths_towards`-style rounding. S. Verify in session.

#### Phase 3 — perception and atmosphere (M–L, optional, after Q5)

**FP-15 Visibility/noise readout**
- Files: new `src/r3d_awareness.h/.cpp` (`auto exposure( const map &, const Character & ) ->
  struct { int light_level; int last_noise; }`), `src/r3d_hud.cpp` (two small bars bottom-right:
  "Lit" from `here.ambient_light_at( pos )` mapped through `lit_level`, "Noise" from
  `you.volume` (`character.h` ≈2678; confirm it is the "noise this turn" field)), option
  `R3D_FP_EXPOSURE` bool default off. Separate the two bars (7DtD lesson [src]).
- Test: pure mapping `light → 0..3` in `tests/r3d_first_person_test.cpp`.

**FP-16 Vision-state overlays**
- Files: `src/r3d_hud.cpp` (`draw_vision_state( renderer, frame )` after `draw_hit_direction`,
  reusing the gradient-quad approach of `draw_hit_direction`), option `R3D_FP_VISION_EFFECTS` bool
  default off.
- Mapping (presentation only, never changes visibility): `has_effect( effect_boomered )` →
  magenta tint α60; `effect_darkness` → vignette α120; `get_perceived_pain() >= 40` → vignette α
  scaled to 160 at 100 pain; `is_blind()` → nothing extra (BN already blacks the map);
  `sight_impaired()` → thin grey border. Steady (no per-action toggling) and never over the tape,
  badges or crosshair label band (clip inner 80 %).
- Verify: session with `debug` effects; screenshot comparison; no test possible beyond the
  renderer-state guard test.

**FP-17 Scope zoom while aiming**
- Files: `src/r3d.h/.cpp` (`auto set_aim_zoom( float factor ) -> void;` stored in `state`,
  applied to `s.cam.fovx` in `draw` after reading the option; `1.0f` resets), `src/ranged.cpp`
  (target UI: on entry, if `r3d::first_person()` and the wielded gun has `flag_ZOOM` or a scope
  mod with magnification, call `set_aim_zoom( 0.5f )`; on exit `set_aim_zoom( 1.0f )` via
  `on_out_of_scope`).
- Risk: picks use `s.shown.cam`, which carries the zoomed `fovx` after the next frame, so the
  crosshair stays correct; the `/` select uses `crosshair()` → consistent.
- Test: `fp_fovx()` with the halved `fovx` in `r3d_first_person_projection_keeps_the_horizontal_field_of_view`
  style; session check that the aim cursor and the picture agree.

**BOLD FP-18 Rear-view mirror** — L, deferred: needs a second `begin_pass` into a separate target
in `r3d_gpu`, a second chunk visibility pass and culling for yaw+π, then a blit into a corner. Do
not start before measuring the current frame cost with `CATA_R3D_DEBUG_CHUNKS`.

**BOLD FP-21 Sound captions** — covered by FP-09 step "description".

**BOLD FP-22 Snapping mouselook** — M, off by default, after Q2:
- Files: `src/sdltiles.cpp` (`SDL_EVENT_MOUSE_MOTION`: when option on, FP, DEFAULTMODE,
  window focused and no menu: `SDL_SetWindowRelativeMouseMode( window, true )`; feed `xrel` into
  `r3d::mouselook( dx, dy )`; release on `SDL_EVENT_WINDOW_FOCUS_LOST`, `Escape`, Alt, or leaving
  DEFAULTMODE — the three existing `false` sites stay), `src/r3d.cpp` (`mouselook`: add to
  `glance_yaw`; while `|glance_yaw| > eighth/2` do `rotate( sign )` and `glance_yaw -= sign *
  eighth` so the heading commits in whole steps and the picture is continuous), option
  `R3D_FP_MOUSELOOK` bool default off.
- Hard rule from AGENTS.md: apply all deltas in the poll loop, redraw once per batch, finish via
  `refresh_display()`; do not render inside `SDL_PollEvent` draining.
- Test: pure `commit_mouse_turn( float &glance, int &heading ) -> int steps` in
  `r3d_first_person_math` with cases: 0.3 rad → no step; 0.5 rad → one step and glance 0.5−π/4.

#### Parallelisation

- Phase 0 items are independent of each other (FP-01/FP-08 share the label option; FP-03 and
  FP-07 both edit `draw_hud` — serialise those two).
- Phase 1 FP-09/FP-10 are independent; FP-11 touches `handle_action.cpp` only in its target
  resolution helper.
- Phase 2/3 items are independent of each other; FP-13 and FP-12 both touch the driving branch in
  `r3d.cpp` `draw()` — serialise.

#### Summary table

| Id | Item | Value | Effort | Depends | Main files |
| --- | --- | --- | --- | --- | --- |
| FP-01 | Crosshair label option | High | S | Q1 | options.cpp, r3d.cpp |
| FP-02 | Docs/option sync | Med | S | FP-01 decision | first_person.md, options.cpp |
| FP-03 | FP compass parity | High | S/M | — | r3d_hud.cpp, r3d_hud_math, r3d.cpp |
| FP-04 | Quick turn | Med | S | — | action.*, handle_action.cpp, keybindings.json |
| FP-05 | 45/90° turn option | Low/Med | S | — | options.cpp, r3d.cpp |
| FP-06 | Minimap facing wedge | Med | S | — | pixel_minimap.cpp |
| FP-07 | Edge pointers in FP | High | S/M | FP-03 (strip ownership) | r3d_hud.cpp |
| FP-08 | Hovered label | Med | S | FP-01 | r3d.cpp |
| FP-09 | Sound ticks + captions | Med | M | — | r3d.cpp |
| FP-10 | Real attacker direction | Low | S | — | r3d_hud.cpp |
| FP-11 | Interaction prompt | High | M | — | new r3d_interaction.*, handle_action.cpp |
| FP-12 | Driving head-turn | Med | S | — | r3d.cpp |
| FP-13 | Orbit while driving | Med | S/M | — | options.cpp, r3d.cpp |
| FP-14 | Driving readout | Low | S | — | r3d.cpp |
| FP-15 | Exposure readout | Med | M | Q5 | new r3d_awareness.*, r3d_hud.cpp |
| FP-16 | Vision overlays | Med | M | Q5 | r3d_hud.cpp |
| FP-17 | Scope zoom | Med | M | — | r3d.*, ranged.cpp |
| FP-18 | Rear mirror (bold) | Med | L | measurement | r3d_gpu, r3d.cpp |
| FP-22 | Snapping mouselook (bold) | Med | M | Q2 | sdltiles.cpp, r3d.cpp |

### 3.6 Open questions for the owner

1. **Q1 Crosshair label**: `e7fef314b1` removed it one day before this plan. Was that because of
   clutter, wrong information, or performance? FP-01 proposes a three-way option with `dot` as
   default; confirm or drop FP-01/FP-08.
2. **Q2 Mouselook/capture**: capture was removed in `3b470163a1`. Should a snapping variant
   (FP-22) be attempted at all, or is middle-drag glance the final answer?
3. **Q3 Turn step**: keep 45° default and add 90° as an option (FP-05), or skip the option?
4. **Q4 Comfort defaults**: keep `R3D_FP_TWEEN_MS = 0`? Keep the weapon bob?
5. **Q5 Presentation effects on perception** (FP-15/16/19): acceptable in principle, as long as
   BN's visibility is untouched? Default off?
6. **Q6 Driving camera**: should FP stay in the cab by default (with FP-12 head turns), or should
   control automatically switch to orbit (FP-13) until mirrors exist?
7. **Q7 Keybinding defaults**: which chord for "turn around" (FP-04)? Should `x` (BN look) stay?
8. **Q8 `forced_3d_on_start`**: intended for this fork, or a leftover? It affects how "first
   person by default" reads to a new player.
9. **Q9 Scope**: are gameplay-facing changes (facing/flank rules, turn costs) off the table? This
   plan assumes yes.

---

## Sources

Genre and design articles

- https://filfre.net/?p=3803 (Digital Antiquarian, "Life on the Grid"; mirror seen at
  https://www.gamebanshee.com/k4fp6 and https://www.filfre.net/2019/01/life-on-the-grid/)
- https://itch.io/jam/dcjam2021/topic/1183406/pov
- https://dungeoncrawlers.org/definition/
- https://dungeoncrawlers.org/game/orcs-elves/
- https://itch.io/jam/dcjam2023/topic/2710844/out-of-curiosity-why-isnt-hexagonal-grid-allowed
- https://itch.io/jam/dcjam2024/topic/3562914/just-verifying-the-rules-about-movement
- https://boingboing.net/2019/01/25/brief-history-of-grid-based-du.html (title only)
- https://gamesbeat.com/how-one-vr-developers-beating-motion-sickness-by-going-back-to-role-playing-game-basics/
- https://store.steampowered.com/app/4982850/ (Captive 3000, marketing claim)
- https://retronauts.com/article/437/rediscovering-the-mostly-lost-art-of-mapping
- https://themagnet.substack.com/p/ive-been-playing-wizardry-for-43
- https://par.nsf.gov/servlets/purl/10591601 (snap turn / FOV restriction study)
- https://developers.meta.com/vr/design/locomotion-user-preferences/ (snippet)
- https://graphicsinterface.org/wp-content/uploads/gi2021-32.pdf (Sign-of-the-UnSeen, abstract)
- https://www.gamedeveloper.com/design/game-ui-discoveries-what-players-want
- https://www.csit.carleton.ca/~rteather/pdfs/GI2015_poster2.pdf (snippet)
- https://kotaku.com/the-problems-with-illegible-text-in-video-games-and-som-1825018305
- https://uksignboards.com/forums-2/discussion/does-anyone-know-of-a-guide-for-distance-legibility/ (snippet)
- https://www.abratabia.com/game-accessibility/audio-accessibility.php
- https://forums.unrealengine.com/t/first-person-dungeon-crawler-style-movement/39672
- https://discussions.unity.com/t/first-person-grid-movement-dungeon-master-style/484014
- https://rnb-games.itch.io/dungeon-crawler-godot-template
- https://midnightspiregames.itch.io/minerva-labyrinth/devlog/473245/building-a-dungeon-crawler-in-unity-part-i
- https://www.lexaloffle.com/bbs/?tid=3585
- https://itch.io/c/333360/grid-based-dungeon-crawlers
- https://store.steampowered.com/curator/10580792-Grid-based-first-person-RPGs/
- https://itch.io/post/15704356

Wizardry, Might and Magic, Dungeon Master, Eye of the Beholder

- https://gamingtrend.com/feature/reviews/wizardry-remaster-review-do-you-like-black-and-white-movies/
- https://wizardry.wiki.gg/wiki/Wizardry:_Proving_Grounds_of_the_Mad_Overlord_(remake)
- https://vaporlens.app/app/2518960/wizardry_proving_grounds_of_the_mad_overlord
- https://seafoamgaming.com/2024/07/17/wizardry-proving-grounds-of-the-mad-overlord-steam-review
- https://steamcommunity.com/app/245410/discussions/0/617328415066916054
- https://www.gog.com/forum/wizardry_series/wizardry_6_7_versus_8 (snippet)
- https://en.wikipedia.org/wiki/Might_and_Magic_III:_Isles_of_Terra
- https://giantbomb.com/users/3788/articles/i-play-old-games-might-and-magic-world-of-xeen (snippet)
- https://atarimagazines.com/compute/issue152/100_Might_and_Magic_Clo.php (snippet)
- https://kvibber.com/reviews/games/might-and-magic-9/ (snippet)
- http://www.gamebanshee.com/3wyp9 (snippet)
- https://www.timeextension.com/features/the-making-of-dungeon-master-a-truly-trailblazing-first-person-rpg
- https://surfhelpt.duckdns.org/textfiles.com/games/REVIEWS/dungmstr.rev (snippet)
- https://www.generationamiga.com/2026/05/16/eye-of-the-beholder-retrospective-inside-westwoods-legendary-commodore-amiga-dungeon-crawler/
- http://www.gamebanshee.com/kax9d

Legend of Grimrock, Vaporum, Operencia, Stonekeep, The Quest, Paper Sorcerer

- https://www.gamebanshee.com/u4c4
- https://bit-tech.net/reviews/gaming/pc/the-legend-of-grimrock-review/2/
- https://www.gamewatcher.com/reviews/legend-of-grimrock-review/9098 (snippet)
- https://gameinformer.com/games/legend_of_grimrock/b/pc/archive/2012/04/04/review.aspx
- https://steamcommunity.com/app/207170/discussions/0/522728180906396337
- https://www.gamebanshee.com/8wp8
- https://www.gamebanshee.com/kuhqk
- https://www.dungeon-master.com/forum/viewtopic.php?p=146111
- https://www.gameskinny.com/reviews/legend-of-grimrock-2-review/ (snippet)
- https://scientificgamer.com/thoughts-legend-of-grimrock/
- https://steamcommunity.com/app/251730/discussions/0/624076027803538385
- https://store.epicgames.com/news/legend-of-grimrock-excavates-the-buried-treasures-of-the-dungeon-genre
- https://cubed3.com/review/5510/1/vaporum-xbox-one.html
- https://www.cubed3.com/games/reviews/xbox-one/vaporum
- https://www.thexboxhub.com/?p=71091
- https://www.gamingonlinux.com/articles/17361/
- https://gamingtrend.com/feature/reviews/hallways-and-hotkeys-operencia-the-stolen-sun-review
- https://en.wikipedia.org/wiki/Stonekeep
- https://www.gamespot.com/reviews/stonekeep-review/1900-2559827/ (snippet)
- https://en.wikipedia.org/wiki/The_Quest_(2006_video_game)
- https://www.indieretronews.com/2015/12/the-quest-from-mobile-to-pc-dungeon.html
- https://toucharcade.com/community/threads/paper-sorcerer-ultrarunawaygames-stylish-first-person-turn-based-rpg.185313/
- https://www.gamingnexus.com/News/39388/The-Bards-Tale-IV-combat-is-layered-and-gridlocked

Etrian Odyssey, SMT, Elminage, Experience Inc., Dungeon Encounters, Refrain/Galleria, Daphne

- https://www.nintendoworldreport.com/review/13697/etrian-odyssey-nintendo-ds
- https://www.siliconera.com/2013/08/21/etrian-odyssey-untold-has-some-very-cool-looking-enemies/ (snippet)
- https://lparchive.org/Etrian-Odyssey-II-Heroes-of-Lagaard/Update%2003
- https://www.nintendoworldreport.com/review/63845/etrian-odyssey-switch-review
- https://www.dampfkraft.com/games/etrian-odyssey-directors-diary.html
- https://www.rpgfan.com/review/shin-megami-tensei-2/
- https://gameinformer.com/games/shin_megami_tensei/b/ios/archive/2014/03/25/shin-megami-tensei-ios-impressions
- https://destructoid.com/review-shin-megami-tensei-strange-journey
- https://gamingtrend.com/feature/reviews/shin-megami-tensei-strange-journey-review (snippet)
- https://www.rpgsite.net/review/7172-shin-megami-tensei-strange-journey-redux-review
- https://www.rpgsite.net/review/3926-elminage-gothic-review
- https://www.rpgfan.com/review/stranger-of-sword-city/
- https://www.keengamer.com/articles/reviews/operation-abyss-new-tokyo-legacy-review/
- https://wccftech.com/review/dungeon-encounters-live-and-die-by-the-atb/amp/
- https://square-enix-games.com/it_IT/news/videogame-dogs (snippet)
- https://www.nintendoworldreport.com/interview/58911/talking-dungeon-encounters-with-square-enixs-hiroyuki-ito-and-hiroaki-kato
- https://www.nintendoworldreport.com/review/58795
- https://www.rpgsite.net/review/7765-labyrinth-of-refrain-coven-of-dusk-review (snippet)
- https://game-wisdom.com/analysis/labyrinth-of-galleria (snippet)
- https://www.rpgsite.net/review/13809-labyrinth-of-galleria-the-moon-society-review
- https://shacknews.com/article/141912/wizardry-variants-daphne-review-score
- https://wizardry.wiki.gg/wiki/Wizardry_Variants_Daphne
- https://steamcommunity.com/app/2379740/discussions/0/600772666094293891

First-person roguelikes, Daggerfall, Underworld, viewers

- https://baronygame.nfshost.com/presskit/
- https://giantbomb.com/wiki/Games/Barony
- https://vaporlens.app/app/371970/barony.md
- https://steamcommunity.com/app/371970/discussions/0/599641410463879901
- https://steamcommunity.com/app/371970/discussions/0/405691147602132372
- https://barony.wiki.gg/wiki/Stealth
- https://allthings.how/barony-beginner-tips-for-surviving-the-dungeon/
- https://en.wikipedia.org/wiki/Jupiter_Hell
- https://gamecritics.com/jeff-ortloff/jupiter-hell-review/
- https://jupiterhell.com/faq
- https://www.gamingonlinux.com/2019/10/the-impressively-smooth-roguelike-jupiter-hell-has-a-big-ai-upgrade-and-a-first-sale/page=1/
- https://forums.dfworkshop.net/viewtopic.php?p=60506 (snippet)
- https://steamcommunity.com/app/1812390/discussions/0/3821908348793349501 (snippet)
- https://steamcommunity.com/app/1812390/discussions/0/4635987888980380978 (snippet)
- https://en.m.uesp.net/wiki/Tes2Mod:Daggerfall_Unity/Gameplay_Differences
- https://www.dfworkshop.net/june-builds-map-upgrades-retro-rendering-crime-features-and-more/
- https://forums.dfworkshop.net/viewtopic.php?p=1469
- https://gallery.ultimacodex.com/the-ultima-underworld-press-kit/
- https://gallery.ultimacodex.com/ultima-underworlds-auto-mapping-feature/
- https://store.steampowered.com/app/2343160/ROGUEFP/
- https://en.wikipedia.org/wiki/Dungeon_Hack
- https://www.gamebanshee.com/ku4
- https://en.wikipedia.org/wiki/Moonring
- https://thegdwc.com/games/dod
- https://steamcommunity.com/app/442780/discussions/0/2798319091577246817
- https://www.avclub.com/eldritch-review
- https://www.pcgamer.com/review/eldritch-review/
- https://nethackwiki.com/wiki/Falcon%27s_Eye
- https://nethackwiki.com/wiki/Noegnud
- https://nethackwiki.com/wiki/Vulture
- https://wiki.gentoo.org/wiki/Games/roguelike
- https://github.com/JapaMala/armok-vision/releases/tag/v0.13.1
- https://aur.archlinux.org/packages/armok-vision
- https://www.pcgamesn.com/dwarf-fortress/3d-mod-dfhack
- https://la-chicungunha.itch.io/voxelcdda
- https://itch.io/games/tag-grid-based/tag-roguelike (snippet)
- https://www.pcgamer.com/games/a-hardcore-turn-based-zombie-survival-roguelike-is-getting-good-buzz-on-steam/
- https://store.steampowered.com/app/1534980/Terminus_Zombie_Survivors/
- https://www.pcgamesn.com/terminus-zombie-survivors/steam-rating
- https://rogueliker.com/best-new-roguelikes/
- https://gamerant.com/best-zombie-themed-roguelikes-roguelites/
- https://gamefish.itch.io/zombie-rogue

Survival sims and HUD design

- https://pzwiki.net/wiki/Bin (snippet)
- https://www.bamboogaming.net/project-zomboid/loot (snippet)
- https://www.bisecthosting.com/blog/project-zomboid-build-42-unstable-multiplayer-update-patch-notes
- https://steamcommunity.com/app/108600/discussions/0/3767857092599707677 (snippet)
- https://projectzomboid.wiki/guides/stealth-mechanics/
- https://pzfans.com/how-does-fog-affect-zombie-vision-and-how-to-make-it-go-away/
- https://steamcommunity.com/app/108600/discussions/0/1696043263510206539
- https://steamcommunity.com/app/108600/discussions/6/4353365802909182320
- https://twt.omada.cafe/r/projectzomboid/comments/1hz7gmn/has_everyone_forgotten_its_a_zombie_survival_game
- https://steamcommunity.com/app/108600/discussions/0/3288067088117206895
- https://steamcommunity.com/app/108600/discussions/0/3380537111200724763
- https://steamcommunity.com/app/108600/discussions/0/4353365446954438710
- https://projectzomboid.com/blog/2017/05/vehicles-tech-test/ (not fetched)
- https://steamcommunity.com/app/108600/discussions/0/595135660380156129 (snippet)
- https://pzfans.com/no_more_snoozing_over_skill_books_how_to_read_faster_in_project_zomboid_b41__b42
- https://steamcommunity.com/sharedfiles/filedetails/comments/2744101803 (snippet)
- https://steamcommunity.com/app/1782120/discussions/0/594021624312822203
- https://www.dexerto.com/escape-from-tarkov/summit1g-explains-how-escape-from-tarkov-looting-system-punishes-kills-1337715 (snippet)
- https://mmos.com/news/escape-from-tarkovs-inventory-and-looting-systems-revealed
- https://caniplaythat.com/?p=14083
- https://www.gamespot.com/articles/how-to-use-visualize-audio-effects-in-fortnite/1100-6498314/ (snippet)
- https://www.moddb.com/mods/stalker-anomaly/addons/squaredov (snippet)
- https://www.moddb.com/addons/ogsr-minimal-hud (snippet)
- https://www.nexusmods.com/stalkeranomaly/mods/47
- https://dyinglight.wiki.gg/wiki/Night
- https://piunikaweb.com/2022/02/05/dying-light-2-survivor-sense-not-working-or-activating-workarounds-inside/
- https://www.engadget.com/2014-03-19-making-everything-new-old-again-in-alien-isolation.html
- https://www.pcgamer.com/au/why-you-should-be-playing-hunt-showdown (snippet)
- https://www.thesixthaxis.com/2024/10/15/hunt-showdown-1896-review/ (snippet)
- https://store.epicgames.com/news/darkwood-is-a-horror-masterpiece-viewed-from-an-atypical-perspective
- https://www.gamedeveloper.com/design/building-the-original-i-thief-s-i-revolutionary-stealth-system
- https://www.ludo.guide/guide/metro-2033/light-meter-watch
- https://steamcommunity.com/app/412020/discussions/0/1798529872641515825
- https://7daystodie.wiki.gg/wiki/Stealth_System
- https://steamcommunity.com/app/251570/discussions/4/3834298194197806932
- https://gamedeveloper.com/design/long-dark-survival-game-hud-ui
- https://steamcommunity.com/app/305620/discussions/0/405694115206941426
- https://archive.vrroom.studio/?p=30438
- https://www.gfinityesports.com/article/the-unsung-hero-of-dead-space-why-diegetic-hud-design-is-crucial-for-ultimate-immersion
- https://steamcommunity.com/app/264710/discussions/0/1693785035819973504/?l=latam
- https://steamcommunity.com/app/264710/discussions/4/2595630410190970247/?ctp=3 (snippet)
- https://wikily.gg/es/subnautica-2/maps/ (secondhand)
- https://community.focus-entmt.com/focus-entertainment/space-marine-2/ideas/8114-can-we-get-an-option-to-remove-the-vignette
- https://itch.io/post/9282571 (snippet)
- https://escapefromtarkov.fandom.com/wiki/File:Pre-alpha_inventory.png (listing only)
- https://steamcommunity.com/app/1874880/discussions/0/604146915402101102
- https://forum.quartertothree.com/t/driving-games-which-view/41203
- https://forums.forza.net/t/always-playing-on-cockpit-view-with-gamepad-do-i-need-to-change/75906
- https://feedback.bistudio.com/T122955 (snippet)
- https://steamcommunity.com/app/221100/discussions/0/1642039363010032851
- https://feedback.bistudio.com/T87202 (unverified)
