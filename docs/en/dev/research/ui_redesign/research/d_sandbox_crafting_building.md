# Crafting and base-building UI in survival sandboxes

Research date: 2026-10-21. Scope: Valheim, Subnautica (and Subnautica 2), Minecraft (vanilla recipe book, JEI/REI), Vintage Story, Satisfactory, Sons of the Forest, Green Hell, Grounded, Enshrouded.

## Method and reliability

- Sources were found with WebSearch, then opened with WebFetch. WebFetch returns text rewritten by a small model, so quotes marked **[fetched]** are the extracted wording, not a byte-exact copy. Re-check any quote before publishing it.
- Quotes marked **[snippet]** come only from search-result summaries. I did not open those pages, so treat them as UNVERIFIED.
- Dates are shown only where the page or thread showed them.
- Reddit was not reachable through search in this session. No Reddit thread is cited.
- Steam pages that returned HTTP 403 or no thread body are listed under "Could not verify".
- Quoted handles are as displayed on the source page.

## 1. Valheim

### Praise
- No verified praise for the hammer UI turned up in the fetched threads. The sources were mostly questions and complaints. Mod pages are the closest thing to praise, but they are self-description, not user reviews.

### Complaints (fetched)
- Raman, 31 Jan 2025, Steam "Valheim" discussion 600769077072443583 **[fetched]**: "With 49 wood in inventory+hammer equipped,right click shows me furniture tab,However in this tab..."
- Raman, 31 Jan 2025, same thread **[fetched]**: "Most strange was following a top ranked guide and that stated to craft workbench open furniture tab"
- xanthosthewise, 19 Dec 2022, Steam discussion 3101267090242259022 **[fetched]**: "IT IS TIME - The building tab is too cluttered. Split into Masonry and Carpentry tabs"
- Rhapsody, 19 Dec 2022, same thread, supporting a queue **[fetched]**: "queuing of some sort could probably be useful for players with controllers or some kind of accessibility issues."
- N0ma13, 14 Apr 2021, Steam discussion 5143704785449399781 **[fetched]**: "This is one of the most annoying aspects of the hammer imho."
- Daxzier, 29 Jan 2022, Steam discussion 4594236462785672596 **[fetched]**: "If items don't show up in crafting menu, I can't craft them?"
- Thor'sHammer, 21 May 2023, same thread as Daxzier **[fetched]**: "IF YOU ARE MISSING WINDMILL AND OTHER ITEMS YOU SHOULD CRAFT SOME IRON NAILS!!"

The pattern in these threads: stations and pieces are hidden behind unlocks and tabs, and the player cannot tell why an item is missing from the menu. The answers that fixed it were tab location and unlock prerequisites, not UI changes.

### Patterns the fans built (mods, fetched)
- **Recipe Pinner** (KadrioS, Thunderstore, v1.3.0, "3 months ago" on the page) **[fetched]**: "Pin any recipe or build piece to your HUD and track the materials you still need in real time."
- **HammerCraftingMaterials** (DrummerCraig, v1.1.3) **[fetched]**: "Right-click any piece in the hammer build menu to pull its required materials from nearby chests." It also says: "Searches all accessible chests within a configurable radius (default 10 m)."
- **AAA Build Menu** (Azumatt) **[fetched, model-rendered]**: classic menu quick filters "Can Craft, Missing, Favorites, and Recent"; the 1.0 menu's Can Build tab shows "only what you have the materials and crafting station for right now"; and the pinned tracker "lists the materials you still need for your pinned builds." Filter text: "Type to filter the current category, or every category at once when global search is on."
- **Buildheim** changelog (AugusDogus) **[snippet]**: material and station shortage display follows vanilla rules, including shortage colours and a crafting-station indicator.

Takeaway: the modding community added the features the base game lacks: missing-material lists, a can-build filter, pins, and remote material pull. These are concrete, already-tested requests.

## 2. Subnautica (original) and Subnautica 2

### Praise
- SeeJay, 21 Mar 2018, Steam discussion 1698294337766641677, on a habitat-builder idea **[fetched]**: "Would be nice to have it in game tho. Great idea." This is a suggestion, not a review. Included because it's the only fetched praise in this group.

