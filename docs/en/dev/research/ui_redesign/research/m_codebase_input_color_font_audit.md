# Codebase audit: input, color and font systems

Scope: read-only audit of `/home/richard/Projects/Cataclysm-BN-3D` on branch `feat/ui-overhaul`.
Method: static reading of source with `rg`/`sed`. Nothing was built or run, so runtime behaviour is
inferred from code unless stated otherwise. Line numbers refer to the working tree at audit time.

Paths used throughout:
- Input: `src/input.h`, `src/input.cpp`, `src/action.h`, `src/action.cpp`, `src/handle_action.cpp`,
  `src/sdltiles.cpp` (SDL event loop), `src/r3d.cpp`, `src/r3d.h`, `src/popup.cpp`, `src/path_info.cpp`
- Keybinding data: `data/raw/keybindings/*.json` (four files: `keybindings.json`, `vehicle.json`,
  `messages.json`, `edit_creature_effects.json`)
- Color: `src/color.h`, `src/color.cpp`, `src/color_loader.h`, `src/sdl_utils.cpp`, `src/hsv_color.cpp`,
  `src/ncurses_def.cpp`, `data/raw/colors.json`
- Font: `src/sdl_font.h`, `src/sdl_font.cpp`, `src/font_loader.h`, `src/font_loader.cpp`,
  `data/raw/fonts.json`, `data/font/`

---

## A. Input system

### A.1 Event types

`input_event_t` (input.h:91-97) has five values: `error`, `timeout`, `keyboard`, `gamepad`, `mouse`.

```cpp
enum class input_event_t : int  {
    error,
    timeout,
    keyboard,
    gamepad,
    mouse
};
```

`struct input_event` (input.h:107) carries:
- `type` (input.h:108)
- `modifiers` (input.h:110): a `std::vector<int>`. **Nothing in the tree ever writes to it.** It is only
  read (input.cpp:787, input.cpp:960, copy/compare at input.h:154 and input.h:177-181).
- `sequence` (input.h:114): a `std::vector<int>` of key codes. It is filled only when loading JSON
  (input.cpp:248, input.cpp:258) or by the single-value constructor (input.h:136-138). Every event that
  SDL produces has a sequence of length 1.
- `mouse_pos` (input.h:117): a `point`.
- `text` / `edit` / `edit_refresh`: UTF-8 text and IME composition state.

Equality (input.h:163-166) compares `type` first, then `sequence` and `modifiers`. Because of that, a
`keyboard` event never equals a `gamepad` event, even when the integer value is the same.

Mouse and gamepad codes are plain integers (input.h:89, input.h:206-222):

```cpp
enum mouse_buttons { MOUSE_BUTTON_LEFT = 1, MOUSE_BUTTON_RIGHT, SCROLLWHEEL_UP, SCROLLWHEEL_DOWN, MOUSE_MOVE };
#define JOY_0        0          // ... JOY_7 = 7
#define JOY_LEFT        (256 + 1)   // JOY_RIGHT, JOY_UP, JOY_DOWN, JOY_RIGHTUP, JOY_RIGHTDOWN,
                                    // JOY_LEFTUP, JOY_LEFTDOWN follow as 256+2 .. 256+8
```

Name tables: keyboard names via `init_keycode_mapping` (input.cpp:374-449). Mouse names are fixed
at input.cpp:444-447 (`MOUSE_LEFT`, `MOUSE_RIGHT`, `SCROLL_UP`, `SCROLL_DOWN`, `MOUSE_MOVE`).
Gamepad names are at input.cpp:426-442 (`JOY_LEFT` .. `JOY_7`).

### A.2 How input_context registers actions and resolves keys

- `input_manager` (singleton `inp_mngr`, input.h:232, 388-ish) owns `action_contexts`, a
  `map<category, map<action_id, action_attributes>>`. Each action holds a list of `input_event`s
  (input.h:232-ish, `t_action_contexts` in the private section).
- Loading: `input_manager::init()` (input.cpp:119) loads every `.json` under the keybindings dir
  (input.cpp:127-133), then the user file (input.cpp:136-141). `load()` (input.cpp:190) reads each
  object's `id`, `category` (default `"default"`, input.cpp:207), and `bindings[]` with
  `input_method` (keyboard | gamepad | mouse) and `key` (string or array).
- Registration: `input_context` constructor takes an optional category string (input.h:415-418).
  `register_action()` (input.cpp:733-751) appends to `registered_actions`. Two special IDs set flags:
  `"ANY_INPUT"` (sets `registered_any_input`) and `"COORDINATE"` (sets `handling_coordinate_input`).
- Resolution: `handle_input()` (input.cpp:1018-1093) loops on `inp_mngr.get_input_event()` and calls
  `input_to_action()` (input.cpp:693-709). That function walks `registered_actions` in registration
  order and returns the first action whose bindings in `category` contain the event. If nothing
  matches it returns `"ERROR"` (`CATA_ERROR`). With `ANY_INPUT` registered it returns `"ANY_INPUT"`.

