# Survival games: needs, health, status effects, morale, map and journal

Research date: 2026-10-21. Sources were searched and fetched in this session. Quotes marked **[fetched]** were read from the page text (verbatim as returned by the fetch tool). Quotes marked **[snippet]** come only from search-result summaries and were not checked against the page. Claims marked **UNVERIFIED** are from memory or unconfirmed summaries.

Handles and dates are as shown on the source. Where a fetch tool returned a paraphrase, that is stated.

## Method and limits

- Forum and Steam threads are anecdotal. Coverage is uneven: several games returned only guides or wiki pages.
- Several fetches failed: the Long Dark affliction wiki (HTTP 402), a Zomboid Steam workshop comment page (HTTP 429), the Project Zomboid Build 41 dev blog (HTTP 403), and the Elden Ring PC Gamer article (no body returned).
- The Valheim forum fetch returned a paraphrase, not verbatim text.

---

## 1. Needs and health

### The Long Dark
- **Shows:** five need/condition bars (warmth, fatigue, thirst, hunger, condition) with rising/falling indicators. Source: Iuliana U, Game Developer blog, 12 Jan 2021. [fetched]
  - "Warmth, fatigue, thirst, hunger and condition along with rising/falling indicators are displayed here."
  - "Notice the lack of numbers that we are accustomed to seeing with bar UI elements."
  - "Numbers take more time to read, when all the player needs to understand is if a certain condition is high/low"
  - URL: https://www.gamedeveloper.com/design/long-dark-survival-game-hud-ui
- **Critique in same article:**
  - "there is no increase/decrease indication on the condition (health bar)"
  - "Drawing player's attention to decreasing health will help insure there are no frustrated surprise deaths."
  - "Aim for better user experience and usability over UI aesthetics"
  - "This creates a unique challenge for the UI, since it must be legible on both extremely bright and dark surfaces."
- **Afflictions (wiki, search summary only [snippet]):** positive "Benefits" shown green, negative shown yellow/red; 39 negative types. The wiki page itself returned HTTP 402, so these figures are **UNVERIFIED** against the page. URL: https://thelongdark.fandom.com/wiki/Afflictions
- **Player-reported UI issue [snippet]:** a 2018 Steam thread on removed treatment-selection UI, and Update 2.19 fixing a stamina bar stuck on HUD. URL for patch: https://mp1st.com/news/the-long-dark-update-2-19-for-june-29-brings-fixes-to-consoles
- **Could not verify:** a current official UI review or redesign plan.

### Project Zomboid (moodles, health)
- **Moodle readability complaints.** Steam thread, Dec 2024 (B42 moodles). [fetched] URL: https://steamcommunity.com/app/108600/discussions/0/595135660380130920
  - Alabeo, 22 Dec 2024, 11:01pm: "These moodlets are basically unreadable at first glance and you have to constantly double check what they mean."
  - Falmingkitter, 22 Dec 2024, 10:46pm: "the new moodles are cartoonish, undetailed, use annoyingly bright colours, and just lack soul"
  - penguiin, 17 Dec 2024, 5:09pm: "for me only the face ones are really bad. they look like really dated like some old flip phone emoji pack"
  - papi is here, 17 Dec 2024, 5:23pm (praise for severity cue): "I think some are more informative like the new over-encumbered"
  - Puro, 17 Dec 2024, 5:05pm: "Most peoples gripes are that they don't match the artstyle, and lose the originals graphic-ness."
  - Evensong, 22 Dec 2024, 11:49pm: "Realistically, when any game has a artstyle shift you have to give it a long time before any feed back is useful"
- **Health/injury panel:** the first 15 of 21 comments in the moodle thread did not mention it. Body-diagram and injury list **not verified** in this session.
- **Build 41 map annotations:** the developer blog (projectzomboid.com/blog/?p=7152) returned HTTP 403. Only a search summary is available: survivor maps "show the way to loot stashes" and some uniformed zombies act as a breadcrumb trail. **UNVERIFIED** beyond that summary.

