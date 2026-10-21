# L. Codebase UI audit: screens, primitives and text rendering

Research date: 2026-10-21. Branch: `feat/ui-overhaul`. Read-only audit of `src/`. Line counts are from `wc -l`. Line references were read in the source during this audit.

Key for the table:

- **Primitives** use the counts from `rg -c` per file: `ui_adaptor`, `uilist`, `input_context`, `scrollable` (scrollable_text / scrollable views), `string_input_popup`, `query_popup|query_yn`, `draw_border`, `mvwprintz|mvwprintw`. Counts of 0 mean the file does not use that primitive directly.
- **Mouse** is inferred from registered input actions, not run. `uilist` registers `SELECT` and `SCROLL_UP/DOWN` (`ui.cpp:230-235`, `ui.cpp:271`). The default bindings map `SELECT` to `MOUSE_LEFT` and `SCROLL_UP/DOWN` to the wheel (`data/raw/keybindings/keybindings.json`, around lines 1209-1221 and 3210-3220). So a uilist accepts a left-click that activates the **highlighted** row, and the wheel. `ui.cpp` contains no coordinate hit-test, so a click does not select the row under the pointer. Explicit `COORDINATE` (cursor-pick) support exists only in `game.cpp:3818`, `game.cpp:10123` (look-around), `ranged.cpp:3284` (targeting) and `popup.cpp:292`.
- **Modal / full-screen**: every screen runs its own blocking input loop, so all are modal. "Full-screen" means the file sizes its windows from terminal dimensions (`TERMX`/`TERMY`/`FULL_SCREEN` references, counted per file). This is an inference from code, not a runtime check.
- Paths are under `/home/richard/Projects/Cataclysm-BN-3D/src/` unless shown otherwise.

## 1. Screen and menu inventory

Default keys are from `data/raw/keybindings/keybindings.json`. Dispatch is from `src/handle_action.cpp`.