Lookup and fallback (input.cpp:536-571):

```cpp
// get_action_attributes(): try the named category, then "default"
if( context != default_context_id ) {
    // ... if found and !is_deleted, return it (even if its bindings list is empty)
}
// otherwise fall back to the default context
```

Consequence: a category entry for an action overrides the default entry entirely. It does not merge
with it. An empty list in the category masks the default binding. This follows from the code as written.

Actions are identified by string IDs. `input_context` accepts any string, not only `action_id`. The
`action_id` enum (action.h:19, about 168 `ACTION_*` enumerators per grep) is a separate, game-level
layer. `action_ident()` (action.cpp:264) maps enum to string. `look_up_action()` (action.cpp:692)
maps back. Many contexts register raw strings such as `"UP"`, `"LEFT"`, `"REMOVE"` and
`"HELP_KEYBINDINGS"`, which have no `action_id`.

Special handling inside `handle_input()`:
- `"HELP_KEYBINDINGS"` opens `display_menu()` (input.cpp:1050-1056).
- Unbound keys in the `DEFAULTMODE` category are first passed to `r3d_key_action()`
  (input.cpp:957-1016, see A.8).
- Mouse events that match no action are dropped unless the context registered `COORDINATE`
  (input.cpp:1065-1072).

### A.3 Categories and conflicts

- Category is a free string from the JSON `category` field. The `default` category is the global
  fallback. The category set is what the code requests; there is no central list.
- Counts by `category` in `data/raw/keybindings/keybindings.json` (grep counts of lines):
  `DEFAULTMODE` 132, `ADVANCED_INVENTORY` 36, `VEH_INTERACT` 24, `INVENTORY_ITEM` 24, `OVERMAP` 21,
  `CRAFTING` 15, `NPC_TRADE` 14, `TARGET` 13 and about 60 more. `vehicle.json` adds `VEHICLE` (42).
  `messages.json` adds `MESSAGE_LOG` (4). `edit_creature_effects.json` uses its own category
  (7 entries). Total entries with a `category` field: 513 in the main file.
- Conflict checks are scoped to the current context. `get_conflicts()` (input.cpp:655-661) lists
  only the `registered_actions` of this context that already use the event. `clear_conflicting_keybindings()`
  (input.cpp:663-691) removes the key from those same actions, in both `default` and the
  current category. Two different contexts can therefore bind the same key without any warning.
- `input_context::get_available_single_char_hotkeys()` (input.cpp:779-810) returns printable
  characters that have no conflicting use in the registered actions.

Layering and inheritance:
- Two levels only: a named category, then `default`. There is no stack of bindings.
- `input_context::input_context_stack` (input.h:403, defined input.cpp:709) is a stack of live
  contexts. It is used only for `get_active_category()` (input.cpp:946-955) and by the touch quick-shortcut
  code (sdltiles.cpp:3001-3002). It does not affect key resolution.
- Contexts are not merged. The `display_menu()` help context (input.cpp:1209-1230) is a separate,
  throwaway `input_context( "HELP_KEYBINDINGS" )`.

### A.4 Keybinding editor (`input_context::display_menu`)

The in-game editor is `input_context::display_menu()` (input.cpp:1206-1557). It is the screen opened
by the `HELP_KEYBINDINGS` action (input.cpp:1050-1056) and by the `EXECUTE` path from
`handle_action.cpp:2029`. A separate `help::display_help()` (help.cpp:143) is the in-game help text and
is not the keybinding editor.

What it can do:
- Lists the registered actions of the current context and their bindings, colour-coded: unbound
  (`c_light_red`), local to this screen (`c_light_green`), global (`c_light_gray`) (input.cpp:1295-1301).
- Add a local binding (`+` or `ADD_LOCAL`), add a global one (`=` or `ADD_GLOBAL`), remove (`-` or `REMOVE`),
  and execute an action if permitted (`.` or `EXECUTE`). The hard-coded keys are in `fallback_keys`
  (input.cpp:1199-1204). Hotkey letters come from `display_help_hotkeys` (input.cpp:1187-1188).
- Scrolling and paging (`UP`, `DOWN`, `PAGE_UP`, `PAGE_DOWN`).
- Capture of a new key uses `query_popup().allow_anykey( true )` (input.cpp:1499-1501). In that mode the popup
  accepts any event except mouse movement and empty keyboard events (popup.cpp:303-309). So a
  keyboard key, mouse button, mouse wheel or gamepad event can all be recorded, one at a time.
- Conflicts prompt to clear the other binding, with a yes/no question (input.cpp:1499-1530 region).
- On exit, if anything changed, it asks "Save changes?" and calls `inp_mngr.save()` (input.cpp:1543).
  `save()` (input.cpp:302-357) writes the user file (`PATH_INFO::user_keybindings()`, path_info.cpp:323).
  `user_keybindings()` is `config_dir + "keybindings.json"`.