### Complaints (fetched)
- gowar20, 18 Jun 2017, Steam discussion 2333276539607034660 **[fetched]**: "This morning the modification station blue print is not amongst the build menu."
- Tagom, 5 Jul 2017, same thread **[fetched]**: the station is "NOT present in the Habitat builder tool menu."
- atroxpaulsen, 21 Mar 2018, Steam discussion 1698294337766641677 **[fetched]**: "Since the right-click function closes the menu today." The request was to let the player batch-select blueprints from the construction menu.
- Valden21, 9 Jul 2018, Steam discussion 1727575977594933279 **[fetched]**: "How to get Habitat Builder?" The thread is mostly answers, which shows the Habitat Builder's unlock path was not discoverable in-game.

### Subnautica 2 (2026) changes
- Rasea, Substack, 21 May 2026 **[fetched]**: "Additionally, the fabricator menu never disappears." "So now everything will snap into place on a grid, and you can turn it off if you want." Also: "your fabricator and Habitat Builder will now craft items pulling from your storage."
- The same post describes these as fan-mod features pulled into the base game. That is a secondhand description; I did not open the mods.

### Snippet-only (UNVERIFIED)
- Keengamer guide [snippet]: the Habitat Builder is crafted at a Fabricator from 2 Titanium, 1 Glass, 1 Basic Battery, and 1 Copper Wire. Some players don't see the tool equipped and are told to check the PDA Inventory.

### Patterns
- Blueprint unlock status is hidden: players could not tell whether a piece was missing or locked (gowar20, Tagom). A visible "unlock needed" state would have answered most of these threads.
- A menu that stays open while placing (the 2026 Subnautica 2 change) answers a complaint that dated to 2018.

## 3. Minecraft (vanilla recipe book, Java and Bedrock)

Note: every Mojang feedback page and the Mojira bug returned HTTP 403 or no body when fetched. The material below is from search snippets, not fetched pages. Treat all of it as UNVERIFIED until checked.

### Complaints [snippet, UNVERIFIED]
- Feedback forum (360017835352): a request to make the recipe book closed by default. Commenters say they "already know their recipes or look them up online" and find the panel distracting.
- Feedback forum (360043036392): shift-click into the crafting table is requested because it would avoid "hunting through the inventory for each ingredient."
- An Xbox player says the recipe book is "too cluttered to replace the older grid-distribution controls."
- Mojira MC-259107 (layout bug): at aspect ratios taller than 16:9 the recipe book appears above the crafting table and does not populate the grid.
- An itch.io commenter argues recipes should not depend on where items sit in the grid.

### Praise
- None verified.

### Patterns (from these sources)
- A recipe panel that can be dismissed by default and closed for good. Users asked for an option to turn it off entirely.
- Shift-click to move a full stack into a crafting grid, and a grid that accepts a recipe regardless of where its ingredients sit.
- Controller players need a non-drag fill path; dragging into each slot is slow.

## 4. JEI and REI (mod ecosystem)

No official JEI or REI documentation was fetched. Everything here is from search snippets (modpack index pages, mod listings). UNVERIFIED.

### Patterns [snippet]
- JEI: clicking an item shows its crafting recipe; the usage view answers "what can I make with this." Shortcut keys: R or left-click for recipes, U or right-click for uses. The "+" button next to a recipe moves ingredients from the inventory into the grid. Search by name, mod, tag, or tooltip text.
- JEI Transform: when materials are short, the transfer pulls the shortfall from nearby containers.
- JEI Crafting Insight: presses "A" to pin items to the left side of the screen and track missing materials. Recipe slots are coloured green for held, red for missing.
- JackItToMe (JEI/REI/EMI): colours slots by stock. Red means missing with no AE2/RS pattern to craft it; green means missing but craftable on the network.
- Tom's Simple Storage NBT fix: damaged tools in storage showed red in the transfer preview even though the transfer succeeded. This is a preview-accuracy bug: the highlight must match what the click does.

## 5. Vintage Story

### Praise
- None found in user sources. The fetched material is the game's own wiki.

### Handbook and grid (fetched from wiki.vintagestory.at, Crafting page)
- "The Handbook is your best friend! Press the H key at any time to open it."
- "If you do not know the shape to use, press H to check the Handbook!"
- "Right-click and drag to quickly spread items in the grid."