| Screen | Entry (key / action) | Files (absolute) | LOC | Primitives (ui_ad / uilist / input_ctx / scroll / strpop / query / border / mvwprint) | Mouse | Modal / full-screen |
|---|---|---|---|---|---|---|
| Main menu, opening screen | Startup. `opening_screen()` at `main_menu.cpp:611`. ESC is bound to `main_menu`. (Startup call site not traced.) | `/home/richard/Projects/Cataclysm-BN-3D/src/main_menu.cpp` | 1218 | 3 / 9 / 1 / 0 / 0 / 7 / 2 / 0 | uilist click/wheel in sub-menus | modal, full-screen |
| New character creation | Main menu "New Game" tab, `main_menu.cpp:889` (`new_character_tab`), calls `avatar::create` at `newcharacter.cpp:579`. Tab setters: `set_points` 913, `set_stats`, `set_traits`, `set_bionics`, `set_magic`, `set_scenario`, `set_profession`, `set_skills`, `set_description`. | `/home/richard/Projects/Cataclysm-BN-3D/src/newcharacter.cpp` | 4799 | 27 / 4 / 11 / 1 / 5 / 15 / 1 / 114 | uilist click/wheel (partial) | modal, full-screen (tabbed) |
| Load character / world tab | `main_menu.cpp:1018` (`load_character_tab`), `main_menu.cpp:1080` (`world_tab`) | `/home/richard/Projects/Cataclysm-BN-3D/src/main_menu.cpp`; world list and mods: `/home/richard/Projects/Cataclysm-BN-3D/src/worldfactory.cpp` | 1648 (worldfactory) | worldfactory: 18 / 0 / 5 / 0 / 4 / 5 / 4 / 11 | keyboard only | modal, full-screen |
| Inventory | `i` (`inventory`). `game_menus::inv::common` at `game_inventory.cpp:266`. Selector class `inventory_selector` at `inventory_ui.cpp:1246+`; drawing: `draw_columns` 1860, `draw_frame` 1899, `draw_footer` 1917; input: `on_input` 2017. | `/home/richard/Projects/Cataclysm-BN-3D/src/inventory_ui.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/game_inventory.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/inventory_ui.h` (799 lines) | 2840 (inventory_ui.cpp); 2341 (game_inventory.cpp) | inventory_ui: 15 / 0 / 0 / 0 / 3 / 0 / 1 / 5. game_inventory: 3 / 0 / 1 / 0 / 0 / 0 / 0 / 0 | keyboard only (no SELECT/COORDINATE registered in these files) | modal, full-screen (columns) |
| Wear, wield, read, gun-mod pick | `W` wear (`handle_action.cpp:1572`), `w` wield, `R` read (`handle_action.cpp:1598`). These use inventory pickers `game_menus::inv::wear/read`. Gun-mod picker: `gunmod_inventory_preset` at `game_inventory.cpp:930`. | `/home/richard/Projects/Cataclysm-BN-3D/src/game_inventory.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/handle_action.cpp` | (see inventory) | (see inventory) | keyboard only | modal, popup-sized picker |
| Reload (ammo selection, target pick) | `r` reload_item (keybindings.json). Called from `avatar_action.cpp:1528` (`reload_ui::select_ammo`) and `avatar_action.cpp:1636` (`reload_ui::pick_target`). | `/home/richard/Projects/Cataclysm-BN-3D/src/reload/reload_ui.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/reload/reload_ui.h`, `/home/richard/Projects/Cataclysm-BN-3D/src/reload/reload.cpp` | 469 | 0 / 3 / 2 / 0 / 0 / 0 / 0 / 0 | uilist click/wheel (partial) | modal, popup |
| Advanced inventory | `/` (`advinv`). `create_advanced_inv()` at `handle_action.cpp:2546`. | `/home/richard/Projects/Cataclysm-BN-3D/src/advanced_inv.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/advanced_inv_pane.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/advanced_inv_area.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/advanced_inv_listitem.cpp` | 1930 + 282 + 470 + 73 = 2755 | advanced_inv: 5 / 7 / 5 / 0 / 4 / 3 / 3 / 26 (pane and area: 0 direct uses) | uilist click/wheel (partial) | modal, full-screen (two panes) |
| Crafting / recipe browser | `&` (`craft`). `handle_action.cpp:2792` calls `u.craft()`, which goes to `crafting.cpp:674` (`craft_command`). Recipe picker: `select_crafting_recipe` at `crafting_gui.cpp:675`, input context at `crafting_gui.cpp:644`. | `/home/richard/Projects/Cataclysm-BN-3D/src/crafting_gui.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/crafting.cpp` (2600 lines, logic) | 2144 | 3 / 6 / 4 / 0 / 2 / 1 / 0 / 4 | uilist click/wheel (partial) | modal, full-screen |
| Construction menu | `*` (`construct`). `handle_action.cpp:2850` calls `construction_menu( false )` at `construction.cpp:416`. Blueprint variant also used from `clzones.cpp:443`. | `/home/richard/Projects/Cataclysm-BN-3D/src/construction.cpp` | 2493 | 3 / 1 / 1 / 0 / 3 / 1 / 1 / 3 | uilist click/wheel (partial) | modal, full-screen |
| Pickup | `g` (`pickup`). `game::pickup` at `game.cpp:8941`, selector `do_pickup` at `pickup.cpp:448`, entry `pickup::pick_up` at `pickup.cpp:1237`. | `/home/richard/Projects/Cataclysm-BN-3D/src/pickup.cpp` | 1532 | 3 / 2 / 3 / 0 / 4 / 1 / 0 / 4 | uilist click/wheel (partial) | modal, popup |
| Examine (`e`) and furniture/item examine | `e` (`examine`). `game::examine()` at `game.cpp:8702`. Item examine: `examine_item_menu.cpp`. Furniture menus: `iexamine.cpp`. | `/home/richard/Projects/Cataclysm-BN-3D/src/examine_item_menu.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/iexamine.cpp` (8865, mostly gameplay) | 552 + 8865 | examine_item: 3 / 2 / 1 / 0 / 0 / 0 / 0 / 0. iexamine: 3 / 43 / 1 / 0 / 8 / 71 / 0 / 0 | uilist click/wheel (partial) | modal, popup |
| Vehicle interaction | Examine a vehicle: `game::exam_vehicle` at `game.cpp:7967`, which calls `veh_interact::run`. Entry key not traced (`e` examines). Parts/tanks picker: `veh_interact::select_part` (used from `character.cpp:7826`). | `/home/richard/Projects/Cataclysm-BN-3D/src/vehicle/veh_interact.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/vehicle/veh_interact.h`, `/home/richard/Projects/Cataclysm-BN-3D/src/vehicle/vehicle_display.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/vehicle/vehicle_preview.cpp` | 3270 + 476 + 248 | veh_interact: 11 / 12 / 3 / 0 / 4 / 1 / 1 / 8. vehicle_display: 0 / 0 / 0 / 0 / 0 / 0 / 0 / 11 | uilist click/wheel (partial) | modal, full-screen |
| Look around (`x` / `;`) | `look_around()` at `game.cpp:10061`. Registers `COORDINATE` at `game.cpp:10123`. | `/home/richard/Projects/Cataclysm-BN-3D/src/game.cpp` (17323, mixed gameplay) | (shared with game.cpp) | game.cpp: 50 / 13 / 16 / 2 / 5 / 28 / 5 / 62 | **COORDINATE: cursor-pick** plus mouse hits (game.cpp has 9 mouse-constant references) | modal, full-screen (map overlay) |
| Item info (item stats text) | Built by `item::info()` at `item.cpp:4522` (iteminfo vector). Rendered by `draw_item_info` (`output.cpp:806`, `988`, `995`), data struct `item_info_data` at `output.h:493`. Shown from inventory and examine. | `/home/richard/Projects/Cataclysm-BN-3D/src/item.cpp` (12007, mostly gameplay data), `/home/richard/Projects/Cataclysm-BN-3D/src/output.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/output.h` | (item.cpp) / 2233 / 1049 | output.cpp: 6 / 0 / 5 / 5 / 3 / 5 / 4 / 9. item.cpp: 0 | keyboard only | modal, popup |
| Character sheet (`@`) | `@` (`player_data`). `character_display::disp_info` at `character_display.cpp:1306`. Tabs (`draw_*_tab`): stats 401, encumbrance 549, traits 584, bionics 633, effects 698, skills 806, speed 945. | `/home/richard/Projects/Cataclysm-BN-3D/src/character_display.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/character_display.h` | 1751 | 38 / 0 / 3 / 0 / 3 / 1 / 8 / 39 | keyboard only | modal, full-screen (tabbed) |
| Skills | No separate screen. Skills is a tab in the character sheet (`draw_skills_tab`, `character_display.cpp:806`, `draw_skills_info` 912). | (see character sheet) | (see above) | (see above) | keyboard only | modal, full-screen (tab) |
| Mutations | `[` (`mutations`). `show_mutations_ui` at `mutation_ui.cpp:106`. | `/home/richard/Projects/Cataclysm-BN-3D/src/mutation_ui.cpp` | 643 | 3 / 0 / 2 / 0 / 0 / 4 / 1 / 6 | keyboard only | modal, full-screen |
| Bionics | `p` (`bionics`). `show_bionics_ui` at `bionics_ui.cpp:557`. (Bionics logic in `bionics.cpp`, 3263 lines.) | `/home/richard/Projects/Cataclysm-BN-3D/src/bionics_ui.cpp` | 968 | 6 / 1 / 2 / 0 / 0 / 0 / 1 / 3 | uilist click/wheel (partial) | modal, full-screen |
| Perks | `CTRL+P` (`perks`). `draw_perk_menu` at `perk_menu.cpp:57`. | `/home/richard/Projects/Cataclysm-BN-3D/src/perk_menu.cpp` | 215 | 3 / 0 / 1 / 0 / 0 / 0 / 2 / 0 | keyboard only | modal, popup |
| Armour layers (sort armour) | `+` (`sort_armor`). `show_armor_layers_ui` (`handle_action.cpp:2784`). Implementation in `armor_layers.cpp` (1000 lines, not read in depth). | `/home/richard/Projects/Cataclysm-BN-3D/src/armor_layers.cpp` | 1000 | not counted | not checked | modal |
| Overmap map | `m` (`map`). `ui::omap::display()` at `overmap_ui.cpp:2245` (static `display` at 1993). Notes manager at 643. | `/home/richard/Projects/Cataclysm-BN-3D/src/overmap/overmap_ui.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/overmap/overmap_ui.h` | 2312 | 20 / 8 / 8 / 0 / 4 / 5 / 4 / 50 | **SELECT + mouse hits (4 MOUSE, 2 SELECT)**; cursor set at 1311 | modal, full-screen |
| Messages log | `P` (`messages`). `Messages::display_messages()` at `messages.cpp:848`. | `/home/richard/Projects/Cataclysm-BN-3D/src/messages.cpp` | 939 | 7 / 0 / 2 / 0 / 2 / 0 / 2 / 5 | keyboard only | modal, full-screen |
| Sidebar panels | Not a menu. Layouts chosen from `panels.cpp:2588` (`initialize_default_panel_layouts`): `classic`, `compact`, `labels-narrow`, `labels`. Default `labels` set at `panels.cpp:2608`. `SIDEBAR_POSITION` option read at `panels.cpp:2630`. Panel-manager UI (`}` toggle, `panel_mgmt`) at `panels.cpp:2860` (`show_adm`). | `/home/richard/Projects/Cataclysm-BN-3D/src/panels.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/panels.h`, `/home/richard/Projects/Cataclysm-BN-3D/src/panels_utility.cpp` | 3040 | 8 / 0 / 1 / 0 / 0 / 0 / 1 / 255 | keyboard only (1 mouse hit) | not modal in play; `show_adm` modal |
| Options | `open_options` action (`handle_action.cpp:3025` calls `get_options().show( true )`). Implementation `options_manager::show` at `options.cpp:3886`. Key not verified. | `/home/richard/Projects/Cataclysm-BN-3D/src/options.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/options.h` | 4671 | 7 / 0 / 1 / 0 / 4 / 1 / 5 / 6 | keyboard only (1 mouse hit) | modal, full-screen |
| Keybindings menu | `?` (`HELP_KEYBINDINGS`). `input_context::display_menu` at `input.cpp:1206`. | `/home/richard/Projects/Cataclysm-BN-3D/src/input.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/input.h` | 1726 (input.cpp), 779 (input.h) | input.cpp: 3 / 0 / 59 / 0 / 2 / 4 / 1 / 7 | keyboard + 8 mouse hits in input.cpp | modal, full-screen |
| Action menu (`Enter`) | `RETURN` (`action_menu`) and `%` (`item_action_menu`). Action menu entries built in `action.cpp` (`make_register_actions` at 1013; menu at ~1129). | `/home/richard/Projects/Cataclysm-BN-3D/src/action.cpp` | 1599 | 1 / 11 / 10 / 0 / 0 / 0 / 0 / 0 | uilist click/wheel (partial) | modal, popup |
| Help | `0` (`help`). `help::display_help()` at `help.cpp:143`. Text viewer uses `scrollable_text` (`output.h:325`). | `/home/richard/Projects/Cataclysm-BN-3D/src/help.cpp` | 234 | 3 / 0 / 0 / 1 / 0 / 0 / 1 / 0 | keyboard only | modal, full-screen |
| Safemode | `!` (`safemode`). `safemode::show()` at `safemode_ui.cpp:40`, `show( name, is_safemode )` at 51. | `/home/richard/Projects/Cataclysm-BN-3D/src/safemode_ui.cpp` | 858 | 9 / 0 / 2 / 0 / 3 / 2 / 3 / 16 | keyboard only | modal, full-screen |
| Autopickup manager | `open_autopickup` (no default key checked). `user_interface::show()` at `auto_pickup.cpp:55`; player and NPC settings at 403 and 779. | `/home/richard/Projects/Cataclysm-BN-3D/src/auto_pickup.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/auto_pickup.h` | 824 | 9 / 0 / 2 / 0 / 2 / 2 / 3 / 9 | keyboard only | modal, full-screen |
| Zones manager | `Y` (`zones`). `game::zones_manager()` at `game.cpp:9548`; implementation `clzones.cpp`. | `/home/richard/Projects/Cataclysm-BN-3D/src/clzones.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/clzones.h` (490 lines) | 1627 | 0 / 3 / 0 / 0 / 3 / 1 / 0 / 0 | keyboard only | modal, full-screen |
| Missions | `M` (`missions`). `game::list_missions()` at `mission_ui.cpp:22`. | `/home/richard/Projects/Cataclysm-BN-3D/src/mission_ui.cpp` | 219 | 3 / 0 / 1 / 0 / 0 / 0 / 1 / 4 | keyboard only | modal, popup |
| Factions | `#` (`factions`). `faction_manager::display()` at `faction.cpp:610`. | `/home/richard/Projects/Cataclysm-BN-3D/src/faction.cpp` | 911 | 3 / 0 / 1 / 0 / 0 / 0 / 1 / 20 | keyboard only | modal, full-screen |
| NPC dialogue / chat | Talk to NPC: `game::chat()` at `npctalk.cpp:428`. Dialogue window class `dialogue_window` at `dialogue_win.h:21`. | `/home/richard/Projects/Cataclysm-BN-3D/src/npctalk.cpp` (3940, mixed chat and gameplay), `/home/richard/Projects/Cataclysm-BN-3D/src/dialogue_win.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/dialogue_win.h` | 3940 + 281 | npctalk: 3 / 4 / 1 / 0 / 3 / 3 / 0 / 0. dialogue_win: 1 / 0 / 0 / 0 / 0 / 0 / 1 / 11 | uilist click/wheel (partial) | modal, popup over map |
| Trade | Entered from NPC dialogue. `trading_window` at `trade_win.h:16`; `show_item_data` at `trade_win.cpp:666`. | `/home/richard/Projects/Cataclysm-BN-3D/src/trade_win.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/trade_win.h` | 1482 | 7 / 0 / 4 / 0 / 3 / 2 / 6 / 48 | keyboard only | modal, full-screen |
| Gun mods, reload, wear (see above) | See rows above | | | | | |
| Debug menu | `F12` (`debug`). `debug()` at `debug_menu.cpp:1517`. | `/home/richard/Projects/Cataclysm-BN-3D/src/debug_menu.cpp` | 2340 | 2 / 164 / 0 / 0 / 9 / 2 / 0 / 0 | uilist click/wheel + 3 mouse hits | modal, popup |
| Lua console | `` ` `` (`lua_console`). `show_lua_console_impl()` at `catalua_console.cpp:151`. | `/home/richard/Projects/Cataclysm-BN-3D/src/catalua_console.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/catalua_console.h` | 417 | 3 / 0 / 1 / 0 / 0 / 0 / 1 / 12 | keyboard only | modal, full-screen |
| Color manager | `open_color` action. `all_colors.show_gui()` (`handle_action.cpp:3045`). | `/home/richard/Projects/Cataclysm-BN-3D/src/color.cpp` (1088) | 1088 | color.cpp not counted for primitives | not checked | modal |
| Diary | `HOME` (`diary`). `diary::show_diary_ui()` (`handle_action.cpp:2987`). | `/home/richard/Projects/Cataclysm-BN-3D/src/diary_ui.cpp` | 411 | 12 / 0 / 7 / 11 / 1 / 3 / 3 / 31 | keyboard only | modal, full-screen |
| Scores / achievements | `)` (`scores`). `show_scores_ui()` at `scores_ui.cpp:69`. | `/home/richard/Projects/Cataclysm-BN-3D/src/scores_ui.cpp` | 217 | 6 / 0 / 2 / 0 / 0 / 0 / 2 / 0 | keyboard only | modal, popup |
| Morale | `v` (`morale`). `u.disp_morale()` (`handle_action.cpp:2995`). Implementation not traced. | `/home/richard/Projects/Cataclysm-BN-3D/src/morale.cpp` (1148) | 1148 | not counted | not checked | modal |
| Status report (text) | `F4` (`status_report`). `status_report( u )` at `status_report.cpp:69` builds one text string. `handle_action.cpp:3287` calls `add_msg` and `popup()`. | `/home/richard/Projects/Cataclysm-BN-3D/src/status_report.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/status_report.h` | 157 | 0 (uses `popup()`) | keyboard only | modal, popup |
| 3D HUD overlay (r3d) | Not a menu. Text in `sdltiles.cpp:1611` (`draw_r3d_hud_texts`). Geometry and arrows drawn with `SDL_Renderer` in `r3d_hud.cpp` (`fill_triangles` 141, `draw_arrow` 195, `draw_edge_arrows` 210). Math in `r3d_hud_math.cpp`. | `/home/richard/Projects/Cataclysm-BN-3D/src/r3d_hud.cpp` (624), `/home/richard/Projects/Cataclysm-BN-3D/src/r3d_hud_math.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/sdltiles.cpp` | 624 | 0 (no catacurses widgets) | not applicable | not modal (overlay) |
| Onboarding overlay and checklist | `onboarding.cpp` `manager::show_popup( lesson )` at 894; `show_checklist()` at 1195. Core logic `onboarding_core.cpp` (343), header `onboarding_core.h` (199), `onboarding.h` (73), `onboarding_input.h` (8). | `/home/richard/Projects/Cataclysm-BN-3D/src/onboarding.cpp` (1361), `/home/richard/Projects/Cataclysm-BN-3D/src/onboarding_core.cpp`, `/home/richard/Projects/Cataclysm-BN-3D/src/onboarding.h`, `/home/richard/Projects/Cataclysm-BN-3D/src/onboarding_core.h` | 1361 + 343 + 199 + 73 + 8 | onboarding.cpp: 0 / 2 / 2 / 0 / 0 / 1 / 0 / 0 | keyboard only | modal, popup |
| Salvage | `ACTION_SALVAGE` (no default key checked). `salvage.cpp` (538). | `/home/richard/Projects/Cataclysm-BN-3D/src/salvage.cpp` | 538 | 0 / 0 / 2 / 0 / 0 / 2 / 0 / 0 | keyboard only | modal, popup |
| Computer terminal | Map computer, `computer_session.cpp` (1703). | `/home/richard/Projects/Cataclysm-BN-3D/src/computer_session.cpp` | 1703 | 3 / 1 / 3 / 0 / 0 / 7 / 1 / 0 | uilist click/wheel (partial) | modal, full-screen |
| Loading screen | Shown during world load. `loading_ui.cpp` (570). | `/home/richard/Projects/Cataclysm-BN-3D/src/loading_ui.cpp` | 570 | 3 / 1 / 0 / 0 / 0 / 0 / 1 / 1 | keyboard only | modal, full-screen |
| String editor (multi-line text) | Used by notes and other text fields. `string_editor_window.cpp` (749). | `/home/richard/Projects/Cataclysm-BN-3D/src/string_editor_window.cpp` | 749 | 4 / 0 / 1 / 0 / 0 / 0 / 0 / 3 | keyboard only | modal, popup |
| Character preview (in creation) | `character_preview.cpp` (370). | `/home/richard/Projects/Cataclysm-BN-3D/src/character_preview.cpp` | 370 | 0 / 0 / 0 / 0 / 0 / 0 / 1 / 0 | keyboard only | embedded in creation |
| Pixel minimap | Map overlay. `pixel_minimap.cpp` (708). Toggle `toggle_pixel_minimap`. | `/home/richard/Projects/Cataclysm-BN-3D/src/pixel_minimap.cpp` | 708 | not counted | not checked | not modal (overlay) |
| Game map (main view) | Not a menu. Tiles: `cata_tiles.cpp` (7552), SDL side `sdltiles.cpp` (4463). Curses: `game.cpp` drawing. | see section 2 | see section 2 | see section 2 | COORDINATE in default context (`game.cpp:3818`) | not modal |

Notes on the table:

- The "Mouse" column is the broadest uncertainty. A left-click on a uilist activates the highlighted row, but the code never hit-tests the click position. That is a reading of `ui.cpp` and needs a runtime check.
- The "Modal / full-screen" column is partly inferred from window-sizing references, as described above.

Sub-counts (LOC for the screen files above, de-duplicated, from `wc -l`):

- Screen-file set (sum of the files in section 1, excluding `game.cpp`, `item.cpp`, `iexamine.cpp`, which are mostly gameplay): **64,931 LOC**. This includes `npctalk.cpp` (3,940, mixed chat and gameplay).
- Primitive set (`ui.{h,cpp}`, `ui_manager.{h,cpp}`, `output.{h,cpp}`, `popup.{h,cpp}`, `string_input_popup.{h,cpp}`, `cursesdef.h`, `ncurses_def.{h,cpp}`, `cursesport.{h,cpp}`, `sdltiles.{h,cpp}`, `sdl_font.cpp`, `font_loader.cpp`, `cata_tiles.{h,cpp}`, `color.{h,cpp}`, `input.h`, `ui_theme.{h,cpp}`, `drawing_primitives.cpp`): **25,472 LOC**.
- Combined (no file in both sets): **90,403 LOC**.
- Five largest UI screen files: `newcharacter.cpp` 4799, `options.cpp` 4671, `npctalk.cpp` 3940 (mixed), `vehicle/veh_interact.cpp` 3270, `panels.cpp` 3040.
- Five largest rendering and UI-infrastructure files: `cata_tiles.cpp` 7552, `sdltiles.cpp` 4463, `output.cpp` 2233, `ui.cpp` 1228, `color.cpp` 1088.

## 2. Primitives and text rendering path

### 2.1 Window abstraction (`catacurses`)

- `src/cursesdef.h:52` defines `class window`. `cursesdef.h:40` declares `init_interface()`. `cursesdef.h:94-130` declares the wrapper functions: `wborder`, `mvwhline`, `mvwvline`, `wnoutrefresh`, `wrefresh`, `refresh`, `mvwprintw`, `wprintw`, `werase`, `wmove`, `mvwaddch`, `wattron`, `wattroff`, `curs_set`.
- `cursesdef.h:20-27` documents the three backends: ncurses (`ncurses_def.cpp`), the in-house `cata_cursesport` (`cursesport.cpp`), and Windows curses.
- Terminal build: `src/ncurses_def.cpp:80` (`wnoutrefresh`), `:85` (`wrefresh`), `:152` (`refresh`), `:157` (`refresh_display`). `ncurses_def.cpp:306` registers the ncurses mouse.
- Tiles build: `src/cursesport.cpp:1` is compiled only under `#if defined(TILES) || defined(_WIN32)`. The SDL frame is presented in `src/sdltiles.cpp:531` (`SDL_RenderPresent`).