What it cannot do (from the code):
- Build or edit a **modifier** combination. `modifiers` is never set. Ctrl+letter is folded into the
  control code (sdltiles.cpp:2086-2089), and Shift+Tab becomes `KEY_BTAB`. Those are fixed keysym
  translations, not configurable bindings.
- Record a **multi-key sequence**. The capture takes one event, so `sequence` has length 1.
- Bind the **middle mouse button**. `mouse_buttons` has no middle entry (input.h:89), and the SDL
  loop never emits an input event for it (see A.5).
- Bind a **gamepad button** in a useful way. A physical joystick button arrives as a keyboard-typed
  event (see A.6), so capturing a button records a keyboard code instead.
- Show or edit camera or 3D-specific bindings differently from other bindings. The `r3d_*` actions
  are ordinary entries in `keybindings.json` (for example keybindings.json:4046-4064).

### A.5 Mouse handling (SDL build)

All mouse handling is in `CheckMessages()` (sdltiles.cpp:2963). Each event in the drain loop
`while( SDL_PollEvent )` (sdltiles.cpp:3290) is handled as follows.

- **Motion** (sdltiles.cpp:3511-3540):
  - When the 3D view is active and not dragging, the hovered tile is picked (3512-3521). The event
    coordinates are divided by the scaling factor first.
  - While the middle button is held (`r3d_dragging`), motion goes to the camera (3524-3529, see A.8).
  - `MOUSE_MOVE` is emitted only when `HIDE_CURSOR` is `"show"` or `"hidekb"` (3530-3538). With any
    other `HIDE_CURSOR` value, motion produces no input event.
- **Button down** (3506-3510): only the middle button does anything (it starts camera drag).
- **Button up** (3541-3555):
  - Middle: ends drag and camera glance (3542-3546).
  - `SDL_BUTTON_LEFT` → `input_event(MOUSE_BUTTON_LEFT, mouse)`; `SDL_BUTTON_RIGHT` → `MOUSE_BUTTON_RIGHT`.
  - Other buttons (X1, X2) produce nothing.
- **Wheel** (3557-3564): `wheel.y > 0` → `SCROLLWHEEL_UP`, `< 0` → `SCROLLWHEEL_DOWN`. Horizontal
  wheel is ignored.

Coordinates:
- `get_input_event()` (sdltiles.cpp:4105, mouse block at 4155-4160) sets `mouse_pos` from
  `SDL_GetMouseState()` at the time the event is consumed. It does not use the event's own x/y.
  The value is in **window pixels**.
- `input_context::get_coordinates()` (sdltiles.cpp:4282-) converts that pixel point to a
  `tripoint_bub_ms` using the window's pixel position and font size (`get_window_dimensions`,
  sdltiles.cpp:4200-4235). The window pixel position uses unscaled `fontwidth`/`fontheight`
  (sdltiles.cpp:4225-4226), while the size uses the scaled font size (4212, 4228-4229). See
  "Uncertain" below.
- The non-tiles version of `get_coordinates` is in input.cpp:1596-1618, inside `#if !(TILES||_WIN32)`.

Click-to-action: `ctxt.get_coordinates(w_terrain)` is used in game.cpp:3539 and game.cpp:10372, and
handle_action.cpp:2051.

Keybindings can name mouse inputs (`"input_method": "mouse"`, keys `MOUSE_LEFT`, `MOUSE_RIGHT`,
`SCROLL_UP`, `SCROLL_DOWN`). keybindings.json:1-8 binds `SCROLL_UP`/`SCROLL_DOWN` in `UILIST`.

### A.6 Gamepad and joystick (current state)

- SDL subsystem: `SDL_InitSubSystem( SDL_INIT_JOYSTICK )` only (sdltiles.cpp:184). **No
  `SDL_INIT_GAMEPAD`**, and no `SDL_Gamepad*` or `SDL_EVENT_GAMEPAD_*` call exists anywhere in `src/`.
  The code uses the legacy SDL_Joystick API (`SDL_GetJoysticks`, `SDL_OpenJoystick`,
  `SDL_GetJoystickHat`, `SDL_EVENT_JOYSTICK_BUTTON_DOWN`, `SDL_EVENT_JOYSTICK_AXIS_MOTION`).
- Startup (sdltiles.cpp:373-390): if `ENABLE_JOYSTICK` is true and at least one device exists, the
  **first** device is opened and a warning is logged if there are several. The option is at
  options.cpp:2158-2161 (default `true`, `COPT_CURSES_HIDE`). It is read only at startup.
- Hot-plug: no `SDL_EVENT_JOYSTICK_ADDED` or `_REMOVED` handling exists (grep: none).
- `gamepad_available()` returns `joystick != nullptr` (sdltiles.cpp:4175-4178). It is used only to
  decide whether gamepad hints are shown (input.cpp:833, input.cpp:881).

