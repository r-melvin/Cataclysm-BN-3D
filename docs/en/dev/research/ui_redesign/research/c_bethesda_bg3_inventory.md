# Inventory, crafting, trade and character-sheet UI: lessons from Bethesda RPGs and Baldur's Gate 3

Research date: 2026-10-21. Scope: Fallout 4, Fallout 76, Skyrim (vanilla and SkyUI), Starfield, Baldur's Gate 3.

## Method and reliability

- Sources were found with web search, then opened with WebFetch. WebFetch passes the page through a small summarising model, so quoted text marked **[fetched]** is what that model returned. It is close to the page but not guaranteed character-exact. Re-check any quote against the live page before publishing it.
- Quotes marked **[snippet]** appear only in search-result summaries. They were not confirmed on the page.
- Nexus Mods pages (SkyUI 12604, FallUI, StarUI 16378) and most Steam discussion pages returned HTTP 403 or a content-warning wall. Nothing below is quoted from those pages.
- Steam discussion threads are mostly 2015 to 2019 (Fallout 4) and 2023 (BG3, Starfield). Patches may have fixed the issues. None of the claims were checked against current patch notes.
- The Reddit threads named in the brief were not reachable. Reddit mirrors were blocked, or redirected to an auth page.
- Quotes are as found. Handles and dates are given only where the fetched page showed them.

## Fallout 4 (Pip-Boy, transfer menu, workshop)

**Complaints**