### 2.2 Drawing helpers (`output.h`, implemented in `output.cpp`)

- `mvwprintz` declared `output.h:342`, defined `output.cpp:2220`. Variadic wrapper at `output.h:344`.
- `draw_border` declared `output.h:364`, defined `output.cpp:585`. It checks `get_ui_theme()` (`output.cpp:588`).
- `draw_custom_border` `output.h:359`. `border_helper` `output.h:368`.
- `draw_tabs` `output.h:768`, `:771`, `:785`, `:807`. `draw_tab` `:755`, `draw_subtab` `:757`.
- `draw_scrollbar` `output.h:822`. `scrollbar` class `:828`.
- `scrollable_text` declared `output.h:325`, defined `output.cpp:362`.
- `scrolling_text_view` `output.h:871`. `scrollingcombattext` `output.h:892`.
- `print_colored_text` `output.h:211`. `trim_and_print` `:296`. `center_print` `:318`. `insert_table` `:322`.
- `draw_item_info` overloads: `output.h:534`, `:536`, `:538`; defined at `output.cpp:806`, `:988`, `:995`. Data struct `item_info_data` at `output.h:493`.
- `refresh_display()` declared `output.h:1011`, defined in `ncurses_def.cpp:157`.

### 2.3 Uilist (`ui.h`, `ui.cpp`)