Event paths:
- **D-pad (hat)**: `HandleDPad()` (sdltiles.cpp:1869-1935) runs at the start of `CheckMessages()`
  (2969). It maps the hat to `JOY_LEFT`, `JOY_UP`, `JOY_RIGHTUP` and so on, as `gamepad`-typed events.
  A 100 ms delay (dpad_delay, sdltiles.cpp:142-143) lets diagonals form. Continuous repeat is 200 ms
  for the first step and 60 ms after that (sdltiles.cpp:1897-1921). This path produces the intended
  gamepad event type.
- **Buttons**: `SDL_EVENT_JOYSTICK_BUTTON_DOWN` → `input_event( ev.jbutton.button, input_event_t::keyboard )`
  (sdltiles.cpp:3499-3500). The button index is therefore sent as a **keyboard** code, not a gamepad code.
  The `JOY_0` binding in `GOT_IT` (keybindings.json:4140-4147) is typed `gamepad`, and `operator==`
  compares type first (input.h:163-166). So from the code it looks like `JOY_0` **cannot match** a real
  button press. This is an inference from reading the code and was not run.
- **Sticks**: `SDL_EVENT_JOYSTICK_AXIS_MOTION` is received and discarded (sdltiles.cpp:3502-3504,
  "TODO: somehow get the digipad values").
- Virtual joystick (`ANDROID_*` options, options.cpp:3519-3569; `draw_virtual_joystick`
  sdltiles.cpp:2759) exists only for Android. It is not part of the desktop gamepad path.

Summary of gamepad state: the d-pad path is wired and uses `gamepad` events. Face buttons are wired
in a way that likely does not match their `JOY_n` bindings. The sticks are not used. Nothing has been
run on hardware.

### A.7 Key repeat, hold vs toggle, text, and modifiers

- **Auto-repeat**: `SDL_EVENT_KEY_DOWN` carries `ev.key.repeat` (sdltiles.cpp:3356). That flag is used
  only to skip the text-refresh path (sdltiles.cpp:3731). Repeated key events therefore produce repeated
  input events. Repeat rate is whatever the OS/SDL sends. The game does not set its own rate.
- **Key down/up**: `KEY_DOWN` converts the keysym with `sdl_keysym_to_curses()` (sdltiles.cpp:2020 onward;
  the call is at 3368). `KEY_UP` only handles Alt-codes (sdltiles.cpp:3427-3451).
- **Text**: `SDL_EVENT_TEXT_INPUT` (sdltiles.cpp:3452-3480) produces a keyboard event with the UTF-8
  code point. For a printable key both KEY_DOWN and TEXT_INPUT fire, and the later one wins.
- **Single slot**: `last_input` is one variable. It is reset once per drain batch (sdltiles.cpp:3277)
  and overwritten by every event that sets it. Inputs that arrive in the same batch therefore collapse to
  the last one. There is no `break` on the first input found inside the loop at sdltiles.cpp:3290
  (checked by reading; not exhaustive).
- **Diagonal keys**: with `DIAG_MOVE_WITH_MODIFIERS_MODE` (options.cpp:1950-1990), Shift/Ctrl+arrow
  is turned into diagonal action IDs (sdltiles.cpp:2029-2080). Mode 3 was not read.
- **Hold vs toggle**: there is no toggle abstraction for keys. Hold-style behaviour appears in three
  places:
  1. The 3D WASD diagonal, which reads `SDL_GetKeyboardState` at the moment of the event (input.cpp:980-993).
  2. The arrow-key diagonal in DEFAULTMODE, which reads the arrow state (sdltiles.cpp:3380-3394).
  3. The middle mouse drag and the first-person glance, which last until the button is released (A.8).
  On Android, a finger hold repeats input on a timer (sdltiles.cpp:3233-3250).

### A.8 3D camera (`src/r3d_*`) input

Mouse (all in `sdltiles.cpp`, state at 2962 `namespace { auto r3d_dragging = false; }`):
- Middle-button down inside the terrain view starts a drag if `r3d::contains()` is true
  (sdltiles.cpp:3506-3509; `contains` at r3d.cpp:1990-1994). Other buttons are not used for the camera.
- Motion while dragging (sdltiles.cpp:3524-3529):
  - `SHIFT` held → `r3d::drag_pan( xrel, yrel )`.
  - Otherwise → `r3d::drag_orbit( xrel, yrel )`.
  - Sets `camera_changed`. The redraw is deferred to once per batch (sdltiles.cpp:3757-3760).
- `drag_orbit` (r3d.cpp:2006-2026): yaw changes by `dx * 0.008` and pitch by `dy * 0.006`, clamped to
  `camera::min_pitch` and `camera::max_pitch`. In first-person mode it sets a temporary glance angle
  instead (`max_glance_yaw` 2.4 rad).