### Green Hell (watch)
- **Watch display [snippet, GameSkinny guide]:** four colour-coded bars: protein (red), carbohydrates (yellow), fats (green), water (blue). Gauges fall with energy use. URL: https://www.gameskinny.com/azx3e/green-hell-beginner-tips-and-tricks
- **Player criticism [snippet]:** a 2018 Steam thread argues that lack of nutrients should cause debuffs rather than slow death. Opinion, not a review. URL: https://steamcommunity.com/app/815370/discussions/0/3111392579681367937 (URL from search results; not fetched).
- **Could not verify:** a dedicated UI critique of the watch. **UNVERIFIED:** TrueAchievements' "25% of total health" per nutrient claim.

### DayZ (status icons)
- Normal Gaming, 9 Nov 2018, Steam discussion. [fetched] URL: https://steamcommunity.com/app/221100/discussions/3/1752358461530962954
  - "The status icons in the bottom right do not give very good feedback."
  - "Food,Drink, and Health icons should all change to lightgreen and green when healthy, energized, and hydrated."
  - "There is currently no feedback to let you know when you feel full either."
  - Finn, 9 Nov 2018: "is should be more dynamic"
  - 150ms, 10 Nov 2018: "Someone has made a mod for this in the workshop." / "But I agree, it should be standard."
- **Arrows [snippet]:** a 2023 mod changelog re-adds arrows for speed gain/loss and colours temperature icons. Mod, not official. URL: https://steamcommunity.com/sharedfiles/filedetails/changelog/2922873514?l=schinese (not fetched).

### Kenshi (limb health)
- Steam thread, 2 Feb 2021 (player discussion of limb loss). [fetched] URL: https://steamcommunity.com/app/233860/discussions/0/3111393214050281326
  - Mecha-Kanye, 2 Feb 2021 1:25pm: "You can lose maybe 2 limbs before the blood loss becomes a threat."
  - lizardmang, 2 Feb 2021 4:19pm: "once a limb goes negative, it actually becomes much less likely to be hit."
  - Hatsune Neko Gaming, 2 Feb 2021 8:22am: "Over all there really is no way with out risking death." / "But using the peeler machine with armour helps."
  - Derek Withers, 3 Feb 2021 1:02pm: "When you heal a character after a fight it works from the head downwards."
- **Assessment:** players read the limb system as a tactical resource (deliberate amputation for augments), not a health UI. No thread discussed the limb display itself. **Health display: not found.**

### Escape from Tarkov (body parts)
- Search summaries only. Body-part HP for head/thorax, each arm, stomach, legs (Wikipedia summary). Limb at 0 is blacked out and standard medkits no longer work. [snippet] AFK Gaming: "Once your limb HP reaches 0, you won't be able to restore it using standard medkits." URL: https://afkgaming.com/esports/guide/how-to-deal-with-blacked-out-limbs-in-escape-from-tarkov
- **Could not verify:** player opinion on the body-part UI. The Tarkov guide claim that the head has 35 HP is from one source (iota.msp.org) **UNVERIFIED**.

### Subnautica (food/water/O2 and beacons)
- Steam thread, 12 Feb 2018. [fetched; Mansen, ImHelping, Demon of Razgriz] URL: https://steamcommunity.com/app/264710/discussions/0/1700542332338086018
  - Mansen, 12 Feb 2018 6:44pm: "The only real thing missing is a proper map that shows you where you've been roughly."
  - Mansen, 12 Feb 2018 6:39pm: "Heck your first visit into an Alien "base" throws you pinpointed location data right into your tablet."
  - ImHelping, 12 Feb 2018 6:11pm: "They should learn landmarks in a game with next to no draw distance!"
  - Demon of Razgriz, 12 Feb 2018 7:06pm: "Oh wait, you can't even do that because you can't get a map."
- **Snippet:** Below Zero players reported beacon dependence; one reported needing 100+ beacons; others praised beacons as immersive. Survival-stat numbers (food/water/O2 in the HUD) were **not** checked in this session.

### Rust (vitals bar bottom-right), Don't Starve, Stranded Deep, The Sims, Frostpunk, This War of Mine
- **Could not verify.** No search or fetch was run for these titles. Anything I might recall about their specific HUD is **UNVERIFIED** and is omitted.