- `uilist_entry` `ui.h:55`. `uilist_callback` `ui.h:169`. `class uilist` `ui.h:189` (forward decl at `:159`). `pointmenu_cb` `ui.h:446`.
- `uilist::init()` `ui.cpp:152`. `uilist::query()` `ui.cpp:1042`.
- Registered actions: `SCROLL_UP` / `SCROLL_DOWN` at `ui.cpp:230-231` and `:271-272`, `PAGE_UP` / `PAGE_DOWN` at `:226-227` and `:267-268`, `SELECT` at `ui.cpp:235`.
- Scroll bar: `uilist::apply_scrollbar()` at `ui.cpp:755` uses `scrollbar()`.

### 2.4 Popups and text input

- `class query_popup` `popup.h:36`. `static_popup` `popup.h:275`. `throbber_popup` `popup.h:310`. `popup.cpp:292` registers `COORDINATE`.
- `class string_input_popup` `string_input_popup.h:47`. Query loop `string_input_popup.cpp:340`. Cursor set for screen readers at `string_input_popup.cpp:326`.

### 2.5 Input routing (`input.h`, `input.cpp`)

- `enum mouse_buttons` `input.h:89`: `MOUSE_BUTTON_LEFT`, `MOUSE_BUTTON_RIGHT`, `SCROLLWHEEL_UP`, `SCROLLWHEEL_DOWN`, `MOUSE_MOVE`.
- `class input_manager` `input.h:232`. `class input_context` `input.h:399`. Coordinate action documented at `input.h:518`.
- `input.cpp:82` `is_mouse_enabled()`. Key names `MOUSE_LEFT` / `SCROLL_UP` mapped at `input.cpp:444-448`.
- Mouse events are translated to actions at `input.cpp:1059-1071`: an unbound mouse event is dropped unless the context is handling coordinates.
- `input_context::register_action` `input.cpp:733`. Built-in directional actions `LEFTUP` etc. at `input.cpp:1098-1101`.
- `input_context::display_menu` (keybindings menu) at `input.cpp:1206`.