- `drag_pan` (r3d.cpp:2028-2044): pans in ground coordinates, scaled by camera distance, and clamped to
  ±48 tiles. It is ignored in first-person.
- Button up of the middle button calls `r3d::end_glance()` (r3d.cpp:1907) and clears the drag
  (sdltiles.cpp:3542-3546).

Keyboard (`r3d_key_action`, input.cpp:957-1016):
- Active only when category is `DEFAULTMODE`, there are no modifiers, and `r3d::wasd_controls()` is
  true (r3d.cpp:1892-1893; option `R3D_WASD`, options.cpp:2667).
- WASD and arrow keys are remapped to the movement actions and the `r3d_rotate_*` / `r3d_pitch_*`
  actions. `q`/`e` rotate unless a vehicle is being controlled. Diagonals are picked from held W/A/S/D
  state.

Mouse wheel: `SCROLLWHEEL_*` events are not consumed by any r3d code (grep shows only sdltiles.cpp:3559-3561
and input.cpp:496-498). 3D zoom is keyboard-driven (`ACTION_R3D_ZOOM_IN`/`OUT`, handle_action.cpp:3277-3282).

### A.9 Input-related options

| Option | Where | Effect |
|---|---|---|
| `ENABLE_JOYSTICK` | options.cpp:2158-2161 | Opens first joystick at startup only |
| `HIDE_CURSOR` | options.cpp:2163 | Controls whether `MOUSE_MOVE` events are emitted (sdltiles.cpp:3530-3538) and whether the cursor is hidden on keypress (3360-3366) |
| `QUERY_KEYBIND_REMOVAL` | options.cpp:1914 | Asks before clearing bindings in the editor (input.cpp:1483) |
| `DIAG_MOVE_WITH_MODIFIERS_MODE` | options.cpp:1950-1990 | Diagonal modes via Ctrl/Shift arrows (sdltiles.cpp:2029-2080) |
| `R3D_WASD` | options.cpp:2667-2672 | WASD and arrow remap in 3D (input.cpp:960-993) |
| `ANDROID_*` joystick/finger options | options.cpp:3519-3569 | Android only |

---

## B. Color system

### B.1 Representation

- `nc_color` (color.h:346-392) wraps a single `int attribute_value`. It is a curses attribute: a colour
  pair index plus the bold/blink bits (`bold()`, `blink()`, color.h:346-372). It is not an RGB value.

```cpp
class nc_color {
    private:
        int attribute_value;   // curses attribute (pair index + bold/blink bits)
    ...
    static nc_color from_color_pair_index( int index );
    int to_color_pair_index() const;
```

- `color_id` (color.h:170-326) is an enum of **145 entries** (`def_` names). Each one is backed by an
  `add_color()` call in `color_manager::load_default()` (color.cpp:219-~400, 145 calls). Families:
  - `def_c_*`: 16 base colours (`black` .. `yellow`).
  - `def_h_*`: 16 highlight variants, used as the blue-background hilite (color.cpp:242; `hilite()` at color.cpp:509-513).
  - `def_i_*`: 16 inverted variants (color.cpp:259).
  - `def_c_unset`: sentinel.
  - About 96 two-colour pairs named `c_<fg>_<bg>` (for example `c_black_red` at color.cpp:276).
- Names are stored in `name_map` (color.h:418) and resolved by `name_to_color()` (color.cpp:102-108).
- `color_manager::get()` returns `entry.custom` when set, otherwise `entry.color` (color.cpp:149-159).
  `name_to_color()` does the same (color.cpp:102-108).

### B.2 Mapping colours to RGB (tiles)

The palette is 16 RGB entries, not per-colour RGB.

- Source: `data/raw/colors.json` (21 lines). Entries: `BLACK`, `RED`, `GREEN`, `BROWN`, `BLUE`, `MAGENTA`,
  `CYAN`, `GRAY`, `DGRAY`, `LRED`, `LGREEN`, `YELLOW`, `LBLUE`, `LMAGENTA`, `LCYAN`, `WHITE`, each as
  `[r,g,b]`.
- Loader: `color_loader<T>::load()` (color_loader.h:60-85). It copies `colors.json` to the user file
  `base_colors.json` if that does not exist (path `PATH_INFO::base_colors()`, path_info.cpp:176-178), and
  then reads the user file. On a JSON error it reports and falls back to the default file. The 16 values
  go into `windowsPalette`.
- SDL tiles: `windowsPalette` is loaded at sdltiles.cpp:3992. `FontFallbackList` and the font objects
  receive that palette (sdltiles.cpp:3999-4006; `palette_array` at sdl_font.h:23).
- Pair to RGB: `curses_color_to_SDL()` (sdl_utils.cpp:195-207):

```cpp
int palette_index = pair.FG != 0 ? pair.FG : pair.BG;
if( color.is_bold() ) {
    palette_index += color_loader<SDL_Color>::COLOR_NAMES_COUNT / 2;   // +8
}
return windowsPalette[palette_index];
```

  So bold selects the bright half of the 16-entry palette. The curses build does the same
  (`ncurses::color_to_RGB`, ncurses_def.cpp:51-62).
