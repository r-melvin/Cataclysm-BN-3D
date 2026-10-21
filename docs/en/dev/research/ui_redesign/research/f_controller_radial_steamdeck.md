# Controller-first UI, radial menus and Steam Deck constraints

Research pass, 2026-10-21. Sourcing key:

- **VERIFIED**: quote or fact taken from a page fetched in this session. URL given.
- **SNIPPET**: only seen in search-result text, not fetched. Treat as unverified.
- **UNVERIFIED**: memory, or a claim whose source could not be opened (403 or not found).

Where a quote came back through the fetch tool as a summary, it is marked "(fetch-extracted)" and the exact wording should be checked before reuse.

---

## 1. Radial and weapon wheel design

### 1.1 Slot count

No source found that directly compares 8-slot and 12-slot wheels for usability. The evidence that exists:

- **VERIFIED** Wikipedia, "Pie menu" (https://en.wikipedia.org/wiki/Pie_menu): "Pie menus work well with keyboard acceleration, particularly four and eight item menus". The same page says the slices are "large in size and near the pointer for fast interaction (see Fitts's law)".
- **SNIPPET** A CHI 2009 marking-menu submission (found via a search summary, not opened) treats eight items per menu as a practical ceiling for high performance, and calls it too few for Word or Photoshop-scale command sets. Primary paper not checked.
- **SNIPPET** A 1988 comparison is often cited as finding radial menus faster than linear lists for eight items. Only seen through a secondary blog. Not verified.
- **SNIPPET** The Long Dark wiki (https://thelongdark.fandom.com/wiki/Radial_Menu) says its wheels "have room for only 8 items at a time", and surplus items are left off the wheel. Real-game example of the cap biting, with a manual "Add to Radial" workaround.
- **SNIPPET** Dying Light 2 guide (powerpyx) says up to four weapons go on the weapon wheel; holding UP and LEFT open consumables and equipment wheels. Not fetched.
- **SNIPPET** GTA V forum and GTA BOOM criticism: the wheel fills with every weapon the player has ever picked up. See 1.4.
- **UNVERIFIED** "Long-distance" 12-slot wheels as a norm are not documented in anything I opened. Do not assume 12 is usable.

Working conclusion (inference, not sourced): 4 and 8 are the counts the literature and shipped games converge on. Above 8 the usual answer is paging, categories, or nesting, not more slices.

### 1.2 Selection by angle, not hover

- **VERIFIED** Wikipedia, "Pie menu": "selection depends on direction instead of distance". Angle selection is the standard for controller pie menus, because the stick gives direction directly.
- **SNIPPET** An Unreal Engine radial plugin on Fab (https://www.fab.com/listings/48347aae-042e-4d8d-908f-268c897537f8) advertises separate mouse and gamepad deadzones, stick selection through the right stick, and a GapDeg parameter for segment spacing. Vendor page, not evaluated.
- **SNIPPET** The same Fab listing describes a "flick and let go" confirm gesture for gamepad. Only in a search summary of a different radial plugin listing; treat as unverified.

### 1.3 Examples

**Baldur's Gate 3 (Larian)**

- **VERIFIED** PlayStation Blog, Emily Gera, Product Manager, Larian Studios, 5 Sep 2023 (https://blog.playstation.com/2023/09/05/how-baldurs-gate-3-adapts-its-expansive-rpg-gameplay-for-your-dualsense-controller/): "turning our original UI into radial menus which felt more intuitive to use with a controller".
- **VERIFIED** Same post: "A light pull of the trigger will let you peek at your radial menus for a moment".
- **VERIFIED** Same post: "Your most essential tools can be accessed at the pull of a trigger button".
- **SNIPPET** Steam community BG3 thread (https://steamcommunity.com/app/1086940/discussions/0/6045572169621274305/?ctp=3): a player reports that on controller, holding Y/Triangle cancels concentration; the Steam Deck hover prompt shows "hold (Y)". Fetch of the page returned no post text, so the quote is UNVERIFIED.
- **SNIPPET** Shacknews BG3 controls guide: Right Trigger opens the Shortcut Menu. Not fetched.
- **SNIPPET** Steam BG3 thread: a controller player reports a radial-menu stutter. Anecdotal, 2023.

**Factorio (Switch port)**

- **VERIFIED** Factorio wiki, "Quick panel" (https://wiki.factorio.com/Quick_panel): "The Quick panel is a radial menu in the Nintendo Switch version of the game." It replaces the PC quickbar, shortcut bar and minimap buttons.
- **VERIFIED** Same page: "It is open while holding L (left bumper)."
- **VERIFIED** Same page: "The different tabs of the quick panel can be selected by pressing Right and Left on the D-pad."
- **VERIFIED** Same page: "Individual buttons in the quick panel are selected by moving the Left Stick in the direction of the button".
- **VERIFIED** Same page: "Note that the quick panel is also available on the PC version of the game while using a controller."
- **SNIPPET** Factorio wiki also describes a free cursor / auto cursor toggle on the right stick for Switch. The fetched page did not include that text, so UNVERIFIED.

This is the cleanest example of paging a radial: the wheel holds one set of items, and D-pad left and right switch between sets.

**Dying Light 2**

- **SNIPPET** Powerpyx guide: up to four weapons are assigned to the wheel, then the wheel is opened by holding the weapon key (Hold RIGHT on controller). Not fetched.
- **SNIPPET** Shacknews controls page: Cycle Weapons on D-pad Right. Not fetched.

**Deep Rock Galactic**

- **UNVERIFIED** No official source on the weapon wheel was opened. Search summaries mention a Steam Input profile that remaps the D-pad to a radial to free the resupply input. Not verified.

**GTA V / GTA 6**

- **VERIFIED** GTA BOOM, Aron Gerencser, 31 Jan 2016 (https://www.gtaboom.com/gta-online-desperately-needs-better-weapon-management-eccb): "If you want to select a weapon within a category you'll need to cycle through all weapons in that category."
- **VERIFIED** Same article proposes "restrict players to carrying only one weapon from each category while out and about", with a weapons locker in apartments.
- **VERIFIED** Dexerto, published Oct 19, 2026 (https://www.dexerto.com/gta/gta-6-devs-tried-a-million-weapon-wheel-alternatives-as-struggles-with-rdr2-realism-led-some-to-quit-3420111/), fetch-extracted: "developers worried that keeping GTA 5's traditional weapon wheel would leave players staring at empty slots". Quoting Bloomberg's Jason Schreier: "And so they spent months, if not years, trying a million different things that would replace the weapon wheel". Confirm the exact wording before reuse.
- **SNIPPET** Kotaku and Notebookcheck report GTA 6 preview backlash to the wheel's blurred background and large rectangular boxes. Not fetched.

**Diablo 4 (controller)**

- **SNIPPET** Forum and Icy Veins complaints: four skills plus left and right click is the limit; the basic skill shares a button with Interact, so it blocks looting while on cooldown; no manual inventory sorting on controller. Dates unconfirmed. Not a radial example, but relevant to slot limits.

**Divinity: Original Sin 2**

- **SNIPPET** RPGamer E3 2018 impression (https://rpgamer.com/2018/06/divinity-original-sin-ii-definitive-edition-e3-impression-2/) described trigger-driven radial menus as the gateway to inventory and stats. Not fetched.
- **SNIPPET** Steam forum: "moving items around is a total pain with the controller compared to the drag/drop mechanic". Anecdotal, not dated in the summary.

**Caves of Qud**

- Not a radial example. See section 2.

### 1.4 Praise and complaints (radial)

- Praise: BG3's developer frames radials as the intended controller answer (VERIFIED, PlayStation Blog above).
- Complaint: whole-wheel cycling and unwieldy long lists (VERIFIED, GTA BOOM 2016).
- Complaint: visual style, empty slots (VERIFIED, Dexerto 2026, fetch-extracted).

---

## 2. Controller navigation of inventories and grids

### 2.1 Virtual cursor: why it is disliked

- **VERIFIED** Push Square, Sammy Barker, 18 Feb 2019 (https://www.pushsquare.com/news/2019/02/soapbox_can_we_please_kill_cursors_in_console_games):
  - "I've no problem with what Destiny did: the user interface is probably my favourite part of that series."
  - "What's wrong with using the d-pad to browse your inventory?" (on Assassin's Creed)
  - "it's time to kill the cursor in console games."
  - "Consoles have analogue sticks and d-pads, not a mouse and keyboard."
- **SNIPPET** Summary of a UX Collective article, "Game UX: The cursor that wasn't supposed to be there" (https://uxdesign.cc/game-ux-the-cursor-that-wasnt-supposed-to-be-there-f9369dafc451). The summary says it reviewed 235 console releases and 507 player comments, and that the most common request ("appearing 95 times") was to make the cursor optional, not remove it. **UNVERIFIED**: the fetch returned HTTP 403. Do not quote the numbers until the article is opened.
- **SNIPPET** Warframe forum thread (https://forums.warframe.com/topic/1103360-controller-ux-is-still-significantly-behind-what-it-was-a-year-ago/): a poster says the new console UI still lacks basic D-pad scrolling. Fetch returned 403. UNVERIFIED wording.
- **SNIPPET** Minecraft console feedback (https://feedback.minecraft.net/hc/en-us/community/posts/360009229292/comments/360004784072): players ask for customizable inventory combos and D-pad wraparound. Fetch returned 403. UNVERIFIED wording.
- **SNIPPET** Crate Entertainment forum: a player calls the cursor "a necessary compromise" and asks that D-pad movement jump to the next item. Not fetched.

Pattern the sources support: cursors are tolerated on a few well-designed menus, hated on inventory and shop screens, and the most frequent ask is an option to turn them off (not a demand to remove them). This last point is the SNIPPET source; treat it as unverified.

### 2.2 Focus, bumpers, triggers, stick acceleration

- Bumper-held tab switching: Factorio quick panel (VERIFIED, section 1.3).
- Trigger two-stage: BG3 light pull peeks, full pull opens (VERIFIED, PlayStation Blog).
- Stick acceleration and focus highlight: not sourced in this pass. UNVERIFIED for any specific title.

### 2.3 Complex-port examples

**RimWorld Console Edition**

- **SNIPPET** Reviews from July 2022 say the console UI was rebuilt rather than ported, with a central circle for world interaction and corner UI opened by shoulder buttons, and a stated target of no more than three presses to any screen. Not fetched. Also reported: reviewers found the menus heavy to read, and noted no in-game font size option at launch. UNVERIFIED.
- **SNIPPET** PlayStation Blog says the UI "required a full makeover and rework for the DualShock 4". Not fetched.

**Stardew Valley (console ports)**

- **SNIPPET** A review says UI and inventory management are "much slower and cumbersome when using a controller". Not fetched. Radial inventory on PC is a mod (Star Control), not vanilla. UNVERIFIED that console versions have no radial.

**Terraria**

- **SNIPPET** Steam forum thread: a "Hotbar Scroll time to Radial" slider in Controls can be moved right to turn the radial hotbar off and restore bumper switching. User report, not official docs. UNVERIFIED.

---

## 3. Text input on controller

- **VERIFIED** Steamworks, Steam Deck recommendations (https://partner.steamgames.com/doc/steamdeck/recommendations): "games automatically display an on-screen keyboard when requiring the user to input text".
- **SNIPPET** Steamworks compatibility docs (same family of pages) say a game must either call a Steamworks text-entry API for controller users, or provide its own in-game entry that works with a controller alone. Search summary cited ShowFloatingGamepadTextInput and ShowGamepadTextInput. Not confirmed on a fetched page.
- **SNIPPET** Defold forum post: a Deck review failed a title because the player-name field did not trigger the on-screen keyboard; the title was marked Playable rather than Verified. Not fetched.

---

## 4. Steam Input, glyphs and SDL3

### 4.1 Action sets and layers

- **VERIFIED** Steamworks, Concepts (https://partner.steamgames.com/doc/features/steam_controller/concepts): "An action set is a logical grouping of associated actions." and "Action Set Layers are optional sets of action bindings which can be overlaid upon an existing set of controls."
- **VERIFIED** Steamworks, Action Set Layers (https://partner.steamgames.com/doc/features/steam_controller/action_set_layers):
  - "Activating a new action set will clear all active layers from the old set."
  - "Activating an action set layer that is already active will not change its order in the stack."
  - "Deactivating an action set layer, and then activating it, will set it to the top of the stack."
  - "The last layer applied will override any conflicting information that came before"
  - "Calling ActivateActionSetLayer is cheap and can be safely called repeatedly"
  - "Just because you can re-apply action set layers every frame doesn't mean you should"
  - "Action set layers are intended for narrow use cases"
- **SNIPPET** "Only one action set can be active for any given input device at a given time." Appears in a search summary of the concepts page. The fetched page did not contain that sentence. UNVERIFIED.

### 4.2 In-Game Actions file

- **VERIFIED** Steamworks, IGA file (https://partner.steamgames.com/doc/features/steam_controller/iga_file): "Developers must supply an in-game actions (IGA) file that defines the actions, action sets, and basic structure"
- **VERIFIED** Same page: "By default, the official configuration will be loaded by players"
- **VERIFIED** Same page: "the steam client uses your master IGA file to generate a starting template"

### 4.3 Glyph API

- **VERIFIED** Steamworks, ISteamInput (https://partner.steamgames.com/doc/api/ISteamInput). Functions and their stated purposes:
  - `GetGlyphPNGForActionOrigin(EInputActionOrigin, ESteamInputGlyphSize, uint32)`: "Get a local path to a PNG file for the provided origin's glyph."
  - `GetGlyphSVGForActionOrigin(EInputActionOrigin, uint32)`: "Get a local path to a SVG file for the provided origin's glyph."
  - `GetGlyphForXboxOrigin(EXboxOrigin)`: "Get a local path to art for on-screen glyph for a particular Xbox controller origin."
  - `GetStringForActionOrigin(EInputActionOrigin)`: "Returns a localized string (from Steam's language setting) for the specified origin."
  - `GetActionOriginFromXboxOrigin(InputHandle_t, EXboxOrigin)`: "Get an action origin that you can use in your glyph look up table or passed into"
  - `GetInputTypeForHandle(InputHandle_t)`: "Returns the input type (device model) for the specified controller."
  - `GetDigitalActionOrigins` and `GetAnalogActionOrigins`: "Use this to display the appropriate on-screen prompt for the action."
  - `GetGlyphForActionOrigin_Legacy`: "New code should use GetGlyphPNGForActionOrigin or GetGlyphSVGForActionOrigin instead."
- **VERIFIED** Steamworks compat (below): Valve "strongly recommend[s] using the SteamInput API, which will automatically show the correct glyphs".

Practical note for a non-Steam-API game: the SteamInput glyph calls need the Steamworks SDK. A game built with SDL3 but shipped on Steam still needs Steam Input for correct Deck glyphs unless it resolves glyphs itself.

### 4.4 SDL3

- **VERIFIED** SDL wiki, SDL_GetGamepadButtonLabel (https://wiki.libsdl.org/SDL3/SDL_GetGamepadButtonLabel): signature `SDL_GamepadButtonLabel SDL_GetGamepadButtonLabel(SDL_Gamepad *gamepad, SDL_GamepadButton button);`. Returns "the SDL_GamepadButtonLabel enum corresponding to the button label." Fetch states "This function is available since SDL 3.2.0."
- **Conflict**: a search summary said the official wiki lists 3.0.0. The fetched page says 3.2.0. Check the SDL headers for the version you target.
- **UNVERIFIED** Enum names (`SDL_GAMEPAD_BUTTON_LABEL_*`): not listed on the pages opened. Check `SDL_gamepad.h`.
- **VERIFIED** Companion function `SDL_GetGamepadButtonLabelForType` exists (linked from the wiki page). Its contents were not fetched.

---

## 5. Steam Deck verification checklist

Source for each item is the Steamworks compatibility page (https://partner.steamgames.com/doc/steamdeck/compat) unless noted.

| # | Requirement | Source status |
|---|---|---|
| 1 | Smallest on-screen font character "should never fall below 9 pixels in height at 1280x800." Aim for 12px where possible. | VERIFIED |
| 2 | "Interface text must be easily readable at a distance of 12 inches/30 cm from the screen." | VERIFIED |
| 3 | Support user-adjustable text size; contrast "when possible". | SNIPPET (search summary of the compat page; the fetched Steamworks recommendations page had no text-size guidance) |
| 4 | Run at a supported resolution: "1280x800 (preferred) or 1280x720." | VERIFIED |
| 5 | Glyphs must match the active input: "On-screen glyphs must match the inputs being used, whether it's Deck, Steam Controller, or Xbox glyphs." | VERIFIED |
| 6 | Mouse and keyboard glyphs hidden when not the active input. | VERIFIED |
| 7 | "Interacting with any Deck or Steam Controller inputs using the default configuration must not show non-controller glyphs." | VERIFIED |
| 8 | Default controller configuration must reach all content: "the default controller configuration must provide users with the ability to access all content." | VERIFIED |
| 9 | "Players must not need to adjust any in-game settings in order to enable controller support or this configuration." | VERIFIED |
| 10 | Automatic on-screen keyboard for any text entry: "games automatically display an on-screen keyboard when requiring the user to input text" (Steam Deck recommendations page). | VERIFIED |
| 11 | Use the SteamInput API for glyphs. | VERIFIED (recommendation, not a hard rule on the compat page) |
| 12 | Touchscreen use does not satisfy the controller test. | SNIPPET (GamingOnLinux commentary, not official text) |
| 13 | Gyro/trackpad friendliness is automatic when using the Steam Input API. | VERIFIED (Steam Deck recommendations page) |

Note: the compat page has been revised more than once (a newer version covers Steam Machine). Re-check the live page before submitting.

---

## 6. How complex games did controller ports (summary)

| Game | Approach | Status |
|---|---|---|
| Baldur's Gate 3 | Radial menus with trigger peek; developer quote VERIFIED | Praised in dev post; forum complaints on radial performance (SNIPPET) |
| Divinity: Original Sin 2 | Trigger radials; inventory weak spot | Mixed. Praise in SNIPPET forum; inventory complaints SNIPPET |
| Factorio (Switch) | Bumper-held radial "Quick panel", D-pad tabs, left stick selection; same radial on PC with controller | VERIFIED wiki; PC availability VERIFIED |
| Caves of Qud | Full native controller support finalized Nov 2023; no settings changes needed; Deck Verified | VERIFIED (SteamDeckHQ, GamingOnLinux 2024). One commenter in 2024 said the UI redesign was "impossible to play with a controller even on steam deck." |
| Dwarf Fortress (Steam) | No native controller support; Steam Input workaround; mouse for some menus | VERIFIED (GamingOnLinux, 6 Dec 2022, Liam Squires-Hand) |
| RimWorld Console | UI rebuilt, not ported; readability and learning complaints | SNIPPET reviews; UNVERIFIED |
| Stardew Valley | Console ports; UI slow with controller; radial via PC mods | SNIPPET |
| Terraria | Radial hotbar option, toggle via settings slider | SNIPPET forum |
| CDDA (Steam Deck) | Community Steam Input layout, Vim/WASD scheme | VERIFIED (community post, supersquirrel, 8 May 2024) |
| Dying Light 2 | Weapon wheel with up to four slots; D-pad cycling | SNIPPET |
| GTA V | Long-list wheel, criticised for cycling | VERIFIED (GTA BOOM 2016) |

### 6.1 Caves of Qud (closest analogue for a roguelike)

- **VERIFIED** SteamDeckHQ, Noah Kupetsky, 15 Nov 2023 (https://steamdeckhq.com/news/caves-of-qud-finalizes-steam-deck-support): "we now have finalized Steam Deck support"; "Part of this includes the game getting full native controller support, making using the Deck's gamepad easy and efficient."; "There's also no need to change any settings to make the gamepad work".
- **VERIFIED** GamingOnLinux, 16 May 2024 (https://www.gamingonlinux.com/2024/05/caves-of-qud-gets-a-huge-new-beta-released-with-the-big-ui-redesign/): the beta UI is "Much better for gamepad and Steam Deck players."; "works fully with mouse or gamepad (on top of keyboard)."; the game "has Native Linux support and is Steam Deck Verified."
- **VERIFIED** Same article, reader comment dated 18 May 2024 by EduAAA: the UI makes it "impossible to play with a controller even on steam deck." A complaint about a redesign shipping in beta. Single reader comment, not a developer statement.
- **Lesson (inference, not stated by the developers)**: Caves of Qud is keyboard-first and still got Deck Verified. The sources say the controller works with no settings change and that the redesigned UI works with "mouse or gamepad (on top of keyboard)". None of the fetched sources says how cursor navigation works, so do not read "no cursor emulation" into them.

### 6.2 Dwarf Fortress

- **VERIFIED** GamingOnLinux, Liam Squires-Hand, 6 Dec 2022 (http://www.gamingonlinux.com/2022/12/dwarf-fortress-arrives-on-steam-works-on-linux-desktop-and-steam-deck/): "It doesn't have controller support though"; "you'll need to mess about with Steam Input for a while"; "it does work on Linux desktop and Steam Deck with Proton".
- **VERIFIED** Same article, comment by Theodis: "I had to use the mouse to interact with the various menus"; comment by Philadelphus: "some menus need the mouse".

### 6.3 CDDA community Steam Deck layout

- **VERIFIED** Post by supersquirrel, 8 May 2024 (https://discourse.cataclysmdda.org/t/cataclysm-dda-steam-deck-fully-mapped-easy-to-remember-control-scheme-based-on-vim-wasd/29241): layout uses Vim keys on left stick, WASD ring on left trackpad, number ring on right trackpad; Escape on the menu button, Tab on view, Enter on right trigger, Shift on left trigger. Y single press is "/", double press "?"; A single press opens the movement toggle.
- **VERIFIED** Same post, self-caveat: "a somewhat tenous mapping to remember I concede." and "this is a draft tho."
- **SNIPPET** Lemmy commenters: "This is cool as hell but my brain is not gonna learn it". Not fetched.

---

## 7. Patterns worth adopting

Mechanics for a keyboard-first roguelike like CBN. Each is tied to a source above.

1. **Hold-to-open, release-to-select radial, selection by stick angle.** Pie-menu selection depends on direction, not hover (Wikipedia). Factorio uses hold L to open, stick direction to pick. Adopt the Factorio pattern for a quick-action wheel.
2. **Cap wheel slots at 4 or 8.** Wikipedia names four and eight as the keyboard-acceleration sizes. Overflow goes to pages or categories, not extra slices (The Long Dark cap is an example of what users hit, SNIPPET).
3. **Paging via D-pad inside the wheel.** Factorio switches tabs with D-pad left and right while the bumper holds the wheel open. Gives more than one "slot set" without growing one wheel.
4. **Two-stage trigger.** BG3: a light pull peeks, a full pull opens. Useful for a "peek at status" vs "open full menu" split.
5. **Direction-as-category.** Dying Light 2 holds UP and LEFT for other wheels (SNIPPET). Use four directions, one per category.
6. **Keep the wheel to one category and short.** GTA's long-list complaint (VERIFIED) argues against an unbounded wheel that holds every item.
7. **Do not build a virtual cursor for the grid.** Push Square's argument (VERIFIED): D-pad browsing is the console expectation. If a cursor is needed, make it optional (SNIPPET-level ask, UNVERIFIED count).
8. **Make the default controller binding reach every function.** Steamworks rule, VERIFIED. Players must not need to change settings to get controller support.
9. **Use Steam Input glyphs, not hard-coded labels.** `GetGlyphPNGForActionOrigin` or SVG for Steam builds; `SDL_GetGamepadButtonLabel` for SDL3 builds. The API is the recommended path (VERIFIED).
10. **Use action set layers only for narrow contexts.** Steamworks: "Action set layers are intended for narrow use cases" and "apply and remove them only on specific game state changes" (VERIFIED). Apply on mode change, not every frame.
11. **Text input: always auto-show a keyboard.** VERIFIED Steamworks recommendation. Missing this failed a real Deck review (SNIPPET, Defold).
12. **Text size 9px floor, 12px target at 1280x800.** VERIFIED. Set a user text-size option, because the compat page recommends one.

---

## 8. Steam Deck verification checklist (for CBN)

- [ ] Smallest font glyph height at 1280x800 is at least 9 px; target 12 px (Steamworks compat).
- [ ] Interface text readable at 30 cm (Steamworks compat).
- [ ] User-adjustable text size exists (search summary of Steamworks compat; not on a fetched page, re-check).
- [ ] Runs at 1280x800 (preferred) or 1280x720 (Steamworks compat).
- [ ] Default controller config reaches all functionality (Steamworks compat).
- [ ] No in-game settings change needed to enable controller support (Steamworks compat).
- [ ] Deck or Steam Controller button glyphs shown under default config; no keyboard or mouse glyphs (Steamworks compat).
- [ ] Glyphs match the active input device; use the glyph API (Steamworks compat, ISteamInput).
- [ ] Automatic on-screen keyboard for every text entry (Steamworks recommendations).
- [ ] Action sets and layers used for context switches, not per-frame (Steamworks action_set_layers).
- [ ] Verify the live compat page; it has been revised more than once (Steamworks compat).

---

## 9. Could not verify

- **8 vs 12 slot usability.** No direct study or critique found. The only quantitative anchor is a secondary-blog reference to a 1988 eight-item study and an unopened CHI 2009 paper.
- **UX Collective figures (235 releases, 507 comments, 95 requests).** Article returned 403.
- **Warframe, Minecraft forum complaints.** Both returned 403. Quotes shown are search-summary paraphrases.
- **BG3 concentration-cancel post.** Steam page fetched, but the post text was not in the returned content.
- **Factorio free-cursor toggle.** Mentioned in a search summary; not in the fetched wiki text.
- **Deep Rock Galactic weapon wheel.** No official source found.
- **Diablo 4 controller complaints.** Only search-level sources, dates unconfirmed; sources not fetched.
- **RimWorld Console UI rules (three-press limit, font size).** Search-level; reviews not opened.
- **Dying Light 2 wheel details.** Search-level guide; not opened.
- **Stardew, Terraria console radial status.** Search-level only.
- **Divinity: Original Sin 2 inventory and radial specifics.** Forum and impression snippets only.
- **SDL_GamepadButtonLabel enum names.** Not on any opened page.
- **SDL availability version.** Conflicting: 3.0.0 in a search summary, 3.2.0 on the fetched wiki page.
- **Steamworks "one action set active per device".** Search summary only; not on fetched pages.
- **Steam Deck touchscreen-does-not-satisfy-test rule.** Commentary from GamingOnLinux, not official text.
- **Hold-to-open timings and stick acceleration values** for any title. Not found.
- **Focus highlighting specifics and bumper/trigger paging conventions across titles.** Only Factorio and BG3 covered, from fetched sources.