### 2.6 Screen composition (`ui_manager`)

- `class ui_adaptor` `ui_manager.h:65`: redraw callbacks (`redraw_callback_t` at `:68`), `disable_uis_below` at `:71`.
- `class background_pane` `ui_manager.h:244`. `invalidate()` `:262`, `redraw()` `:270`, `redraw_invalidated()` `:274`, `screen_resized()` `:279`. Implementation `ui_manager.cpp` (489 lines).
- Screens register a `ui_adaptor` and draw in its callback, which is how most newer screens redraw.

### 2.7 Colours and theme

- `color.h` / `color.cpp`: `color_manager::name_to_color` `color.cpp:102`, `color_to_id` `:130`, `get` `:149`, `get_invert` `:167`, `get_highlight` `:189`.
- Terminal palette: `ncurses_def.cpp:252-268` asserts that base colours match ncurses constants.
- Tiles palette: `sdltiles.cpp:117` (`windowsPalette`), lookup at `sdltiles.cpp:430`. Used for glyph colouring at `sdl_font.cpp:338-339` and `:465`.
- Theme: `ui_theme.h:6-13` (`struct ui_theme`: `modern_borders`, `border`, `title`), `ui_theme.cpp:5` `get_ui_theme()`. Selected by the `MODERN_MENUS` option (`options.cpp:2546`). The only caller found is `output.cpp:588` (`draw_border`). The theme does not alter `color.cpp` pairs.