- Wrappers used by tiles: `curses_color_to_RGB()` (hsv_color.cpp:96-103), `color_as_sdl()`
  (sdltiles.cpp:428-431), and the font's `palette[color]` lookup (sdl_font.cpp:97).
- Tileset tint: colour names in tileset JSON go through `colors.name_to_color()` and then
  `curses_color_to_RGB()` (cata_tiles.cpp:2470-2477).
- Direct palette access that bypasses the helper: cata_tiles.cpp:3406 uses `windowsPalette[catacurses::green]`.

### B.3 How JSON refers to colours

- `nc_color::deserialize()` (color.cpp:30-43) accepts an **integer** (legacy id) or a **string**.
- Strings go through `color_from_string()` (color.cpp:562-595). It prepends `c_` if the second character
  is not `_`, so `"light_red"` becomes `c_light_red`. It also rewrites the deprecated `lt`/`dk` suffixes
  to `light_`/`dark_` and logs a `debugmsg`. Unknown names return `c_unset` and raise an error in
  `deserialize`.
- Plain names such as `"red"` therefore resolve to `c_red`. Bright variants are `light_red` etc.

### B.4 Existing theme and colour-blind support

- **Colour-blind**: none found. Grep for `colorblind`, `colour blind`, `deuter`, `protan`, `tritan`,
  `high contrast`, `palette` (as a user option) and `theme` gives no colour-related hits.
- **Theme**: the only user-facing theme-like hooks are two files in the config directory.
  1. `base_colors.json`: edit the 16 RGB values. This changes the whole palette, for both tiles and
     curses, with no code change.
  2. `custom_colors.json` (path_info.cpp:196-199): remaps a colour **name** to another colour name.
     `color_manager::serialize`/`save_custom`/`load_custom` (color.cpp:1029-1067). The editor is opened
     by `ACTION_COLOR` → `all_colors.show_gui()` (handle_action.cpp:3045-3046; `show_gui` at color.cpp:744).
     Because `get()` honours `entry.custom`, the remap applies everywhere a colour name is resolved.

### B.5 Colour as the only carrier of meaning (samples)

- **Hunger/thirst/fatigue** (sidebar): text and colour together. `get_hunger_description()` returns a
  word such as "Hungry" or "Starving" with a colour (character.cpp:5431-5461). The sidebar prints both
  (panels.cpp:1279-1288). The word alone carries the state. Colour is redundant but present.
- **Morale**: colour is set only at the extremes (`>=10` green, `<=-10` red; panels.cpp:641-651). The
  emotion face glyph chosen by `morale_emotion()` (panels.cpp:837-...) carries the finer state.
- **Stat values** (STR, DEX and so on): `value_color()` (panels.cpp:625-638) buckets the value into four
  colours (>=75 green, >=50 yellow, >=25 red, else magenta). This is the only encoding of the bucket
  itself. The number is also shown (exact line not checked).
