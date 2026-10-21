# CDDA / BN UI pain points (sourced)

Research date: 2026-10-21. Scope: GitHub issues (CleverRaven/Cataclysm-DDA, cataclysmbn/Cataclysm-BN), Steam reviews (app 2330750), CDDA Discourse.

## Method and limits

- **Reddit was not reachable.** WebSearch rejects reddit.com as a domain, and `curl` to reddit.com returns a "Blocked" page. r/cataclysmdda, r/cataclysmbn and r/roguelikes are therefore not mined. Nothing below is from Reddit.
- **YouTube comments were not reachable.** One WebSearch returned only Steam/App Store text, which I did not count.
- **Steam discussion page** (steamcommunity.com/app/2330750/discussions/...) is age-gated when fetched with curl. Its content is not verified here.
- **Steam reviews** were read via the public `store.steampowered.com/appreviews/2330750` JSON API (100 most recent, plus 51 most recent negative). Review URLs below are constructed from the author steamid in that API response, in the standard `steamcommunity.com/profiles/<steamid>/recommended/2330750/` form. I did not open each URL.
- **Discourse** threads were read via the `.json` endpoint, which returns the raw cooked HTML; text below is the stripped version, copied verbatim.
- **GitHub** issue bodies were read via `gh api`. Many are developer-filed "UI audit" issues rather than player complaints; these are marked as such.
- Counts are "distinct sources I actually read in full or in the API/JSON body". Title-only hits are not counted as sources.
- Quotes are verbatim. Where I removed Markdown bold markers, it is noted.

## 1. Pain points ranked by frequency

| Rank | Pain point | Distinct sources read | Platforms |
|---|---|---|---|
| 1 | Menus overloaded / too many options / nested menus | 16 | Steam, GitHub, Discourse |
| 2 | Keybindings inconsistent, hard to learn, rebinding breaks things | 15 | Steam, GitHub |
| 3 | Text overflow, truncation, layout not fitting (small screens, font size, long names) | 14 | GitHub, Steam |
| 4 | ImGui (and curses-ImGui) rendering / interaction defects | 15 | GitHub, Discourse |
| 5 | Pocket / nested-container mental model confuses players | 10 | Steam, Discourse, GitHub |
| 6 | Advanced inventory: filter/sort/search gaps (filters ignore container contents, no sort) | 5 | GitHub |
| 7 | Mouse support missing or inconsistent | 6 | GitHub, Steam |
| 8 | Discoverability of actions and hidden features | 5 | GitHub, Steam |
| 9 | Menu selection / tab state resets on reopen or scroll | 3 | GitHub |
| 10 | Slow menus with many items | 2 | GitHub |

Notes on the counts:
- Rank 1 includes 2 Steam reviews that are positive but still complain about inventory ("the inventory is so bad").
- Rank 4 includes Discourse thread 29530 and GitHub issues filed by players and developers.
- Rank 6 is small in count because I did not search every AIM issue. It is likely higher than shown.
- Rank 10 is low because I searched only construction/crafting/recipe performance.

## 2. Quotes per pain point

### Pain point 1: Menus overloaded, too many options, nested menus