### 2.8 Tiles text rendering (SDL build)

- `sdltiles.cpp:183` `TTF_Init()`. `sdltiles.cpp:188` `atexit(TTF_Quit)`.
- Font selection: `sdltiles.cpp:3932` (`font_loader fl`). `font_loader.cpp:8` `ensure_unifont_loaded` adds `unifont.ttf` from `PATH_INFO::fontdir()`. `font_loader.cpp:12` `load_throws`, `:44` `load()`.
- `sdl_font.cpp:51` `Font::load_font()`. If the typeface ends with `.bmp` or `.png` it is treated as a bitmap font (`sdl_font.cpp:57-80`). Otherwise TrueType (`sdl_font.cpp:81` onward). `sdl_font.cpp:12` `open_font_index()` uses `TTF_OpenFontWithProperties` (SDL3_ttf API). `sdl_font.cpp:243` lists known suffixes `.ttf .otf .ttc .fon`.
- Glyph rasterisation: `sdl_font.cpp:338-339`, `TTF_RenderText_Blended` when blending is on, else `TTF_RenderText_Solid`, both with `windowsPalette[color]`. `Font::draw_ascii_lines` at `sdl_font.cpp:94` draws line-drawing glyphs.
- Conclusion: text in the tiles build is **SDL_ttf (SDL3_ttf) with a bitmap-font fallback path**, not ImGui and not a plain bitmap atlas. Default TTF is `unifont.ttf`.
- Map: `cata_tiles.h:736` `class cata_tiles`. `cata_tiles.cpp` (7552 lines). Context created at `sdltiles.cpp:3952`. Overmap drawing `cata_tiles::draw_om` `cata_tiles.cpp:826` (and `sdltiles.cpp` references).
- r3d overlay: drawn with raw `SDL_Renderer` in `r3d_hud.cpp` and text via `sdltiles.cpp:1611`.