- **Item condition**: `damage_color()` (item.cpp:7303-) and `damage_symbol()` (item.cpp:7332-) give both a
  colour and a glyph (`++`, `||`, `|\`, `|.`, ...). Shape and colour together.
- **Hit chance / aim confidence**: no dedicated colour for "hit chance" was found in melee. The
  ranged aim UI defines `confidence_rating` with a symbol, a label and a colour
  (ranged.cpp:2019-2024; configs at ranged.cpp:2296-2301: `*` "Great" green, `+` "Normal" light_gray,
  `|` "Graze" magenta). The percentage is printed (ranged.cpp:2199, 2214). Colour, symbol and label all
  carry the meaning.
- **Keybinding editor**: state colours (unbound/local/global) are shown with a legend
  (input.cpp:1277-1290). The legend text names the states, so colour is not the sole carrier.

Net: in the samples checked, colour is almost always paired with text or a glyph. The exception is
the stat bucket colours.

### B.6 How hard is a palette remap layer?

What already exists:
- A 16-entry RGB palette, user-editable (`base_colors.json`). This is a palette swap, but it applies to
  the whole palette at once. It can be used to make a colour-blind-friendlier palette today without
  code changes.
- A per-name remap (`custom_colors.json`) that maps one colour name to another (B.4).

Where a new layer would hook in:
1. `windowsPalette` load (sdltiles.cpp:3992; `color_loader::load` at color_loader.h:60-85). A transform
   applied here reaches all font drawing and the tile palette lookups.
2. `curses_color_to_SDL()` (sdl_utils.cpp:195) and `ncurses::color_to_RGB()` (ncurses_def.cpp:51). These
   are the single points for pair-to-RGB conversion.
3. Direct `windowsPalette[...]` reads that bypass (2) (for example cata_tiles.cpp:3406).

Assessment (from reading only): **moderate**. The 16-entry palette and the two conversion helpers make
it small in principle. The risks are: any direct `windowsPalette[...]` read; the r3d renderer, which
uses its own colour values (not audited in detail); and the fact that a palette change cannot separate
hues that are already close unless the mapping is per-name rather than per-palette-index. A per-name
mapping already exists through `custom_colors.json`.

---

## C. Font and text rendering (SDL/tiles build)

### C.1 Engines

- Two engines, both in `src/sdl_font.cpp`:
  1. **TrueType/OpenType via SDL3_ttf**: `CachedTTFFont` (sdl_font.cpp:233). It opens fonts with
     `TTF_OpenFontWithProperties` (sdl_font.cpp:11-20). Glyphs are rendered on demand and cached as textures
     (`create_glyph`, sdl_font.cpp:332). Coverage is checked with `TTF_FontHasGlyph` (sdl_font.cpp:372-375).
     Font blending is passed in (`fontblending`, sdl_font.cpp:237-239, 650-655).
  2. **Bitmap sheet**: `BitmapFont` (sdl_font.cpp:407-). It reads a PNG/BMP and assumes a 16×16 grid
     (`glyphs->w / 16`, sdl_font.cpp:418-419).
- Engine choice is by file suffix in `Font::load_font()` (sdl_font.cpp:51-87): `.bmp`/`.png` tries bitmap
  first (user font dir, then the font dir), otherwise TTF. Failure returns `nullptr`.
- `.fon` files are treated as TTF, with a face test (`test_face_size`, sdl_font.cpp:25-44; applied at
  sdl_font.cpp:322-324) to work around SDL_ttf sizing issues.

### C.2 Configuration and fallback

- Config file: `data/raw/fonts.json`. It lists `typeface`, `map_typeface` and `overmap_typeface` as
  arrays of paths. It currently lists `data/font/Terminus.ttf` then `data/font/unifont.ttf` for all three.
- Override: `user_fontconfig()` = `config_dir/fonts.json` (path_info.cpp:224-226). If the user file does
  not exist it is copied from the default (font_loader.cpp:44-59). If the user file fails to parse, the
  loader falls back to the default file (font_loader.cpp:60-70). If the default fails, the game aborts
  (font_loader.cpp:70).
- Fallback order: the list is ordered. `FontFallbackList::OutputChar()` (sdl_font.cpp:671-687) uses the
  first font whose `isGlyphProvided()` returns true. The last font in the list is used for any glyph none
  of them provide. The per-glyph choice is cached in `glyph_font` (sdl_font.h:167,
  sdl_font.cpp:675-683).
- `unifont.ttf` is always appended as the last resort (`ensure_unifont_loaded`, font_loader.cpp:5-10;
  called from `load_throws`, font_loader.cpp:12-41).
- Typeface search: `CachedTTFFont` tries the typeface as given and with the user font dir, the font dir,
  and system directories (`/usr/share/fonts/`, `/usr/local/share/fonts/`, `$HOME/.fonts/` on POSIX,
  sdl_font.cpp:240-300) by adding suffixes `.ttf`, `.otf`, `.ttc`, `.fon`.

### C.3 Sizes and whether they are global

Options (options.cpp:2304-2321, in the tiles-only block, `COPT_CURSES_HIDE`):

| Option | Range | Default | Note |
|---|---|---|---|
| `FONT_BLENDING` | bool | false | Passed to TTF glyph creation |
| `FONT_WIDTH` | 8..100 | 8 | Cell width, main terminal |
| `FONT_HEIGHT` | 8..100 | 16 | Cell height, main terminal |
| `FONT_SIZE` | 8..100 | 16 | TTF point size (`<=0` means height-1, sdl_font.cpp:318-320) |
| `MAP_FONT_WIDTH/HEIGHT/SIZE` | 8..100 | 8/16/16 | Map window, separate font object |
| `OVERMAP_FONT_WIDTH/HEIGHT/SIZE` | 8..100 | 8/16/16 | Overmap window, separate font object |

All of these say "Requires restart".

- Wiring: `font_loader` gets the values at sdltiles.cpp:3933-3943. The main terminal's width and height are
  copied into the globals `::fontwidth` and `::fontheight` (sdltiles.cpp:147-148, 3944-3945). Those
  globals are used for window sizing and cell-to-pixel maths across the file (for example
  sdltiles.cpp:217-218, 654-656, 856).
- So **the main terminal cell size is global**. The map and overmap windows each have their own font object,
  and `get_window_dimensions()` picks the right one per window (sdltiles.cpp:4196-4215).
- Three font objects are created once in `WinCreate` (sdltiles.cpp:3999-4006) and released at shutdown
  (sdltiles.cpp:4067-4069). Nothing rebuilds them later.

### C.4 Runtime change

- **Font size: no runtime change.** No code path re-creates fonts after startup. The only creation
  sites are sdltiles.cpp:3999-4006.
- **Tileset zoom: yes, tiles only.** `ACTION_ZOOM_IN` / `ACTION_ZOOM_OUT` (handle_action.cpp:3252-3259)
  call `game::zoom_in()` / `zoom_out()`. Zoom steps are computed by `calc_next_zoom()` using
  `ZOOM_STEP_COUNT` (game.cpp:10565-10577; option at options.cpp:2488-2489). The result goes to
  `rescale_tileset()` (sdltiles.cpp:4180-4183), which calls `tilecontext->set_draw_scale()`. Only the tile
  art is scaled. The ASCII cell size and the other windows are not.
- **Display scale**: `SCALING_FACTOR` (options.cpp:2749-2755; values 1, 2, 4; requires restart) multiplies
  the window size (sdltiles.cpp:217-218, 2185-2186) and sets SDL logical presentation
  (sdltiles.cpp:1414-1416, 1654-1656).

### C.5 Other points

- ASCII line drawing: `USE_DRAW_ASCII_LINES_ROUTINE` (option text "SDL ASCII lines", options.cpp ~2300) selects
  `draw_ascii_lines()` (sdl_font.cpp:92-) instead of font glyphs for box-drawing characters. Colour comes
  from the palette (sdl_font.cpp:97).
- Font colour: each glyph is drawn with the 16-entry palette index, so text colour follows the palette
  (C.6, sdl_font.h:23).
- The Windows GDI path is separate (`wincurse.cpp:601-637`, `CreateFontW` with `fontheight`/`fontwidth`).
  It is not part of the SDL build.

### C.6 Bundled font files

- `data/font/Terminus.ttf` and `data/font/unifont.ttf`: used by `data/raw/fonts.json`.
- `data/font/terminus.fon`: present, but no JSON in the repo references it. It is reachable only through a
  user `fonts.json`.
- `data/font/map_font_LARWICK.png`: present, but no JSON in the repo references it. It would be loaded
  as a `BitmapFont`, which expects a 16×16 grid.

---

## Not found / uncertain

**Not found (searched, absent):**
- `src/input_context*` files. `input_context` lives in `src/input.h` and `src/input.cpp`.
- `data/raw/keybindings.json`. The file is `data/raw/keybindings/keybindings.json`.
- `fontdata.json`. The font config is `data/raw/fonts.json` (path_info.cpp:220-222).
- Any `SDL_Gamepad`, `SDL_INIT_GAMEPAD`, `SDL_EVENT_GAMEPAD_*` or `SDL_CONTROLLER` use. Only the legacy
  `SDL_Joystick` API is used.
- `SDL_EVENT_JOYSTICK_ADDED` / `_REMOVED` handling (hot-plug).
- Writes to `input_event::modifiers` (only reads exist).
- Any mouse middle-button binding or event (only the camera uses it).
- Any colour-blind mode, colour-blind palette option, or named theme option.
- Any runtime font re-creation or runtime font-size option.
- Use of `SCROLLWHEEL_*` events by the 3D renderer.
- `display_help` as a keybinding editor. The keybinding editor is `input_context::display_menu`
  (input.cpp:1206). `help::display_help` (help.cpp:143) is the in-game help.

**Uncertain (code reading only, not run):**
- Whether a physical joystick button can ever match a `JOY_n` binding. The code analysis says no (A.6),
  but this should be confirmed on hardware. Also unknown: what the raw button index maps to in
  `get_keyname` for keyboard codes (for example button 10 → Enter).
- The click coordinate path when `SCALING_FACTOR > 1`. `window_pos_pixel` is computed from unscaled
  `fontwidth` (sdltiles.cpp:4225-4226) while the size uses the scaled font size (4212). The r3d
  motion path divides by `scaling_factor` (sdltiles.cpp:3508, 3512-3516), but `get_coordinates()` passes
  the raw click point to `r3d::pick()` (sdltiles.cpp:4300-4310 region). Whether these are consistent at
  scale 2 or 4 has not been checked.
- Whether events are lost in practice when several arrive in one drain batch (single-slot `last_input`,
  sdltiles.cpp:3277 and 3290 loop). The code suggests yes.
- DIAG mode 3 (L/R tilt) behaviour (options.cpp:1950-1990; not read).
- Exact count of `ACTION_*` enumerators (grep gives about 168; not verified by compile).
- Whether `base_colors.json` edits are picked up without restart (the palette is loaded once at startup,
  sdltiles.cpp:3992, so probably not). Not verified.
- Whether the r3d renderer takes its colours from `nc_color`/palette or from its own values. Not audited.
- Stat-value number placement in the sidebar (panels.cpp:625-638 gives only the colour function).

**Not searched:** complex-script shaping or ligatures in TTF text; IME handling beyond `edit`/`edit_refresh`;
Android-specific input beyond the virtual joystick lookup; runtime behaviour of any of the above.