### Darkest Dungeon (stress)
- Search summary only. Store page frames stress as central: "The Affliction System" with quirks. No stress-UI player critique was found beyond one GOG review calling "the worst UI I've ever seen in a video game" (target unspecified). URL: https://www.gog.com/en/game/darkest_dungeon?page=46 (search result). **UNVERIFIED** as a UI finding.

---

## 2. Morale and mood reasons

### RimWorld (mood breakdown)
- Steam thread, 22 Jul 2017. [fetched] URL: https://steamcommunity.com/app/294100/discussions/0/1458455461488631091
  - Rous, 22 Jul 2017 10:04pm: "Moods should tell you a breakdown (Pun intended!) of things that you can try to fix, to keep their mood up."
  - GuessMDK, 22 Jul 2017 10:07pm: "They stack and will kill your colony."
  - Swirler, 22 Jul 2017 10:16pm: "The mood thing is kind of a combination of factors that amounts to mood breaks and wanders/binges that can cause a death spiral."
- **Mods [snippet]:** Moody (colonist mood dashboard), Color-coded mood bar, Owl's Colonist Bar (mouse-over grievances for break risk). URL: https://ludeon.com/forums/index.php?msg=289469
- **Verdict:** breakdown-as-list is the praised pattern. Vanilla tooltip complaints were **not** found.

### Cataclysm: Dark Days Ahead (morale)
- Discourse thread "The Jank That Is Morale", VIVIT, Feb 2019. [fetched] URL: https://discourse.cataclysmdda.org/t/the-jank-that-is-morale/18640
  - VIVIT, 1 Feb 2019 3:41pm: "I think we can all agree that the morale mechanics in Cataclysm are handled pretty awkwardly."
  - VIVIT, 1 Feb 2019 3:41pm: "The biggest problem I see is the way morale modifiers stack, both positive and negative."
  - TheZoneWizard, 4 Feb 2019 3:52pm: "I like to know why my toon is happy/sad."
  - VIVIT, 3 Feb 2019 8:43pm: "A flat cap is much more opaque, since until you've reached it, it might as well not even be there."
  - kevin.granade, 2 Feb 2019 5:10am: "There's already a cap on maximum morale bonus or penalty from a single kind of source"
- **Note:** the paraphrased dates in the fetch show VIVIT's opening post on 1 Feb; the fetch also listed 1 Feb 2019 for the first quote. Treat the exact date as reported by the fetch tool.
- **Verdict:** players want a reason list (TheZoneWizard), but capping hides the effect of stacking (VIVIT). Tension between transparency and caps.

---

## 3. Map, markers and journal

### Valheim (shared map pins)
- Steam thread by INC3NSUS, 11 Feb 2023, "Cartog table.. my least favorite implement that could have been awesome". **Paraphrased by the fetch tool, not verbatim.** URL: https://steamcommunity.com/app/892970/discussions/0/3763356482615063697
  - Complaint: pins transfer through the cartography table; players want pin toggles (all / own / one player) and separate personal and shared markers.
  - One player reported about 1,000 pins and did not want others to see them.
  - A suggestion: shared markers appear automatically to nearby players (~200m).
- **Snippet (Steam, 2024):** copying another player's map data brought back pins they had deleted. URL: https://steamcommunity.com/app/892970/discussions/0/3154202776541089289 (not fetched).
- Mods (Thunderstore "One Map to Rule Them All", "Player Map Layers") fill the gap. URLs: https://thunderstore.io/c/valheim/p/DrummerCraig/OneMapToRuleThemAll/v/1.2.14/ and https://thunderstore.io/c/valheim/p/Zephyrfit/PlayerMapLayers/ (search results, not fetched).

### The Witcher (journal) — NOTE: the praise below is for the first game, not The Witcher 3
- GameBanshee preview, BuckGB, 5 Sep 2007. [fetched] URL: https://www.gamebanshee.com/9w8a
  - "the game sports one of the most impressive journal systems I've seen in a video game to date."
  - "It's also linked to the gameplay, in that missing journal entries may keep you from taking certain actions."
  - "if you haven't learned anything about the Bloedzuiger monster, then you won't be able to accept any witcher contracts that deal with slaying such beasts."