Note: a search summary claimed the page carries an "outdated" notice. The fetched page had no such note. The summary also described the drag shortcut as dropping exactly one item per square; the fetched wording is "quickly spread items." Treat the one-per-square detail as UNVERIFIED.

### Community
- Improved Handbook Recipe Helper (mods.vintagestory.at/show/mod/47215) **[snippet]**: adds auto-fill buttons and mouse-wheel scrolling through recipe variants. Listed as supporting 1.22.0 to 1.22.7.

### Patterns
- In-game handbook is reachable with one key from any screen, and it shows the recipe shape. That is the reference the other games lack.
- Recipe variants (any wood, any planks) are scrolled through in place, not shown as separate entries.

## 6. Satisfactory

### Complaints (fetched, Steam discussions)
- Spogg, 20 Sep 2024, Steam discussion 4845399746424677958 (Hard Drive list) **[fetched]**: "I scanned to many hard drives, now I can't select or reroll them because theres no scroll bar or anything"
- Khaylain, same thread **[fetched]**: "Yeah, it's a poorly implemented UI."
- WinterSolstice, same thread **[fetched]**: "It was an oversight I believe and easily fixed because the code is there already"
- x0Z3ro0x, 27 Jan 2024, Steam discussion 4147320315759001824 (codex search) **[fetched]**, replying to a proposal for search-by-name that shows recipes: "This would require knowing what you've unlocked which defeats what the OP is trying to do."
- Clumsy, same thread, 27 Jan 2024 **[fetched]**: "The only thing required to know is the item. Not the recipe."
- Fenix, same thread, 27 Jan 2024 **[fetched]**: "N = Search for it by typing the name, and it will open codex straight at that item"

### Build-menu workflow (fetched)
- scrooge34, 3 Jan 2022, Steam discussion 3203747834755864032 **[fetched]**: asked for a way to select a building by clicking it, "so much faster than selecting from the build panel."
- Vectorspace, 4 Jan 2022, same thread **[fetched, paraphrased]**: press F for the dismantle gun, then middle-click a building to make it the active build selection.
- scrooge34, 13 Jan 2022, same thread **[fetched]**: confirms "F" and Middle click is "exactly, what I was looking for."
- flannan, same thread **[fetched]**: "10 build panels" and recommends tailoring them to specific workflows.
- nhatanh0475, same thread **[fetched, paraphrased]**: binding buildings to the hotbar and cycling a category with E to swap belt tiers.

### Praise
- scrooge34's "exactly, what I was looking for" on F + middle-click is the only praise I fetched. It praises a workaround, not the menu.

### Patterns
- Copy-the-thing-you-are-pointing-at (F + middle-click, recipe-to-pick) beats browsing a menu. Two players asked for it independently.
- Hotbar binding of buildings with a cycle key for tiers. The request is that the player bind a building and cycle tiers with one key.
- Search by typed name that opens the item. The objection (x0Z3ro0x) is about revealing locked recipes: search must not expose unknown items.

### Hard Drives and alternate recipes [snippet, UNVERIFIED]
- Guides conflict on reroll rules. One says a drive offers two alternate recipes and can be re-rolled once. Another says a reroll consumes the drive. One says alternate recipes are usable only in factory buildings, not the handheld Craft Bench. Check the wiki or in-game before relying on any of these.

## 7. Sons of the Forest

Every Steam discussion page I opened returned no thread body (403 or only a content warning). The points below are from search-result snippets only, UNVERIFIED.

### Complaints [snippet, UNVERIFIED]
- Blueprints: ghost-image placement works for platforms and ramps, but walls and doorways can't use it. Players place logs one at a time with aim-based snapping. Dec 2024.
- Two building books: the right book is a build tutorial, the left holds placeable blueprints. Players must switch between them. Mar 2023.
- Crafting max: crafting a stack needs the inventory closed and reopened for the last one. A "craft max" option was suggested. Dec 2023.
- Full inventory: crafting and combining fail when items are held at maximum quantity. Jan 2024.

### Patterns from these snippets
- A single-place building book, a visible craft-max, and snapping that works for walls and doorways.