### 2.9 Terminal build

- Non-tiles build uses `ncurses_def.cpp` (ncurses library) directly. Text is terminal glyphs with `init_pair`-style colours (`ncurses_def.cpp:252-268`).

## 3. Accessibility and screen-reader search results

Commands run, with results:

1. `rg -i 'tts|speech|narrat|screen.?reader|accessib' src/` returned many hits. All were unrelated:
   - `speech` is the game's Speech skill and in-game speech sounds (`activity_speed.cpp:304`, `character_effects.cpp:139`, `speech.cpp`, `iuse.cpp:5517`, `npc.cpp:1796`).
   - `accessible` / `inaccessible` refers to reachability in pathfinding and item access (`pathfinding.h:182-193`, `map.cpp`, `crafting.cpp:557`).
2. Stricter grep: `rg -in '\btts\b|text.?to.?speech|speech.?synth|screen.?reader|narrat|accessibil|colou?r.?blind|high.?contrast|nvda|jaws|a11y' src/` returned **15 hits**, all comments or unrelated names:
   - `messages.cpp:887` and `:920`: "messages will not be missed by screen readers" (wredrawln keeps message lines in the output so screen readers pick them up).
   - `ui_manager.h:41` and `:157`: `record_cursor` / `set_cursor` documented as "for screen readers and IME preview". Line 157 says screen readers "do not work with the current tiles implementation".
   - `string_input_popup.cpp:326`: cursor placed "for screen readers and IME preview".
   - `game.cpp:4940` and `:5132`: the cursor is placed over the player, "This allows screen readers to describe the area around the player". Line 4940 is inside the map draw path.
   - `game.cpp:7349`: "we don't have screen readers in game", paired with the message "You can not see a computer screen!" for blind characters.
   - `game.cpp:11453`: cursor moved to the selected item "(for screen readers)".
   - `overmap/overmap_ui.cpp:1311`: "Set cursor for screen readers".
   - `status_report.h:15`: `status_report` is described as "meant for the message log and screen readers" (plain-text summary of surroundings).
   - Unrelated uses: `item_contents.h:102` ("accessibility" in item expansion), `vehicle/vehicle_selector.h:38` (line-of-sight), `vehicle/vpart_range.h:99` and `:101` (C++ constructor access, NOLINT).