- Pattern worth noting: a journal that gates actions (you need the knowledge to take the contract).
- **The Witcher 3 journal:** no praise piece for its quest log was found. Search results mentioned a separate J-key quest log and a lore catalogue (Quarter to Three). **Could not verify** that Witcher 3's quest log is praised.

### Skyrim / Elder Scrolls (quest markers)
- Search summaries only. Players report floating quest markers disappearing while map markers stay, and missing markers for Miscellaneous quests. URL: https://steamcommunity.com/app/489830/discussions/0/282992646967950443 (search result, not fetched). These threads are mostly 2016 Special Edition era.
- **Quest log praise/complaint on Fallout:** not checked.

### Subnautica (beacons, see section 1)
- Player split: beacons as immersion (Mansen: "The only real thing missing is a proper map...") vs tedium. See section 1 for quotes.

### Elden Ring (map markers)
- Steam thread, March 2022: request to raise the map-marker limit from 100. **Search summary, quoted from the summary, not the page:** "Love the game. Please increase the map marker limit (or remove the limit entirely)." URL: https://steamcommunity.com/app/1245620/discussions/0/3183486955459919672
- Destructoid: beacons pile up on the compass; the game can wipe all markers at once but cannot remove all beacons in one step; community workaround exists. URL: https://vip-develop.destructoid.com/?p=312316 (search result).
- NPC markers (patch 1.03): Digital Trends says the change was a quality-of-life update "met with discourse and ridicule." URL: https://www.digitaltrends.com/?p=3019945 (search result). PC Gamer piece on this could not be fetched.

### Outer Wilds (ship log)
- Cubed3 review by Michael McCann, dated 31 Jan 2024 (as stated by the fetch). The review body does not discuss the Ship Log. Reader comment by Riebeck, "1 year ago": "Did you even use the ship log?" [fetched] URL: https://www.cubed3.com/games/reviews/nintendo-switch/outer-wilds
- Search summary, unfetched: "information gathered each loop persists permanently in the Ship Log". **UNVERIFIED** source.
- **Could not verify:** critic praise specific to the Ship Log. The praise found is for the game overall (BAFTA 2020, Best Game Design).

### Return of the Obra Dinn (journal)
- Cubed3 review, Michael McCann, 9 Nov 2019. [fetched] URL: https://cubed3.com/review/5687/1/return-of-the-obra-dinn-nintendo-switch.html
  - "The journal, an incomplete report of the events during this lost voyage,"
  - "The organisation and design of everything here in the journal is clearly something that has been highly considered."
  - "ascertaining the identity and fate of everyone aboard"
- jmduke blog, dated 2022/12/24 as reported by the fetch tool. URL: https://www.jmduke.com/posts/return-of-the-obra-dinn.html
  - "At it's best, playing Obra Dinn feels almost like a Sudoku — you notice a detail you overlooked which unlocks an identity which unlocks two more, and so on."
  - "I am a sucker for any game that makes me take out pen and paper to solve it."
- **Could not verify:** that players praise the journal over Disco Elysium's Thought Cabinet. No direct comparison source was found.

### Disco Elysium (Thought Cabinet)
- Wikipedia summary only: thoughts are "internalized" over in-game hours; benefits with occasional negatives. URL: https://en.wikipedia.org/wiki/Disco_Elysium (not fetched). **Player reception: not verified.**

### Project Zomboid map (Build 41)
- See section 1. Map annotation design **UNVERIFIED** (dev blog returned 403).

### 7 Days to Die (waypoints), RimWorld history tab
- **Not checked** in this session.

---

## Patterns worth adopting

Each is tied to a source above. "Adopt" means a candidate for this game's UI, not a decision.