1. Steam, negative, 2026-07-20, author steamid 76561197981255486 (https://steamcommunity.com/profiles/76561197981255486/recommended/2330750/):
   > "The menus in this game are excessively complicated. There are way too many options, and I don’t want to read a novel just to open and drink a stupid bottle of water."
   > "Instead of coding proper situational menus, they just throw the entire list at you every time."

2. Steam, negative, 2025-02-08, steamid 76561198030828931 (https://steamcommunity.com/profiles/76561198030828931/recommended/2330750/):
   > "Navigating a pocket-based inventory system with so much menu jank is hard enough as is"
   > "Rather than work on that, rather than try to make the menus navigable, readable, or just colour-coding them better, the devs added in a new system."

3. Steam, positive, 2025-05-21, steamid 76561198170937002 (https://steamcommunity.com/profiles/76561198170937002/recommended/2330750/):
   > "This game is fire but good lord the inventory is so bad"

4. GitHub BN, #3602 by @olanti-p, 2023-11-09 (https://github.com/cataclysmbn/Cataclysm-BN/issues/3602):
   > "Many UIs are clunky to use, and require the player to “get used to” them."

5. GitHub CDDA, #70909 "Construction UI needs an overhaul badly", @NetSysFire, 2024-01-14 (https://github.com/CleverRaven/Cataclysm-DDA/issues/70909):
   > "The construction UI has not been updated since ages, yet the crafting UI has gotten a lot of features which the construction UI would benefit from."

Additional sources counted for rank 1 (quotes in pain point 5 or 2): Steam 189355397, Steam 195322870, BN #5442, BN #1198, CDDA #30019, CDDA #28611, Discourse 12592/23533/23529/24456, Discourse QoL thread 23753 (Gadgetpatch, 2020-05-25, "There are so many inventory-like interfaces" in post 1 of that thread).

### Pain point 2: Keybindings inconsistent, hard to learn, rebinding breaks things

1. Steam, negative, 2025-01-25, steamid 76561198040659710 (https://steamcommunity.com/profiles/76561198040659710/recommended/2330750/):
   > "Its a great game that is unfortunately ruined by an ancient and awkward control scheme that is in desperate need of an overhaul."

2. Steam, negative, 2023-12-20, steamid 76561198011358609 (https://steamcommunity.com/profiles/76561198011358609/recommended/2330750/):
   > "the lack of consistent keys (ie press B to back out of menu or 2 or whatever) is very frustrating."

3. Steam, positive, 2024-12-29, steamid 76561197992764003 (https://steamcommunity.com/profiles/76561197992764003/recommended/2330750/):
   > "This is the kind of game you watch tutorial video series on and still find it extremely hard."

4. GitHub CDDA, #45927 "Changing movement keybinds breaks menu navigation", @JBloodthorn, 2020-12-09 (https://github.com/CleverRaven/Cataclysm-DDA/issues/45927):
   > "This renders the menu unusable, as in teh carat will not move when the newly bound key is pressed."

5. GitHub CDDA, #86331 "new crafting menu bindings for batch size adjustment conflict with existing bindings", @esotericist, 2026-04-07 (https://github.com/CleverRaven/Cataclysm-DDA/issues/86331):
   > "#86193 added batch size increase and batch size decrease bindings, setting their defaults to to `+` and `-`, but prioritize and deprioritize already had those as their default keys"

Additional rank-2 sources: Steam 225815036 ("this game has a pretty big learning curve when you're first learning the controls on your keyboard"), Steam 206650287 ("Takes a lot of characters to learn the keybinds and game mechanics."), Steam 189355397 (player spent 30 minutes "figuring out how to navigate my inventory"), Steam 208331993 ("Learning the controls as I go"), CDDA #74637 (filter hint shows wrong key), CDDA #88538 (keybinding ineffective in menu after reassigning direction keys), CDDA #89172 (default `+` for missing components does not work; 2026-10-19), CDDA #88409 (`+` conflict, 2026-08-24), CDDA #58358 (keybindings menu UI audit, dev-filed), BN #5442, BN #3602.

### Pain point 3: Text overflow, truncation, layout not fitting

1. GitHub CDDA, #84796 "The text does not fit in the window", @Snobi4, 2026-01-20 (https://github.com/CleverRaven/Cataclysm-DDA/issues/84796):
   > "Because I learned spells with long names, the window now looks like this."

2. GitHub CDDA, #84564 "imgui issue - quest descriptions do not have word wrap", @anoobindisguise, 2026-01-09 (https://github.com/CleverRaven/Cataclysm-DDA/issues/84564):
   > "I cannot read parts of quest descriptions, because they trail off the right edge of the quest menu."

3. GitHub CDDA, #88376 "Long dialogue options break the newest Imgui dialogue window", @Parhynr, 2026-08-19 (https://github.com/CleverRaven/Cataclysm-DDA/issues/88376):
   > "In the newest Imgui dialogue window, long dialogue options don't wrap around."

4. GitHub CDDA, #77116 "Spell menu gets squashed into ueslessness with long spell names", @Keltas, 2024-10-18 (https://github.com/CleverRaven/Cataclysm-DDA/issues/77116):
   > "It stretches the name box so far that the information about the spell on the right is illegible."

Additional rank-3 sources: CDDA #53803 (@AxemaVales, 2021-12-27, "Long texts are cut-off and has no other ways to view the rest."), CDDA #88385 ("two consecutive newlines (`\n\n`) cause the rest of the text to disappear"), CDDA #85064 (@vsisko, 2026-02-01, "The menu is moved right, and it hides the item description panel."), CDDA #84923 (@vsisko, 2026-01-26, "The new \"List nearby items\" panel is moved out of the screen with my settings."), CDDA #86931 (mending info cut off), Steam 171651188 (2024-08-08, "you need to adjust graphics to fit your screen so you can see the font"), BN #10427 (@bullkang, 2026-09-29, "Scenario and Profession don't fit on the screen"), BN #10675 (@midgardormr, 2026-10-21, "making the whole menu practically unreadable."), CDDA #58230 (dev-filed profession tab audit).

### Pain point 4: ImGui and curses-ImGui rendering / interaction defects

1. GitHub CDDA, #82161 "Curses UI problems", @SandwichPie, 2025-07-31 (https://github.com/CleverRaven/Cataclysm-DDA/issues/82161):
   > "imGUI still has several major problems in the curses build that makes menus difficult to use."

2. GitHub CDDA, #73120 "Imgui rendered fonts do not look as crisp as in the other menus with the software renderer", @NetSysFire, 2024-04-19 (https://github.com/CleverRaven/Cataclysm-DDA/issues/73120):
   > "Brain immediately complains that the fonts look.. blurry? They definitely look off and not as crisp as the menu just before."

3. GitHub CDDA, #73728 "Font rendering is blurry in IMGUI under WASM", @nornagon, 2024-05-12 (https://github.com/CleverRaven/Cataclysm-DDA/issues/73728):
   > "On the WASM build, text in IMGUI UIs is blurry, almost like it's off by half a pixel or something."

4. GitHub CDDA, #75288 "clicking outside of an ImGui window should cancel it (or do nothing), rather than selecting something", @db48x, 2024-07-28 (https://github.com/CleverRaven/Cataclysm-DDA/issues/75288):
   > "Currently, the `input_context` turns all mouse clicks into the SELECT action, no matter where the user actually clicked."

5. GitHub CDDA, #75744 "Blank ImGui menus/Menus not updating correctly when opened", @Kozakow, 2024-08-16 (https://github.com/CleverRaven/Cataclysm-DDA/issues/75744):
   > "Seemingly for no reason some menus can open with text being rendered far enough up from the \"window' that it isn't visible at all"

6. GitHub CDDA, #87475 "Crash on alt+F4 in any ImGui window", @Brambor, 2026-06-06 (https://github.com/CleverRaven/Cataclysm-DDA/issues/87475):
   > "When any ImGui window is displayed, the game crashes on Alt+F4."

7. GitHub CDDA, #88651 "ImGui main menu mouse hit-testing is offset on high-DPI (200%) Windows displays", @YellowBee123, 2026-09-13 (https://github.com/CleverRaven/Cataclysm-DDA/issues/88651) (bold markers removed):
   > "On Windows with display scaling set to 200%, mouse clicks on the ImGui-rendered main menu land on the wrong entry."

Additional rank-4 sources: CDDA #77297 ("Incorrect text position in menu [ImGUI]", @ghost, 2024-10-24), CDDA #88254 (@Love-and-Tea, 2026-08-07, "solid black rectangles inside ImGui windows" for CJK glyphs, outdated bundled unifont), CDDA #87781 (buttons do not respond to mouse, 2026-06-24), CDDA #86378 (mouse "still active in some menus", 2026-04-09), CDDA #88385 (dialogue text truncated, 2026-08-20), Discourse 29530 (see upstream section).

### Pain point 5: Pocket / nested-container mental model

1. Discourse, topic 12592 "Inventory Menu Confusion", @PoeSalesman, 2016-09-27 (https://discourse.cataclysmdda.org/t/inventory-menu-confusion/12592/1):
   > "it’s all in one, long, painful line on the left side."

2. Discourse, topic 24456 "An Item in a container is Separate from the container in the inventory", @Havis, 2020-08-15 (https://discourse.cataclysmdda.org/t/an-item-in-a-container-is-separate-from-the-container-in-the-inventory/24456/1):
   > "I haven’t played this game in a long time, and I’m sort of confused about the way containers work in my inventory."

3. Discourse, topic 23533 "Inventory Screen messed up?", @Bjo_Hart, 2020-05-04 (https://discourse.cataclysmdda.org/t/inventory-screen-messed-up/23533/1):
   > "A little bit confusing this new typ of inventory."

4. Discourse, topic 24456, reply by @KorGgenT, 2020-08-15 (https://discourse.cataclysmdda.org/t/an-item-in-a-container-is-separate-from-the-container-in-the-inventory/24456/2):
   > "a ton of people have been confused by this but i have no idea how."

5. Steam, negative, 2025-02-08, steamid 76561198030828931 (see pain point 1 link):
   > "It started with the pockets, which add a level of immersion the game isn't built to deal with."

Additional rank-5 sources: Discourse 23529 (@Dany_Raven, 2020-05-18: "there is a thing that grind my gears in the inventory"), CDDA #87232 (@Termineitor244, 2026-05-25: "the keybindings are left floating in front of the item selection list"), BN #2308 (@Kaylakaze, 2023-01-26: "Right now, Unloading stuff can be a pain."), BN #1013 (@Ixella, 2021-10-08), BN #3525 (@Daenyth, 2023-10-30: "This gives a really awkward structure where how to equip an NPC is in two completely separate menus").

### Pain point 6: Advanced inventory filter / sort / search gaps

1. GitHub BN, #7280 "Allow item filters to also work on items inside containers", @jeremy7986, 2025-09-24 (https://github.com/cataclysmbn/Cataclysm-BN/issues/7280):
   > "However, if the item is placed inside a container (for example, apple juice inside a bottle), it cannot be found."

2. GitHub BN, #3509 "[QoL feature] Allow filtering categories in 'R'ead book menu", @Daenyth, 2023-10-27 (https://github.com/cataclysmbn/Cataclysm-BN/issues/3509):
   > "I think multiple `/` menus could have some amount of advanced filtering keys, but don't."

3. GitHub BN, #10161 "Sort Read menu", @narkiel, 2026-08-31 (https://github.com/cataclysmbn/Cataclysm-BN/issues/10161):
   > "This must have broken at some point."

4. GitHub BN, #1198 "Advanced Inventory default state convenience improvement", @vorpal-void, 2021-12-24 (https://github.com/cataclysmbn/Cataclysm-BN/issues/1198):
   > "This situation relates on player ability to overcome unknowns of the UI and actually go read the manual about how to work with that thing."

5. GitHub CDDA, #26311 "Improving discoverability for crafting search options", @jbytheway, 2018-10-18 (https://github.com/CleverRaven/Cataclysm-DDA/issues/26311):
   > "Everyone I've seen use these options (myself and two or three YouTubers) gets confused."

### Pain point 7: Mouse support missing or inconsistent

1. GitHub BN, #3602 "☂️ Gameplay: Issues with UI", @olanti-p, 2023-11-09 (https://github.com/cataclysmbn/Cataclysm-BN/issues/3602):
   > "There is no full mouse support. Partial mouse support is present, but for many cases it’s impossible to interact with the UI using mouse."

2. GitHub BN, #5442 "Add DDA mouse support", @thedyze, 2024-09-27 (https://github.com/cataclysmbn/Cataclysm-BN/issues/5442):
   > "The complexity of the giant amount of screens each with its own keybinds is another barrier, that is not fun to master."

3. GitHub CDDA, #87781 (2026-06-24, @DropletsUponFiveLargeDroplets) (https://github.com/CleverRaven/Cataclysm-DDA/issues/87781):
   > "clicking with m1 just makes the buttons flash like they've been pressed and do nothing"

4. GitHub CDDA, #86378 (2026-04-09, @adamkad1) (https://github.com/CleverRaven/Cataclysm-DDA/issues/86378):
   > "Notice that mouse is still active in some menus"

### Pain point 8: Discoverability of actions and hidden features

1. GitHub CDDA, #1368 "Add action menu for action discoverability", @HeatherSoron, 2013-06-04 (https://github.com/CleverRaven/Cataclysm-DDA/issues/1368):
   > "so that newbies don't have to remember that '(' is disassemble (for example)."

2. GitHub BN, #3602 (see pain point 7):
   > "Many UIs have hidden features not communicated in any way."

3. GitHub BN, #9805 "Remove the “Examine” button's function to obtain items", @cosing, 2026-07-05 (https://github.com/cataclysmbn/Cataclysm-BN/issues/9805):
   > "This multi-purpose nature frequently leads to a frustrating \"double-menu\" (nested menu) issue."

4. GitHub CDDA, #28611 "too many key presses: reading a book", @spacenookie, 2019-03-10 (https://github.com/CleverRaven/Cataclysm-DDA/issues/28611):
   > "if I read a book, but don't want to read until I gain a skill level, I have to press either 4 keys for each time I read the book"

5. Steam, positive, 2024-12-29 (see pain point 2): tutorials needed to learn hotkeys.

### Pain point 9: Selection and tab state resets

1. GitHub BN, #8299 "Construction Menu Cycles Counterclockwise One Tab Each Opening", @sato2077, 2026-03-10 (https://github.com/cataclysmbn/Cataclysm-BN/issues/8299):
   > "opening the construction menu will always move one tab counterclockwise from where it was before."

2. GitHub BN, #8154 "Construction menu shifting tabs and not remaining focused on selected item", @Meridian117, 2026-02-15 (https://github.com/cataclysmbn/Cataclysm-BN/issues/8154):
   > "This also prevents repeated use of the selected action due to menu changing and not keeping selection on previous item."

3. GitHub CDDA, #75744 (see pain point 4): "Pressing any key causes for the list to update and render correctly."

### Pain point 10: Slow menus with many items

1. GitHub CDDA, #88904 "Extreme slowdown in Contruction Menu with a lot of items around", @thaelina, 2026-10-04 (https://github.com/CleverRaven/Cataclysm-DDA/issues/88904):
   > "Opening the construction menu in an area with a lot of items takes a while to open, seemingly longer the more items there are"

2. GitHub CDDA, #88435 "Extremely slow recipe list loading and other performance issue due to the large number of items", @Zerdocriil, 2026-08-27 (https://github.com/CleverRaven/Cataclysm-DDA/issues/88435):
   > "More that 2+ minutes"

## 3. Upstream CDDA ImGui migration (2023-2026)

Praise (found, few items):
- GitHub CDDA PR #73134 "Migrate mission UI to imGUI" (opened 2024-04-19 by @RenechCDDA). Reviewer @db48x, 2024-05-03 (https://github.com/CleverRaven/Cataclysm-DDA/pull/73134):
  > "Overall the UI works really well, but it is odd that the mission list is not as tall as the whole window."
- Steam, positive, 2025-06-20 (steamid 76561198153533790, https://steamcommunity.com/profiles/76561198153533790/recommended/2330750/). This review does not say which UI is ImGui, so it is not attributed to the migration:
  > "the advanced inventory management system which is actually incredibly intuitive to use and not at all as complicated as people make it out to be"

Complaints about font scaling and rendering:
- #73120 (@NetSysFire, 2024-04-19): "Brain immediately complains that the fonts look.. blurry?" (quoted above)
- #73728 (@nornagon, 2024-05-12): "text in IMGUI UIs is blurry, almost like it's off by half a pixel or something." (quoted above)
- #88254 (@Love-and-Tea, 2026-08-07): "solid black rectangles inside ImGui windows" for CJK glyphs, because the bundled unifont is outdated.

Complaints about "dev tool" look and about the migration itself:
- Discourse topic 29530 "Changing the menu interface", @AnimeAss, 2024-11-22 (https://discourse.cataclysmdda.org/t/changing-the-menu-interface/29530/1):
  > "Uh… I just started playing experimental version of cdda and I don’t like new “graphical” ui of some windows."
  > "Like, escape menu is now not ascii-like and I can’t find setting to disable it."
- Reply by @Profugo_Barbatus, 2024-11-25 (https://discourse.cataclysmdda.org/t/changing-the-menu-interface/29530/2):
  > "Nope, the imgui migration is not optional."

Complaints about mouse and input:
- #75288 (@db48x, 2024-07-28): clicks outside an ImGui window select an item instead of closing it (quoted above).
- #86378 (2026-04-09): mouse still active in some menus after disabling (quoted above).
- #88651 (@YellowBee123, 2026-09-13): click offset at 200% DPI (quoted above).
- #87475 (@Brambor, 2026-06-06): Alt+F4 crash in any ImGui window (quoted above).

Complaints about text and layout:
- #84564, #88376, #88385 (quoted above or in pain point 3).
- #77297 (@ghost, 2024-10-24): "Incorrect text position in menu [ImGUI]".

Performance:
- Not found with a clear ImGui attribution. #88904 and #88435 are about construction and crafting menus; I did not confirm whether they are ImGui-rendered, so I did not attribute them to the migration.

Dev-side context:
- Discourse topic 24091 "Game Features for Player Created Content" (@mr_sep, 2020-08-17), from a search blurb only; the post was not read in full. Topic URL: https://discourse.cataclysmdda.org/t/24091 (slug omitted, so this form is unverified). Quote: "I’ve poked around lately, talked to the other devs, and found a GUI Creator called Dear ImGUI."
- Discourse 29530 reply by @Profugo_Barbatus (2024-11-25): "The ‘finished’ (as much as anything is finished in an open source project like this) version of the new interfaces may restore some of the visual st..." (truncated in blurb; the full post was not read).
- GitHub CDDA PR list shows an active ImGui push through 2026-10 (e.g. #88235 "Migrate dialogue window to imgui", #87137 "Upgrade vendored ImGui to 1.92.8", #88665 "Imguify Help Menu"). These are titles only, not comment threads.

"Looks like a dev tool" wording: **not found verbatim** in any source I read.

## 4. Could not find evidence for

- **Reddit (r/cataclysmdda, r/cataclysmbn, r/roguelikes):** blocked by WebSearch and curl. Zero Reddit quotes.
- **YouTube comment threads:** not reachable. A WebSearch returned only Steam/App Store snippets, which I did not count.
- **Steam discussions page** (app 2330750, discussion 6197556602757688644): age-gated when fetched. Not used.
- **"Why I quit / bounced off" threads:** one WebSearch returned blog/forum mentions of quitting for non-UI reasons (lights on turrets, save deletion). No UI-specific quit thread found.
- **"Looks like a dev tool" / font scaling complaints from players about ImGui:** not found. Font complaints were dev/bug reports, not player reviews.
- **BN Discourse / BN Steam reviews:** not searched in depth. BN Steam page not checked.
- **BN "too many keys" and BN "discoverability" issue searches:** zero hits.
- **CDDA "ImGui slow/performance" issue searches:** zero hits.
- **CDDA "not intuitive" / "confusing menu" GitHub searches:** zero hits.
- **Steam reviews beyond the most recent 100 (and 51 negative):** not paged further.
- **CDDA character creation as a player complaint:** only developer-filed "UI audit" issues (#58223, #58226, #58230, #58232, #58260, #58265, #58290, #58302, 2022-06). No player quotes.
- **CDDA construction menu player complaints beyond #70909:** only one substantive player-filed item found.
- **App Store reviews of the iOS port:** WebSearch snippet only ("the UI is a problem", "zoom to work"), not fetched or quoted here.
- **WebFetch of the Steam discussion** and **WebFetch of the Discourse QoL thread** were attempted; the Discourse thread was read via its `.json` endpoint instead, which contains 5 posts (full text verified).

## Verification notes

- The Discourse QoL thread (topic 23753, 2020-05-25) was read via its JSON. Its five posts were verified to contain the text of the WebFetch-summarised items I checked (e.g. "use the same keys for identical functions", "crammed off to one side", "I get the sense that the Dev Squad"). Those items are not quoted in the body above; I used the Discourse 2020 thread only for its substance (see pain point 1, rank 1 notes). The thread is at https://discourse.cataclysmdda.org/t/general-quality-of-life-ui-improvement-discussion/23753 (posts 1-5).
- GitHub issue/PR URLs, Steam review IDs and Discourse topic IDs above all appeared in API or JSON responses read in this session. Two kinds of URL are constructed and not confirmed by a response: the Steam per-review profile links (built from the API steamid) and the Discourse slugs (e.g. `/t/inventory-menu-confusion/12592/1`). The numeric topic IDs are confirmed; the slugs are not. Treat the Discourse links as "topic ID confirmed, slug unverified".
