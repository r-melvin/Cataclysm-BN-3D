# UI research: Project Zomboid, 7 Days to Die, DayZ

Research notes for the UI redesign. Scope: inventory and loot panels, crafting, status icons (moodles), health, radial/context menus, controller support, Build 42 changes (Zomboid); crafting grid, toolbelt/hotbar, inventory, map, challenges/journal (7DTD); inventory grid, vicinity panel, hands slot, hold-to-interact (DayZ).

Evidence grades used below:
- **[fetched]**: text was returned by WebFetch from the page at the given URL. WebFetch is a small model summarising the page, so wording is the tool's rendering and should be checked against the live page before reuse in anything public.
- **[snippet]**: text appeared in WebSearch result text only. The page itself was not opened.
- **[UNVERIFIED]**: stated from memory or inferred; no source opened.

Reddit (r/projectzomboid, r/7daystodie, r/dayz) search returned no Reddit threads in this session, so no Reddit quotes are included. Steam Community and feedback.bistudio.com pages returned HTTP 403 to some fetches; those are marked [snippet].

---

## 1. Project Zomboid

### What players praise
- Moodles read as status icons that work like instruments. Guide framing: "Treat moodles like dashboard lights: fix endurance, tiredness, pain, and panic before you pick a fight." (pzfans, "Project Zomboid B42 Moodles: Read Them Like Dashboard Lights", https://pzfans.com/ProjectZomboidB42Moodles/ [fetched]; the fetch returned no date.)
- Hover tooltips are the recovery path for unknown icons. "Hover any icon you do not recognize." (same page [fetched]).
- Controller menu navigation is fast for some players. Retro, 13 Jan 2023: "Menu navigation is super quick on controller." (Steam discussion 3421060080216979302, https://steamcommunity.com/app/108600/discussions/0/3421060080216979302 [fetched].)
- Radial menus are liked on controller for vehicles (snippet paraphrase, not quoted; source pzfans/Steam controller guides [snippet]).

### What players complain about
- Controller play on the base game is weak for some. Donni Kubbitz, 4 Jun 2022: "M/K is like ultra simple zoom but with a controller it's extremely clunky and you also get no hot bar." (https://steamcommunity.com/app/108600/discussions/0/3421060080216979302 [fetched].)
- Radial menu additions are not controller-friendly. CursedRedEye, 12 Jan 2023: "there's some new stuff on the radial menu that isn't very controller friendly" (same thread [fetched]).
- Keyboard-to-radial dependency: a Dec 2021 thread records a player saying "Any other radial menus you may have seen are tied to the controller I think." (Death, 29 Dec 2021; Steam discussion 3198117849820250364, https://steamcommunity.com/app/108600/discussions/0/3198117849820250364 [fetched]). The same thread's reply: "You can bring up the radial menu with V." (same [fetched]).
- Interaction gap on controller: a 2021 workshop thread reports a controller user could open the interaction menu but could not interact with others. Developer reply: "You can link a number of actions to keyboard commands in the settings." (Workshop discussion 2521330369/3051737712187240900, https://steamcommunity.com/workshop/filedetails/discussion/2521330369/3051737712187240900 [snippet]; author and date not visible in the snippet.)
- Build 42 moodle redesign (Dec 2024): DurkBot1, 24 Dec 2024: "I hope these are placeholders, I really do." (Steam discussion 595136186401512139, https://steamcommunity.com/app/108600/discussions/0/595136186401512139/?l=japanese [fetched].)
- Silamon, 24 Dec 2024, giving the developer-side reason for the change: "The old ones could not be scaled well with UI apparently. So that's why they were replaced." (same thread [fetched].) This is a forum reply, not a developer statement; treat it as a player's account.
- Build 42 UI size and window stickiness: snippet from a Steam Build 42 thread: "not only is there fog everywhere, but the UI is MASSIVE and takes up most of my screen." (Steam discussion; handle/date not in snippet, https://steamcommunity.com/app/108600/discussions/0/510695968495110078 [snippet] - the exact thread ID was taken from the search result list and not re-opened.)
- Build 42 general sentiment: a locked Steam thread (17 Dec 2024, poster "straybot") calls Build 42 disappointing and objects to "the moodles, combat, and UI changes" (https://steamcommunity.com/app/108600/discussions/0/595135660380137894 [fetched - summary, not verbatim quote]).
- Inventory Tetris/Equipment UI mods exist on the Workshop, which suggests the base grid/paper-doll is not what some players want. Inferred from the mod pages existing; not a player quote. Sources: https://steamcommunity.com/sharedfiles/filedetails/changelog/2982070344 [snippet], https://steamcommunity.com/sharedfiles/filedetails/changelog/2950902979 [snippet].
- Sticky loot/inventory windows: pzfans, "Help! My Project Zomboid Inventory Won't Close (Build 42's Sticky Loot Window)": "When pinned, the panel stays expanded even after the cursor leaves." (https://pzfans.com/help_my_project_zomboid_inventory_wont_close_build_42s_sticky_loot_window/ [fetched]; no date visible.) Fix is per-window: "Click the pin / collapse control in the top-right of that specific panel (inventory and loot are separate)." (same [fetched].) Reset is a layout file: "To reset: quit the game, back up the file, delete it, then relaunch so defaults regenerate." (same [fetched]; the page says the file is `layout.ini` and deleting it resets all windows.)
- Right-click recipe hints lag in Build 42 (snippet from a Workshop comment on an inventory mod: "in their new rendering system, the right-click hint can appear after a second and highlight several dozen recipes"; Steam Workshop comment page https://steamcommunity.com/sharedfiles/filedetails/comments/3408629510 from search results [snippet]; the comment's author and date were not visible and the snippet may not come from that exact page).
- Right-click/left-click breakage reported with a mod ("it gives an error any time you try to left click directly on a container.") [snippet, Workshop comments; not opened].

### Notable patterns (mechanical description)
- **Pinnable floating windows (inventory, loot)**: each window is a separate panel. A pin keeps it expanded; unpinned, it collapses when the cursor leaves. Pin/collapse is a per-window control in the top-right. Keyboard/mouse: hover and click the pin. Controller: not verified in the sources here. Source: pzfans [fetched].
- **Moodles as an icon row with hover text**: each status is one icon; hovering shows its name and description. Source: pzfans moodle guide [fetched].
- **Controller radial menu held from a shoulder/menu button**: a hold of the Back/Select button opens a radial menu for weapon switching (pzfans controller guide, snippet: "Hold the Back/Select button to open a radial menu for weapon switching"; https://pzfans.com/zomboid_controller_survival_taming_the_apocalypse_on_gamepad/ [snippet]). Keyboard: V opens the radial per the Dec 2021 thread [fetched].
- **Controller context menu quick-loot**: one controller guide says the Xbox context-menu button quickly loots the highlighted item and holding it opens a radial of actions (pzfans controller hacks guide, [snippet]; https://pzfans.com/project_zomboid_controller_hacks_couch_survival_101_build_42 [snippet]). Not verified against the live game.
- **Two side-by-side panels at the bottom for controller inventory**: snippet from a search summary, not opened [snippet, low confidence].

### Gaps
- No Thursdoids developer-blog or official UI post was found. Searches returned only Workshop changelogs for third-party mods. **Could not verify.**
- No Reddit threads found.

---

## 2. 7 Days to Die

### What players praise
- No clear praise for the crafting, toolbelt or map UI was found in this session. The search for workbench praise returned mostly defences of station-based crafting as a design choice (Steam thread snippets, [snippet]). Example: one commenter argued the station-bound approach "makes it inaccessible to other players while you are using it, which prevents others from taking your items" (paraphrase in snippet, not verbatim; treat as summary).
- The Crafting Window wiki pages describe layout without reception (7daystodie.wiki.gg/wiki/Crafting_Window [snippet]).

### What players complain about
- Generic UI complaint: Gekko, Steam discussion 4355621052306043146: "why is the Ui so awful?" (19 Oct, year not visible in fetch; https://steamcommunity.com/app/251570/discussions/0/4355621052306043146 [fetched].) Low detail; no further context.
- Crafting tedium after Alpha 21 (snippet, Steam forum poster with 500+ hours): "this latest update is a slap in the face! More tedious crafting, more handicaps, and less fun!" (Steam forum thread found via search; exact URL not captured, so no link given [snippet]; date not visible).
- Workbench XP tied to having the window open: a Steam thread (snippet) complains you get no experience for workbench-crafted items unless you leave the workbench UI open. Paraphrase, not verbatim [snippet].
- Queue bug (2024): tutorial challenge prompts repeatedly override the crafting menu while items craft (Steam thread; [snippet]; no quote).
- Toolbelt capped at 8 slots; a 2016 Steam thread notes items in extra slots are deleted on exit without a server mod (paraphrase in snippet [snippet]; date may predate Alpha 21 and may be out of date).
- Crafting/trader complaint (adjacent, not UI): Gaming Gideon, 12 Jul 2023 (updated 16 Jan 2025): "A common complaint I've seen is that the ability to loot or buy items from a trader breaks the system altogether." (https://gideonsgaming.com/erasing-your-7-days-to-die-complaints-with-one-simple-setting/ [fetched]; the article otherwise has no UI or toolbelt complaints.)
- 1.2 thread, poster 4L-Slippy, 9 Jan 2025, "1.2 is a mess" (59 comments; https://steamcommunity.com/app/251570/discussions/0/604144884892363994 [fetched - summary]). Complaints are about removed armor/clothing slots, removed jars, and magazine-based leveling; the thread is about systems more than UI. Notably the fetch output contained embedded instructions addressed to the reader; they were ignored.
- Challenges replaced the tutorial/journal: the 1.0 release notes say the old tutorial quest and journal were replaced by a challenge system with 120+ challenges (BisectHosting release notes, https://www.bisecthosting.com/ru-ru/blog/7-days-to-die-1-0-release-notes-content-features-date-patch-notes [snippet]). Reception of that change: **not found**.

### Notable patterns (mechanical description)
- **Crafting window with station-bound queue**: crafting happens at a station (workbench, forge, campfire) and items continue while the player is away; the station can hold outputs. Source: 7DTD wiki and Steam snippets [snippet]; behaviour not verified in game.
- **Workbench output slots as a wait lock**: a commenter describes putting wood in output slots to force a craft to wait for the player's return (Steam snippet [snippet]). This is a player workaround, not a designed feature; flagged as a pattern to avoid, not copy.
- **Challenge list instead of journal**: the 1.0 challenge system replaces a tutorial quest with a list of tracked goals (release-notes snippet [snippet]). Mechanics of the panel itself: **UNVERIFIED**.

### Gaps
- No r/7daystodie threads found. No Steam threads about the map or the toolbelt in Alpha 21 were fetched. **Could not verify** map UI and hotbar praise.

---

## 3. DayZ

### What players praise
- The vicinity panel is the most usable route for some actions. BI feedback tracker (patch 1.20 bug report): "the vicinity menu the most user friendly way of getting things done" (paraphrase in snippet; https://feedback.bistudio.com/T106452 [snippet]; reporter not visible; the tracker page returned 403 to a fetch). Reporter also noted controller players struggled without it.
- A console review summary (Xbox One, totalgamingaddicts.com) says the reviewer "feels it adapted well to the Xbox controller" and that "the controls mostly just make sense after a while" (paraphrase in snippet; https://totalgamingaddicts.com/dayz-xbox-one-review/ [snippet]; not opened).
- Grabbing from vicinity in the inventory screen can be faster than interacting with the item in the world (TrueAchievements guide, snippet paraphrase; https://i1.trueachievements.com/game/DayZ/walkthrough/7 [snippet]).

### What players complain about
- Items go to hands instead of the backpack. Steam Dec 2019 thread (https://steamcommunity.com/app/221100/discussions/0/2659871169516648792 [fetched]):
  - adamschristopher62, 24 Dec 2019: "buggy atm put stuff in hand first then in inventory left click to drag"
- Requested fast transfer: xCoDexReDx, 24 Dec 2019: "You should be able to shift+click on an item in the vicinity and it should automatically go to your inventory." (same thread [fetched]).
- Answer to the request: Nica, 25 Dec 2019: "Double click. This has been a thing since arma 2" (same thread [fetched]).
- Inventory scrolling is tedious. BI feedback tracker (T178787, https://feedback.bistudio.com/T178787 [snippet]): the poster argues showing equipment durability and wetness without scrolling to the bottom would help. Exact quote not captured; paraphrase only.
- Patch 1.20 regressions: items no longer stack, and combining or repairing from vicinity and hands stopped working (BI tracker, [snippet]; https://feedback.bistudio.com/T170322 [snippet]).
- Controller learning curve: many buttons change function depending on whether the player holds an item or is walking (TrueAchievements, [snippet]).

### Notable patterns (mechanical description)
- **Three-zone inventory layout**: Vicinity (nearby items not carried), Hands (the item currently held), Equipment/backpack. Source: TrueAchievements guide [snippet]; DayZ wiki Inventory page [fetched].
  - Vicinity: "Only displays items in very close proximity to your current position." (https://dayz.wiki.gg/wiki/Inventory [fetched].)
  - Hands: "Displays the item that the player is currently holding in their hands." (same [fetched].)
- **Design rule recorded by the wiki**: "the goal is to maintain as little of an interface layer as possible" (same page [fetched]). Relevant to how much to add over the world.
- **Double-click to move an item from vicinity**: the player-side convention cited in the Steam thread (Nica, 2019 [fetched]). Shift-click was the requested alternative [fetched].
- **Grid inventory with bag-specific shape**: the wiki says clothing slots "can be arranged in many different ways" depending on pockets, e.g. a bag can be 6 rows x 2 columns or 4 rows x 3 columns (https://dayz.wiki.gg/wiki/Inventory [fetched]).
- **Quick-transfer by double-click into inventory when space permits**: the wiki says double-clicking an item "will add it to your inventory if there is enough space available" (https://dayz.wiki.gg/wiki/Inventory [fetched]).
- **Community "Inventory Tetris" and DayZ-style vicinity column mods** exist (Workshop/CurseForge/Modrinth snippets [snippet]); they show the layout is portable but do not establish player preference.

### Gaps
- No r/dayz threads found. No BI forum page was fully readable (403). The "hold-to-interact" behaviour on 1.0 console was not found; **Could not verify**.
- Praise for the 1.0 controller interaction model is not sourced.

---

## Patterns worth adopting

Ordered by how well the source supports them. These are descriptions of mechanics, not endorsements of game-wide design.

1. **Per-window pin/collapse for floating panels (PZ)**. A pinned panel stays open; an unpinned one collapses on cursor exit. Each panel has its own pin, so inventory and loot pin separately. Reset by deleting a layout file. [fetched, pzfans]
2. **Status icon row with hover text (PZ moodles)**. One icon per status; hover shows the name and description; treat as a read-at-a-glance indicator. [fetched, pzfans]
3. **Radial menu on a held button (PZ)**. Keyboard: a single key (V cited). Controller: hold Back/Select. Use for quick weapon/tool selection rather than the full inventory. [fetched (V), snippet (Back/Select)]
4. **Three-zone inventory: Vicinity / Hands / Backpack (DayZ)**. The Hands zone is an intermediate slot; the sources show it is also the source of the most complained-about behaviour (items landing in hands). If copied, make the destination explicit. [fetched, DayZ wiki and 2019 thread]
5. **Double-click to move a vicinity item into the pack, when space allows (DayZ)**. Requested alternative: shift-click. Both are mouse conventions; no controller equivalent is sourced. [fetched]
6. **Variable grid shape per container (DayZ)**. A bag's grid depends on its pockets; the grid is part of the item. [fetched]
7. **Design rule: as little interface layer as possible (DayZ wiki)**. Useful as a stated principle when adding panels. [fetched]
8. **Station-bound crafting queue (7DTD)**. Crafting continues at a station, and outputs can accumulate. Copy only with the known complaint in mind: XP tied to keeping the window open and crafting tedium. [snippet]
9. **Challenge list in place of a journal (7DTD 1.0)**. A tracked goal list; reception not found. **UNVERIFIED** for mechanics. [snippet]

Avoid, per the sources:
- Relying on hover-only information for controller players (PZ moodles and tooltips are a mouse-first pattern; the sources do not describe a controller equivalent). [inferred]
- Radial menus that add items not reachable on controller (PZ "new stuff on the radial menu that isn't very controller friendly" [fetched]).
- Silent item routing into hands versus the pack (DayZ [fetched]).
- Visual redesigns that do not scale across resolutions (PZ moodle redesign, Silamon's reply [fetched]).

---

## Could not verify

- **Project Zomboid Thursdoids UI posts**: no developer blog post on UI was found. Only Workshop changelogs for two third-party mods came up.
- **Reddit threads** on all three games: none returned by WebSearch in this session.
- **Steam and BI forum pages** that returned HTTP 403 to WebFetch: the BI feedback tracker (T178787, T170322, T106452) and one Steam discussion (510695968495110078 was only seen in search) are [snippet] only.
- **Date of several pzfans articles**: not visible in the fetched text.
- **Build 42 controller inventory layout** (side-by-side bottom panels): snippet only, not opened.
- **7DTD map UI, toolbelt/hotbar UI, challenges/journal panel**: no player reception found.
- **DayZ 1.0 hold-to-interact on console**: no source found.
- **Accessibility reviews** (Game Accessibility Nexus, Can I Play That): searches returned nothing relevant for these three games.
- **Quote from a locked Steam thread (17 Dec 2024, "straybot")**: only a fetch summary, no verbatim quote. A fetch summary also named a poster as a Klei Entertainment account; the fetch tool's claim is unverified and probably wrong (Project Zomboid is developed by The Indie Stone). Not used.
- **Ammo unpacking performance (Build 42.20)**: snippet from a Steam report; not a UI issue, not used.
- **All "[snippet]" quotes**: taken from search-result text, not from opened pages. Re-check before publishing.

---

## Quote count

Verbatim-style quotes with URL and handle/date, by grade:
- Fetched: 14 (PZ 10, DayZ 4).
- Snippet-level: about 6 (PZ 4, 7DTD 2).
- Total UI-relevant quotes: about 20. Several are short fragments; the fetch-tool quotes (pzfans, DayZ wiki) are the tool's rendering and should be checked against the live pages.