1. **Show a trend, not just a level.** Long Dark (no increase/decrease on condition, "surprise deaths") and DayZ (Normal Gaming: "should be more dynamic"). Mechanic: a small up/down/steady glyph next to each need, computed from the last few turns.
2. **Icon + level, with a readable severity cue.** Zomboid's icons are "basically unreadable at first glance" (Alabeo), but the severity cue (over-encumbered) worked for some. Mechanic: a fixed icon set with one severity ladder (colour + shape), plus a tooltip giving the number.
3. **Bars for "is it high or low", numbers on demand.** Long Dark: numbers "take more time to read." Mechanic: bars by default, exact value on hover or on a detail screen.
4. **A reason list for morale and mood.** RimWorld (Rous: "Moods should tell you a breakdown...") and CDDA (TheZoneWizard: "I like to know why my toon is happy/sad."). Mechanic: a sum-by-source list ("-8 cold, +5 fine meal") rather than a single number.
5. **Be careful with caps.** CDDA VIVIT: "A flat cap is much more opaque, since until you've reached it, it might as well not even be there." Mechanic: if a cap exists, show when it is hit.
6. **Gate knowledge behind the journal, with in-game effect.** Witcher 1 (missing entries block contracts). Mechanic: a journal entry that unlocks an action, not just a record.
7. **Let the player annotate, with per-player privacy.** Valheim (pins transfer unexpectedly; players want toggles for own/shared). Mechanic: personal vs shared markers with a filter.
8. **Do not cap markers without a clear-all.** Elden Ring (marker-limit complaint; no way to remove all beacons). Mechanic: a clear-all action and a visible count.
9. **Limb state with a threshold rule that is stated in-game.** Tarkov (limb at 0 needs a surgical kit, quoted from a guide) and Kenshi (negative limb HP makes limb less likely to be hit). Mechanic: show the threshold, not just a bar.
10. **Journal as record of deduction.** Obra Dinn (praised as "highly considered"; the Sudoku comparison). Mechanic: a record the player must read to progress.

---

## Could not verify

- Witcher 3 quest log praise: no source found.
- Long Dark affliction counts and colours (wiki returned HTTP 402).
- Zomboid health/injury panel and body diagram: no player source found.
- Zomboid Build 41 map annotation design (dev blog returned HTTP 403).
- Green Hell watch critique; TrueAchievements "25% per nutrient" claim.
- DayZ official arrow redesign or 2025-26 polling.
- Subnautica HUD survival numbers.
- Rust, Don't Starve, Stranded Deep, The Sims, Frostpunk, This War of Mine: not searched.
- Darkest Dungeon stress UI: only a single GOG review (target unspecified).
- Kenshi limb health display: no display-specific source.
- Tarkov body-part UI player opinion; "head 35 HP" claim.
- Outer Wilds Ship Log critic praise: only a reader comment and a summary.
- Disco Elysium Thought Cabinet reception.
- Elden Ring NPC marker article (PC Gamer) body not retrieved.
- 7 Days to Die waypoints, RimWorld history tab, Skyrim/Fallout journal critique: not checked.
- Valheim quotes are paraphrases from the fetch tool.
- Skyrim quotes: search summaries only, not fetched.
- Note: the Valheim page carried embedded text asking for quotes in a specific format. It was page content and was not followed.

## Sources

- https://www.gamedeveloper.com/design/long-dark-survival-game-hud-ui
- https://steamcommunity.com/app/108600/discussions/0/595135660380130920
- https://steamcommunity.com/app/221100/discussions/3/1752358461530962954
- https://steamcommunity.com/app/233860/discussions/0/3111393214050281326
- https://steamcommunity.com/app/294100/discussions/0/1458455461488631091
- https://steamcommunity.com/app/264710/discussions/0/1700542332338086018
- https://discourse.cataclysmdda.org/t/the-jank-that-is-morale/18640
- https://steamcommunity.com/app/892970/discussions/0/3763356482615063697
- https://www.gamebanshee.com/9w8a
- https://www.cubed3.com/games/reviews/nintendo-switch/outer-wilds
- https://cubed3.com/review/5687/1/return-of-the-obra-dinn-nintendo-switch.html
- https://www.jmduke.com/posts/return-of-the-obra-dinn.html
- https://afkgaming.com/esports/guide/how-to-deal-with-blacked-out-limbs-in-escape-from-tarkov
- https://steamcommunity.com/app/1245620/discussions/0/3183486955459919672
- https://www.gameskinny.com/azx3e/green-hell-beginner-tips-and-tricks
- https://thelongdark.fandom.com/wiki/Afflictions