## 8. Green Hell

Every Green Hell thread is from search snippets only (no thread body fetched). UNVERIFIED.

### Complaints [snippet, UNVERIFIED]
- New blueprints do not appear in the notebook for some saves. Reloading sometimes fixes it.
- Blueprint unlock conditions are unclear: one 2022 thread asks how to get new recipes; answers say picking up a bamboo stick unlocks the blowgun blueprint, and crafting the blowgun unlocks the frog stretcher.
- Placement on a blueprint ghost fails; one player reports a fireplace that stayed as a blueprint.
- Floating save button in the shelter was missed; the tutorial does not explain how to research.

### Patterns
- The notebook should show, next to each locked blueprint, the action that unlocks it.

## 9. Grounded

No Grounded thread body was fetched. The Windows Central changelog returned truncated text without the 0.7.0 entries.

### Snippet-only (UNVERIFIED)
- Grounded 0.7.0 (Feb 2021): non-grid buildings can be relocated without being destroyed; crafting draws ingredients from nearby storage.
- Placement stopped working while zipping, gliding, or fighting (0.7.0 notes).

## 10. Enshrouded

### Complaints (fetched, Steam)
- Traumacare, 24 Jan 2024, Steam discussion 4142816460193592893 **[fetched]**: "WTF, what is up with these tiny tiny foundation blocks?"
- Dead Goat, same thread, 24 Jan 2024 **[fetched]**: "Yea I remember building being fun in the demo but this... this was painful"
- Gunther Wolfgang, same thread, 24 Jan 2024 **[fetched]**: "does anyone else have problems with the building menu when scrolling?"
- LuckyHaans, 16 Feb 2024, Steam discussion 7599331177366914867 **[fetched]**: "once I put building mats away in a chest I wasn't able to build without taking them back out."
- Jollykitty, 17 Feb 2024, same thread **[fetched]**: asks for "an option to take stuff from magic chest when you are BUILDING."

### Patterns (fetched)
- Size switching inside the build menu: Blaise, same thread, 24 Jan 2024 **[fetched]**: "you can switch between 1x1, 2x2 or 4x4, peruse around the building menu to find it." X-C0brA-X: "You can hold ALT and choose bigger blocks." The discoverability problem is the same as Valheim's tab problem.
- Magic chests feed crafting from anywhere in the base (xitooner, same thread, **[fetched]**: "The items stored here can be used for crafting purposes from anywhere in the base."). The gap: they don't feed placement. Placement is the complaint.

### Snippet-only (UNVERIFIED)
- Multi-block placement hotkeys with the building hammer: Tab changes shape, Alt+scroll changes rows, Ctrl+scroll changes material (Steam thread 4-1 to 4-5). Not fetched.
- Feature Upvote suggestion 712562: build furniture and decorations from the build menu so placement can be previewed before crafting. The page returned HTTP 403.

## Patterns worth adopting

Each pattern is tied to a source above. Keyboard/mouse and controller are listed separately. Where a pattern is only snippet-sourced, it is marked.

### Keyboard and mouse
1. **Can-build / missing filter as a first-class tab or filter.** Source: AAA Build Menu (Azumatt, fetched). Filter by Can Craft, Missing, Favorites, Recent.
2. **Pin recipes or build pieces and show the remaining shortfall in a HUD tracker.** Source: Recipe Pinner (KadrioS, fetched). Real-time missing-material list.
3. **Right-click a build or craft entry to pull its shortfall from nearby containers within a radius.** Source: HammerCraftingMaterials (DrummerCraig, fetched; default 10 m). Show the radius in the UI.
4. **Type-to-filter across all categories**, with a global toggle. Source: AAA Build Menu (fetched).
5. **Pick a building from the world, not the menu.** F + middle-click to make a placed building the active build. Source: Satisfactory thread 3203747834755864032 (fetched).
6. **Single-key handbook.** Vintage Story's H opens the reference from anywhere (fetched). Show the recipe shape, not only a name.
7. **Size or variant switch inside the build menu** (1x1/2x2/4x4 in Enshrouded; Alt to choose a bigger block). Source: Enshrouded thread (fetched). Show the active size in the menu.
8. **Shared-storage feed for placement, not only crafting.** Gap found in Enshrouded (fetched): magic chests feed crafting but not placement. Valheim's HammerCraftingMaterials mod fills this gap for placement (fetched). The Sons of the Forest equivalent is not verified.
9. **Preview that matches the click.** Tom's Simple Storage NBT fix [snippet]: a red highlight that does not match what the transfer does destroys trust. The preview must use the same rules as the action.
10. **Show the unlock action next to a locked entry.** Green Hell and Valheim complaints are both about items hidden behind unlocks with no explanation [fetched for Valheim, snippet for Green Hell].
11. **Do not reveal unknown recipes via search.** Satisfactory: search by name is fine, but it must not expose a recipe the player has not unlocked (x0Z3ro0x, fetched). Show "unknown" rather than the answer.
12. **Snap to a grid with an off switch.** Subnautica 2 (Rasea, 2026, fetched): snapping on by default, toggle to disable.
13. **Menu that stays open during placement.** Subnautica 2 (Rasea, fetched): "the fabricator menu never disappears." Subnautica (2018) complaint: right-click closes the menu (atroxpaulsen, fetched).