3. `rg -il imgui` (excluding docs) returned **0 files**. ImGui is not used.
4. There is **no TTS, screen-reader API, narration, colour-blind mode or high-contrast mode** in `src/`, `data/json` or `data/raw` by these searches. The only accessibility-adjacent option found is `MODERN_MENUS` (`options.cpp:2546`), which is a border/title colour theme, not an accessibility feature.
5. Related research documents already in this folder: `g_accessibility_guidelines.md` and `h_screen_reader_tts.md` (not reviewed in this audit).

What the code does provide: the **hardware cursor is placed** on the focused element (`ui_adaptor::set_cursor`, `ui_manager.h:157` onward), so a platform screen reader using the terminal's cursor can follow the focus on the curses build. The comment at `ui_manager.h:157` says this does not work on tiles.

## 4. Not found / uncertain

- **Not found:** TTS, speech output, narration, a screen-reader API, colour-blind palette, high-contrast mode, ImGui. The stricter grep found only comments.
- **Uncertain: mouse semantics.** Click activates the highlighted row, with no hit-test, is read from `ui.cpp` (no coordinate code) and the key bindings. It has not been run.
- **Uncertain: modal vs full-screen.** Inferred from window-sizing references per file. Not checked at runtime.
- **Keys not verified:** options (`open_options`, keybindings.json:2592), autopickup (`open_autopickup`, :2598), autonotes, distraction manager (`open_distraction_manager`, :2616), colour manager (`open_color`, :2622), vehicle examine key, armour layers, salvage, gun-mod menu entry. Each has an action ID in `keybindings.json` but the key was not read.
- **Not traced:** whether `main.cpp` calls `opening_screen()` at startup; the implementation of `disp_morale` (`morale.cpp`); `armor_layers.cpp` (1000 lines) and `pixel_minimap.cpp` (708 lines) were counted but not read.
- **Mixed-use files:** `game.cpp` (17,323), `item.cpp` (12,007), `iexamine.cpp` (8,865), `npctalk.cpp` (3,940), `ranged.cpp` (4,921, targeting uses `COORDINATE` at `ranged.cpp:3284`). Their UI parts were not separated into LOC. The totals above exclude `game.cpp`, `item.cpp` and `iexamine.cpp`, but include `npctalk.cpp`.
- **Gun mod and reload menu entry points:** the reload path is verified (`avatar_action.cpp:1528` and `:1636`). The gun-mod picker is verified as an inventory preset (`game_inventory.cpp:930`), but the action that opens it was not traced.
- **Primitive counts** are from `rg -c` on each file and undercount helper functions in other files that reuse these primitives.