- **[fetched]** F4TES, Steam, 1 Dec 2015 (https://steamcommunity.com/app/377160/discussions/0/492379159717872389): "Why oh why couldn't they give us a quick & easy way to sort our inventory?"
- **[fetched]** F4TES, same thread: "When I'm in Pip Boy, I have all those types categorised, but not in transfer (R) !"
- **[fetched]** F4TES, same thread: "to be able to mark the exact amount you need & then be notified when you've got enough of it."
- **[fetched]** F4TES, follow-up, 1 Dec 2015 12:07pm, same thread: "I should be allowed to deposit that either where I got it from", about a 7 wt quest coat that forced them to "watch my weight constantly."
- **[fetched]** CoffeeGecko8, Steam, 19 Nov 2015 (scrap/Enter thread, https://steamcommunity.com/app/377160/discussions/0/496881136908404402): "I couldn't believe how badly the UI was messed up for PC that it is misleading in a number of areas."
- **[fetched]** CoffeeGecko8, same thread: "there's no scroll bar in a trade/container menu, you have to manually cycle down with the mouse wheel"
- **[fetched]** CoffeeGecko8, same thread: "I'm hoping for a mod to re-haul the UI."
- **[fetched]** leftharted, same thread, 13 Nov 2015: "tap E instead of enter."
- **[fetched]** Dionysus, same thread, 24 Dec 2018: "the Enter/E issue isn't even bad controls, just bad UI."
- **[fetched]** Celvas, Steam Pip-Boy thread, 6 Dec 2015 (https://steamcommunity.com/app/377160/discussions/0/496881136917737588): "Turns out I had two invisible items cluttering about." Weight shown on the Pip-Boy did not match visible items. Same thread, BOYCOTT S-T-E-A-M!, 24 Nov 2015: "I see two different current weights for my character." Same thread, kalebkeen, 6 Dec 2015: "I can confirm that i do in fact have a ♥♥♥♥ ton of invis items in my inventory." (censored text as returned)
- **[fetched]** Spooks, Steam workshop thread, 30 Aug 2016 (https://steamcommunity.com/app/377160/discussions/0/350540780273587833): "They're in the workshop menu and everything, they're just iconless"
- **[fetched]** Spooks, same thread, 1 Sep 2016: "we're still missing things from the workshop menu"
- **[snippet]** A 2024 thread asked why junk at a workbench cannot be broken down in bulk. A 2016 thread complained about scrapping junk one item at a time. (search summary only)
- **[fetched, Fallout 76 not Fallout 4]** See the Fallout 76 section below for the sort-order bug. It was wrongly attributed in the first search; it is a r/fo76 post.

**Praise / what works**

- **[snippet]** A Steam reply says the Pip-Boy tagging system lets you flag junk so you do not collect unneeded items. From the junk tab, you pick components to search for. (search summary, paraphrased)
- **[snippet]** Pip-Boy categories are used to group items when viewing your inventory in the Pip-Boy, as the F4TES quote above implies, but the transfer menu (R) does not keep the same categories.

**Item comparison / weight and value sort**

- Not found. No source in this run confirmed a Fallout 4 comparison-arrow or delta tooltip. The first search summary claimed one; it was not confirmed. Mark it **UNVERIFIED** if reused.
- Built-in sort: F4TES says the built-in sort helps only a little, and only by niche categories (paraphrase from the summary of the 2015 thread).

## Fallout 76 (sort, Pip-Boy ordering)

- **[fetched]** r/fo76 post, titled with "[2019]", about Pip-Boy ordering (https://devtrackers.gg/fallout/p/37d9b210-item-sort-orders-bug): "the order of items in the pip-boy randomly changing when scrapping or dropping items"
- **[fetched]** Same post, on the cause: "when the items being re-ordered all are equivalent based on the currently selected sort"
- **[fetched]** Same post, on the fix: "including a unique field at a lower priority as an input to the sort function" and the example "sorted by weight + name instead of just weight"

This is a real, specific, sortable-list lesson: when sorting by one key, add a secondary key (name, id) so equal items do not reshuffle.

## Skyrim (vanilla and SkyUI)

**Why SkyUI exists (complaints about vanilla)**

- **[snippet]** A Nexus or Steam post (source URL not resolved from search): "The favorites hotkeys are awful, they only allow you to equip one item per key, making it impossible to equip more than one weapon or armor."
- **[snippet]** Same source: "Sorting the items alphabetically was my biggest beef with the vanilla favorites menu."
- **[snippet]** Same source: "Changing my weapons in combat is so unintuitive when you want to equip two different items in your hands."
- **[snippet]** Steam user: "vanilla favourites is not very detailed and once you've got loads of favourited items it's a real pain to navigate." (https://steamcommunity.com/app/72850/discussions/0/... search result; exact URL not resolved)
- **[snippet]** Another user found the default categories too broad for a mage build: "since there's one category for magic. All magic." They wanted more categories, not only SkyUI's group boxes.

**SkyUI praise / features**

- **[snippet]** The SkyUI Special Edition Nexus page (mod 12604) describes a "powerful, categorized favourites menu" and "eight customizable hotkey groups" with an MCM configuration menu and a "feature-rich inventory panel with search and sorting." Wording comes from a search summary, not the page. **UNVERIFIED wording.** Nexus returned 403.
- **[snippet]** SkyUI's group keys are F1 to F4. Numbers handle item and spell hotkeys. Items must be favourited before they can be bound. Forum threads show new players often miss this step.
- **[snippet]** A commenter: "skyui now comes with its own favorites menu, which I find to be better than most others out there."

**SkyUI complaints (also reasons players leave it)**

- **[snippet]** A user went back to vanilla because "SkyUI is slow, and takes a few extra seconds over the vanilla menu."
- **[snippet]** Some controller users report SkyUI hotkeys and button prompts do not work reliably, and one said it does not support their DS4 controller.
- **[snippet]** Mod 175109 (a controlmap tweak) exists to make SkyUI favourites groups usable with a controller (Nexus page, 403 on fetch).

## Starfield

**Complaints**

- **[snippet]** Steam thread, September 2023: the inventory UI is poor and the weight limit adds only tedious management. Outpost containers have low capacities, so base-building becomes a chore.
- **[snippet]** A Steam reply says storage-chest carry weight cannot be changed through the console and needs a mod. Another reply says the console can raise carry weight.
- **[snippet]** One Steam thread blames weapon-swap and scanner lag on total stack count, not weight. This is an unverified theory from one player.
- **[snippet]** Kotaku (opinion; page did not load): players are "consistently confused by the menus". The main menu was called visually noisy. The page could not be fetched; treat as **UNVERIFIED** wording.

**Developer response**

- **[fetched]** Pete Hines, Bethesda head of publishing, 21 Aug 2023 (https://www.purexbox.com/news/2023/08/bethesda-exec-responds-to-unprofessional-criticism-of-starfields-menu-screen; same reply in https://www.pcgamesn.com/starfield/menu-response): "Or they designed what they wanted and that's been our menu for years and was one of the first things we settled on."
- **[fetched]** Same reply: "Questioning out a developer's 'care' because you would have done it different is highly unprofessional"
- The reply was about the main menu screen, not the inventory. It did not respond to inventory sorting or weight.

**Redesign and mods (what the community did instead)**

- **[fetched]** Gamewave (French, 6 Sep 2023, https://gamewave.fr/starfield/starfield-un-joueur-reimagine-l-interface-de-l-inventaire-bien-mieux-que-l-originale/). Original: "La publication de ce concept a récolté près de 10 000 avis positifs". Gloss: the post drew nearly 10,000 positive reactions.
- **[fetched]** Gamewave, same article. Original: "qui est principalement conçue pour que les utilisateurs consoles ne soient pas trop perdus en naviguant à la manette". Gloss: the interface is mainly designed so console users do not get lost navigating with a controller. This is a complaint about PC players being stuck with a controller-first layout.
- **[snippet]** The redesign was an r/Starfield post by u/turbokacperel, titled "I redesigned Starfield inventory UI" (per Gamewave). Prima's article (https://primagames.com/?p=173229) was not fetchable.
- **[fetched]** MP1st, Jack Breitenbach, 5 Sep 2023 (https://mp1st.com/news/starfield-ui-mod-provides-much-needed-improvements-to-inventory-system), on the StarUI Inventory mod:
  - "Lists can be sorted by properties with a simple click of the respective column"
  - "Categories now show the combined mass of all items within them"
  - "More compact view of categories and items"
  - Its listed inputs are keyboard shortcuts and mouse wheel settings. The article does not mention a controller.
- **[snippet]** Nexus mod 662 adds a sort by total mass (mass times quantity). This catches stacks of light items that add up.
- **[snippet]** ArmadaUI (Nexus 16378) is an inventory-only rebuild with controller support and a more compact view.

## Baldur's Gate 3 (inventory, camp, radial menu)

**Inventory and sorting complaints**

- **[fetched]** Ashlee Manalang, Gfinity Esports, 2 Apr 2025 (https://www.gfinityesports.com/article/why-baldurs-gate-3s-inventory-needs-a-simpler-system): "There's no decent sorting option that groups similar gear together in a logical way."
- **[fetched]** Same article: "The moment I swap someone out, I lose access to everything they were carrying."
- **[fetched]** Same article: "Even sending things to camp doesn't really solve the problem."
- **[fetched]** Same article: "tooltips take just long enough to load that it slows the whole process down."
- **[fetched]** Same article, on Steam Deck: "scrolling through tiny item icons, waiting for tooltips to load, and dealing with sluggish navigation make it even worse."
- **[snippet]** Steam thread, 2023 (BG3 discussion, handle not resolved): "Filter buttons for different categories have to be placed on the main UI interface, not be hidden behind a "Sort by..." button!" and "In this game it needs 2 mouse clicks to get the inventory sorted."
- **[snippet]** Steam thread, same era: "Items will not appear until I use a filter", and the "Show All" filter "does not actually show all the items."
- **[snippet]** Steam thread: "The code can't be called optimized when it takes next to one second to switch from one character's inventory to another one's."
- **[snippet]** Steam threads report that sorting by category does not group potions as expected, sorting the Traveler's Chest mixed swords, daggers and food, and sorting failed on some characters.
- **[snippet]** A Steam post reports a crash while sorting a large camp chest.
- **[snippet]** Steam thread, 2021, comparing Divinity: Original Sin 2: one player says that UI "is a lot better for the most part" and praises that it "automatically sorts and gives proper information on item stats." (search snippet; the "praises" wording is the summary's, so treat as paraphrase.)

**Camp and Traveler's Chest**

- Players report they cannot view a character's inventory at camp unless that character is active. **UNVERIFIED** (search summary).
- Players build their own sorting with categorised containers, and one player's camp chest needed a self-made system to stay usable. **UNVERIFIED** paraphrase of search summaries.
- No rename or item-tag feature for containers was confirmed. A 2024-era review said there is "no way to rename containers or tag items" (paraphrased in search summary, **UNVERIFIED** wording).

**Radial menu (controller)**

- **[snippet]** r/BaldursGate3 post, Sep 2023 (teddit mirror, blocked on fetch): "How to fix controller default radial headache." Search summary of the thread: slots re-arrange on their own, and "the game will swap my radial wheels around at random." Removing a radial is "unreliable" per Dec 2023 r/BaldursGate3 comment. Mirrors were blocked; treat as **UNVERIFIED** wording.
- **[snippet]** Steam post, Aug 2023: LT with the left stick, which previously opened the rest/camp wheel, opened inventory or the Reactions menu after an update. RT still opens the wheel.
- **[snippet]** Steam user on Steam Deck: disarm and rearm "ends up redesigning the radial menu layout, which defeats the muscle-memory purpose of a radial."
- **Mouse-specific radial complaints**: not found in this run.

**Larian forums**: no Larian forum thread was reachable. Nothing here reflects a Larian response.

## Patterns worth adopting

Each pattern is tagged with the complaint it answers. Items are grounded in sources above unless marked otherwise.

1. **Sort by column, with a visible control** (Starfield/StarUI; BG3 two-click complaint). Clicking a column header sorts. Keyboard: press a letter or key to cycle sort key, and show the active sort. Controller: bumper cycles sort key, one press.
2. **Secondary sort key** (FO76 devtrackers). Sort by weight then name, so equal items do not reshuffle. Applies to Cataclysm's existing sort modes too.
3. **Filter and sort controls on the main screen** (BG3 complaint, sort hidden behind a button). Put category tabs and sort on the visible header, not one extra menu deep. Keyboard: number or letter keys for tabs. Controller: shoulder buttons.
4. **Category totals** (StarUI "combined mass" per category). Show the sum of weight and count per category header, so a light stack that adds up is visible.
5. **Favourites with numbered hotkey groups** (SkyUI groups F1 to F4 and 1 to 8 for items, per search summary). Bind several items per key, so swapping between weapons is one press. Keyboard: number keys bind. Controller: radial-style or group picker.
6. **Favourites must be explicitly set before binding** (SkyUI forum complaint). If you bind from a favourites list, make that obvious at the bind point.
7. **Junk tagging** (FO4 tagging; F4TES wanted "mark the exact amount you need" and be notified). Mark junk and set target quantities. Auto-pickup of marked items only (Steam suggestion, FO4).
8. **Transfer-all and category-preserving transfer** (F4TES: categories kept in Pip-Boy, lost in transfer menu). Transfer keeps the same categories, and a single action moves all matching items.
9. **Scrollbar on long lists** (CoffeeGecko8: no scrollbar, mouse wheel only). Show a scrollbar in trade and container lists.
10. **One confirm key that matches the label** (FO4 "Enter" vs "E" thread). Label the actual key shown.
11. **Show invisible or broken item state** (FO4 invisible items, phantom weight). Count only items that exist. The Pip-Boy weight must match the visible list.
12. **Tooltip latency** (BG3 Gfinity and Steam: tooltips too slow). Tooltips should show without a delay, or cache on first hover.
13. **Stable radial layout** (BG3 radial slots reordering). Keep radial slot positions fixed across sessions and do not rebuild them on unequip or rearm.
14. **Character inventory switch without lag** (BG3 one-second switch). Switching characters should take well under a second.
15. **Multi-character access** (BG3 Gfinity: losing access on swap). Let the user see a non-active character's stash at camp, or show a shared stash view.
16. **Item comparison deltas**: no source in this run supports a specific comparison-arrow design. **UNVERIFIED**; do not treat as researched.
17. **Compact list view** (StarUI and ArmadaUI). A dense mode for many items, with a scrollable category column.

## Could not verify

- Fallout 4 Pip-Boy comparison tooltips or weight/value delta arrows. No source found. The first search summary claimed a weight-comparison tooltip; this was not confirmed.
- Reddit threads (r/fo4, r/skyrimmods, r/BaldursGate3, r/Starfield). Reddit and its mirrors were blocked or redirected to auth pages. Reddit-attributed claims are search-summary or **UNVERIFIED**.
- The text of the SkyUI Nexus page (403). Its feature list is from a search summary.
- Kotaku's Starfield article (page did not load).
- Prima's Starfield redesign article (not fetchable).
- Most Steam discussion threads for BG3, Starfield and Skyrim (content-warning gate; no posts loaded). Quotes from them are **[snippet]** only.
- Whether any complaint above was fixed in later patches. No patch notes were checked.
- Larian forum threads. None reachable.
- Whether any Bethesda or Larian team responded to inventory-specific feedback. Only the Starfield menu reply (Pete Hines, 21 Aug 2023) was confirmed, and it concerns the main menu.
- Mouse-specific BG3 radial complaints. None found.
- Fallout 4 build-mode complaints beyond the Steam threads above (missing or iconless workshop items, load lag, unallied settlements). These come from 2015 to 2019 threads and are not confirmed for current patches.