### Controller
1. **Queue crafting.** Valheim thread (Rhapsody, fetched): queuing "could probably be useful for players with controllers or some kind of accessibility issues." Queueing removes the need to repeat a long action.
2. **Fill-the-grid action without dragging.** Minecraft controller complaint [snippet]: dragging items slot by slot is slow. A one-press "fill from storage" is the pattern; shift-click-equivalent is the keyboard analogue.
3. **Hotbar binding for buildings and tier-cycle on one button.** Satisfactory (nhatanh0475, fetched): bind a building to the hotbar and cycle its variant with E.
4. **Keep the same number of build panels as there are workflows.** Satisfactory (flannan, fetched): "10 build panels" is a problem; tailor to workflows and bind them.
5. **Controller remap in-game, not via Steam.** Valheim controller binds are only changeable in Steam (BisectHosting guide, snippet). Expose remapping in the game's own settings.
6. **Avoid the hard-to-reach category.** Valheim: the build menu is scrolled slowly; players move frequently used items to the hotbar (Satisfactory thread, snippet).

## Could not verify

- **Reddit threads.** None reachable through search in this session. No Reddit claim is in the report.
- **Minecraft feedback.minecraft.net pages and Mojira MC-259107.** WebFetch returned HTTP 403 or no content. All Minecraft complaint claims are snippet-only.
- **Sons of the Forest Steam threads.** Two fetched pages returned no body. All SotF claims are snippet-only.
- **Green Hell.** No thread body fetched. All claims are snippet-only.
- **Grounded.** Windows Central changelog truncated before the 0.7.0 entries. Grounded claims are snippet-only.
- **Enshrouded Feature Upvote 712562 and Steam thread 4298194451986230064.** 403 or not fetched.
- **Enshrouded multi-block hotkeys.** Snippet-only.
- **Satisfactory Hard Drive reroll rules.** Guides conflict. Not resolved.
- **Ingredient substitution display.** No source found in any game. Not covered.
- **Placement preview with validity colour, icon and text.** No source found that describes this in a surveyed game. The Subnautica 2 grid snap and Enshrouded size switch are the closest found, and neither describes a validity colour.
- **Reverse lookup ("what can I make with this").** Found only for JEI (usage view, U key), snippet-only.
- **Craft-N / craft-max as a UI pattern.** Found only as a Sons of the Forest complaint (snippet). No game's craft-max implementation was verified.
- **Accessibility reviews.** No accessibility review from a major outlet or a formal audit was found. The Valheim controller discussion turned up only guides and a Game Revolution article (partial controller support, DualSense issues, reliance on the mouse for drop/repair), not an accessibility audit.
- **Official JEI or REI documentation.** Not fetched. REI controls are not described in any source I opened.
- **Vintage Story "outdated" note and one-per-square drag.** Contradicted by the fetched page; see section 5.
- **Subnautica 2 Keengamer guide claims** (Habitat Builder recipe, PDA equip issue). Snippet-only.
- **Valheim 1.0 Can Build tab and filter wording** comes from a model-rendered extraction of a mod page; check the mod's own text before quoting.
