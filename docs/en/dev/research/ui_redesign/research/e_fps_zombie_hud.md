# FPS / zombie / survival HUD conventions: research notes

Scope: first- and third-person zombie, survival and FPS games, as requested. Date of research: 2026-10-21.

## Method and caveats (read first)

- Quotes below were fetched with WebFetch, which runs the page through a small model that extracts text. Each quote was checked against the fetch summary only, not against a raw copy of the page. **Spot-check any quote before reusing it in a design doc.**
- Where a fetch was blocked (HTTP 403, redirect loops, TLS errors), the claim is marked as search-snippet only, or as UNVERIFIED.
- Most Steam and forum quotes are from threads whose handle and date were visible in the extracted text. Where no handle/date was visible, that is stated.
- Several requested sources (Game UI Database, interfaceingame.com, Can I Play That reviews for most titles, GDC talks on Dying Light, L4D2 and DRG) were not reached, or returned nothing relevant. See "Could not verify".

---

## Per-game sections

### The Long Dark (strongest source set)

Quotes (gamedeveloper.com blog, by Iuliana U, 12 Jan 2021: https://www.gamedeveloper.com/design/long-dark-survival-game-hud-ui):
- "Many Heads-Up Display elements are contextual and some can even be set to fade out over time."
- "Health zone is one of the only persistent UI elements on the screen"
- "the 5 status indicators are crucial to player survival in the game."
- "red for damage, yellow for warning, white for minor importance."
- "Numbers take more time to read, when all the player needs to understand is if a certain condition is high/low"
- "Predator attraction icon is bigger and more prominent than wind protection icon to indicate importance."

Player reaction (Steam discussion, General Discussions, 2016-2023: https://steamcommunity.com/app/305620/discussions/0/405694115206941426):
- KOTN Baby-Eating Dingo, 12 Mar 2016 9:17am: "I really hate this change."
- Scoundrel, 12 Mar 2016 2:49pm: "I like the newer version of the HUD since it only displayes when there is something for you to deal with."
- Scoundrel, 12 Mar 2016 2:49pm: "I have never felt like I had to 'spam' TAB to keep on top of my status."
- Creation, 12 Mar 2016 9:06am: "The minimalist HUD is supposed to help with the immersion i dont understand."
- ScrapyardBob, 12 Mar 2016 4:18pm: "I'd like to see a settings slider for how sensitive those icons are in the lower-left corner."
- ZzozZ, 3 May 2017 10:17am: "let the Hud Open plz or put a On/Off switch...that informations are SO important in a survival game."
- SirunSong, 4 Nov 2023 3:37pm: "This was a poor decision to place crucial information in that form."

Takeaway: the contextual-icon HUD split the player base. Fans said it reduced TAB-spamming; critics said it hid information that is critical in a survival game. The fix players asked for was a sensitivity or on/off option, not a return to the old HUD.

Affliction and condition facts (Long Dark Wiki, search snippet only, UNVERIFIED beyond snippet): frostbite reduces max Condition by 10% per application (up to 5); burns reduce max Condition by 15% temporarily; one archived page says 75% for burns. The sources conflict.

### Dying Light 2 (Techland)

Quotes:
- Techland UI producer, as reported by VideoGamer (Ben Borthwick, page "Last Updated December 21, 2021": https://www.videogamer.com/news/dying-light-2-stay-human-developers-talk-ui-and-accessibility-in-latest-video/): "you'll be able to switch on and off many HUD elements as you see fit."
- NME, Georgina Young, 22 Dec 2021 (https://www.nme.com/news/dying-light-2-will-have-accessibility-options-and-a-fully-customisable-hud-3125013): players complained that HUD elements, including "health bars and hints, were constantly visible," and Techland's response was to give players "the option to remove them."
- NME (same article): the developer's stated position is that "user preference comes first."
- Can I Play That, Ben Bayliss, 4 Feb 2022 (https://caniplaythat.com/2022/02/04/dying-light-2-stay-human-to-receive-accessibility-improvements/): "perhaps some button choices for activating Survivor Sense" could be added; a console bug where Survivor Sense did not trigger on press is noted.
- Techland dev blog #9, dated 2024-04-15 (https://outpost.dyinglightgame.com/news/dev-blog-9, reached via redirect): "the stamina management system, which we called Tactical Stamina"; "it also gets restored more slowly"; Nightmare Mode "removes the majority of HUD elements"; players should "rely on your eyes and ears, instead of checking the UI markers".

Stamina design intent (Multiplayer.it, Italian, search snippet only, UNVERIFIED): a 2018 interview said stamina should matter during chases and high-pressure areas, not for crossing the city.

Weapon durability (search results, guides, not fetched in full): durability carries over from DL1; scrap weapons break quickly; mods restore durability (about 50 per mod). Guides disagree on whether weapons can be repaired. Treat the mod-based account as the better-supported one.

Survivor Sense: no source found that explains the mechanic in detail. A GGRecon guide titled "Dying Light 2 Survivor Sense Not Working" suggests a known bug. Contents not verified.

Player-sentiment gap: no Reddit or Steam thread about DL2 stamina or Survivor Sense was reached.

### Left 4 Dead 2

No official Valve design source or GDC talk was reached. Evidence is from Steam Workshop HUD mods (community, not Valve):
- Commenter on an avHUD 2 page: the temp-health display "made how much temp health I have easier to read at a glance." (Steam Workshop, commenter handle not verified in the fetch.)
- Changelog (avHUD 2): "fix for green temp health not being animated"; blinking when low, faster when critical.
- Requests: vertical health bar, an option to hide the numeric HP. (Steam Workshop comments, search snippet.)
- Modder note: "All the HUDs you see in the workshop have the same base! Only the sprites (images) and their placements on the screen are modified." (search snippet, Steam Workshop.)

These show what players tweak, not what Valve intended. Teammate outline and prompt conventions for L4D2 are UNVERIFIED (memory only).

### Dead Island 2

- Zombie health bars can be turned off; PCInvasion/Prima guide says this "adds more immersion" (search snippet; article fetch returned 403/redirect loop). UNVERIFIED for verbatim wording.
- UI preset "Dynamic" hides map status icons such as health and reticle until a weapon is drawn or the player is damaged (Dot Esports, search snippet; fetch returned 403).
- No Reddit complaint thread about the HUD was reached.

### The Division (2016) and Resurgence (2026)

Quotes (HUDs and Guis, Jono Yuen, 26 Jun 2013: https://www.hudsandguis.com/home/2013/06/26/tom-clancys-the-division-interface-design):
- "instead of framing the screen with game information, the UI is incorporated into the environment"
- "Everything that carries the story along looks like it plays out in the game's environment."
- The wrist UI "features a carousel type UI that wraps around the character's wrist"; the only criticism: "I'm not sure how the character scrolls through different options though"

NeoGAF commenter (search snippet, not fetched; handle not visible): the AR concept was promising but the execution was "overblown and distracting."

Resurgence (2026, Steam): reviewers called the UI "the most distracting, convoluted, overcomplicated on purpose" (search snippet, PC Gamer/Player One). Most negative reviews cited monetisation and performance, not only the UI. UNVERIFIED as a UI-only complaint.

### Deep Rock Galactic

Quotes (Steam discussion 3057363335741893276, 25 Jun 2021: https://steamcommunity.com/app/548430/discussions/3/3057363335741893276):
- Eri (Nuwa_Eri), topic opener: "Like more specific pings"
- Eri, 2:01am: "I can't assume that everyone playing the game would be able to understand the signal"
- Fact Got Ferret, 1:50am, quoting Femboy Thighs: "The pointer works BECAUSE it's simple."
- Fact Got Ferret, 1:50am: "I think a couple ping options would be nice. We don't need a whole lot."
- BezaoBuilder, 9:27am: "just pinging the usual way will make a generic ping, but you have options for more specific pings"
- BottomScorer (Banned), 5:37pm: "I need commands like "watch this way", "be around this spot", "cover me"."
- BottomScorer, 5:35pm: "Player icons are quite small, especially from a distance."
- Chibbity, 5:41pm: "No you don't." followed by "K.I.S.S."

Takeaway: the debate is simple generic ping versus more command types. The simple ping works because most pingable things have one obvious meaning. The thread did not mention a radial wheel. Terrain scanner and radial-wheel design: UNVERIFIED (not found).

### Escape from Tarkov

Quotes (Shacknews, Timo Reinecke, 30 Aug 2024: https://www.shacknews.com/article/141231/tarkov-healing):
- "Unlike other games, Escape from Tarkov distributes your character's HP to every limb."
- "Each limb has to be patched up individually and can get into various states of distress depending on the injury."
- "If your head or thorax reaches 0 health you'll drop dead instantly while your arms and legs can be "blacked out"."
- "A blacked-out limb loses all function and will affect the way you walk, run, and aim until it's addressed."

Limb UI: the article does not describe the on-screen display of a blacked-out limb. One guide (AFKGaming, search snippet) says the limb "turns black on the UI." UNVERIFIED beyond snippet. Reddit feedback on the limb UI: not found.

### Days Gone

- No official option to hide the whole HUD (search results). A community "Toggle HUD" mod on Nexus Mods exists; it also hides menus, prompts and subtitles. The author says the weapon wheel had to be bound separately. (Nexus, search snippet.)
- Official accessibility page (Bend Studio, fetch returned 403): persistent centre dot and a High Contrast HUD option, per search snippet. UNVERIFIED wording.
- Nexus forum user (search snippet): asked for finer control, especially over the quest marker.

### Metro Exodus

- Hard difficulty disables the HUD; the watch shows time and radiation information (search snippet, article). UNVERIFIED wording.
- Lemmy post (lemmy.dayl.in/post/140428/1274003; fetch failed with TLS error): describes the game as a strong no-HUD example because the map and filter timer are shown through diegetic elements. Quote not verified.

### Resident Evil 4 (2023 remake)

Quote (Steam discussion 3827536762640406899, Shapely Twig, 25 Mar 2023: https://steamcommunity.com/app/2050650/discussions/0/3827536762640406899):
- "I also despise the generic crosshairs introduced with the RE2 Remake but I simply disable the HUD to remedy that."
- Reply, 10:21pm: "the removal of the shakey laser aim"

Other points (search snippets): the attaché-case grid inventory is kept and is treated as a weapon-choice constraint; the demo let players toggle the HUD and the red damage borders.

### STALKER 2 (Heart of Chornobyl)

Quotes (Steam discussion 4626981579416274293, Mormoloc, 1 Dec 2024: https://steamcommunity.com/app/1643320/discussions/0/4626981579416274293):
- "Annoying stash. I mean... the inventory is small enough , but the stash is stupid."
- "If you get too close to an NPC you cannot talk to them as the F menu dissapears."
- "a black screen would do." (on the death screen overlay)

Other points (search snippets, not verbatim): a developer said the HUD can be hidden entirely or individual elements switched off. A user said hit markers and a compass full of arrows and quest PoIs "kind of kills the atmosphere." Fans asked for a hardcore mode with no HUD or hints; GSC has not ruled it out.

### State of Decay 2

- Reviewers praised the menus (Easy Allies, Game Informer, search snippets). One reviewer reported the HUD flashing in and out, making text unreadable and the mini-map useless (2017-2018 launch coverage).
- Fatigue and stamina: Asian Age (search snippet) called fatigue "probably more infuriating." Stevivor explained that caffeine restores the stamina pool.
- Verbatim fetches were not done. Treat as UNVERIFIED beyond snippet.

### Cross-game UI theory (usable background)

Quotes (Game Developer, "Game UI Discoveries: What Players Want," Marcus Andrews, 23 Feb 2010: https://www.gamedeveloper.com/design/game-ui-discoveries-what-players-want):
- A customisable interface "seems to greatly increase the tolerability of traditional 2D HUD elements even when the screen gets cluttered." (about WoW)
- "when given the choice, players want more information and more UI to aid them reach optimal play performance."
- Intrusive non-diegetic interfaces are "tolerated and even appreciated if they aid the player to perform his task."
- Far Cry 2's fading HUD creates "a complete lack of control for the player."

Hit markers (PC Gamer, "Hit markers are an FPS crutch," https://www.pcgamer.com/games/fps/hit-markers-are-an-fps-crutch/): the headline was verified by fetch, but the article was truncated and the argument was not. Snippet-level claim: hit markers are useful in PvP but can become the only cue players watch; the author prefers impact animations and wound effects. Author and date not visible. UNVERIFIED beyond headline.

---

## Patterns worth adopting

Mechanics described as they appear in sources; each is tied to the evidence above.

1. **Contextual, fading HUD with an option to keep it.** The Long Dark's icons show only when they need attention and can fade over time. Players praised this, and others asked for a sensitivity slider or an on/off switch. Ship a default that is contextual plus a persistent-mode option. (Long Dark sources; DL2 NME.)
2. **Icon-first status, with numbers only on demand.** The Long Dark article argues numbers take longer to read than a high/low icon. Keep numbers in a detail view (TAB-style) rather than on the main HUD. (Long Dark gamedeveloper article.)
3. **Colour and size encode priority.** Red for damage, yellow for warning, white for minor; larger icon for more important status. (Long Dark article.) Relevant to a status strip.
4. **Persistent minimum set.** The Long Dark article calls the health zone "one of the only persistent UI elements." The rest appears when needed. (Long Dark article.)
5. **Stamina as a risk, not just a bar.** Dying Light 2's Tactical Stamina: running out makes the restore slower, so depletion is costly. Show the stamina bar only when it is being used or is low. (Dev blog #9; Dead Island 2 "Dynamic" preset, snippet.)
6. **Dynamic UI preset.** Dead Island 2's preset hides map status icons until a weapon is drawn or the player takes damage (Dot Esports snippet). This is the same idea as 1 and 5.
7. **Full HUD off, with prompts kept.** RE4, STALKER 2 and Metro Exodus all allow hiding the HUD. The Days Gone mod that hides everything also hides prompts and subtitles, which players found a problem. Keep interaction prompts and subtitles when the rest is hidden.
8. **Generic ping with optional specific commands.** The DRG thread supports one generic ping plus a few specific commands ("watch this way", "cover me"). The ping works because most targets have one clear meaning. (DRG Steam thread.)
9. **Limb-level health as a body state, not a number.** Tarkov spreads HP across limbs; a blacked-out limb loses function (walk, run, aim). Show function loss in the model, not only the number. Verify the on-screen display before copying (see Could not verify).
10. **Diegetic information in the world.** The Division's world-anchored UI and wrist carousel were praised in 2013. The complaint was the wrist controls. Keep the wrist or in-world information with simple controls. (HUDs and Guis.)
11. **Hold-to-confirm with one radial fill.** Unity's input docs describe a radial hold indicator that resets on early release. Use one progress signal, not two competing rings. (Unity docs URL https://docs.unity3d.com/Packages/com.unity.inputsystem@0.2/manual/Interactions.html is an old 0.2 package doc; the pattern is standard but the source is dated.) Motion UI guidance: "one fill, no competing ring - the sole progress signal" (search snippet, web-UI context).
12. **Hit feedback: cue, not crutch.** PC Gamer argues hit markers can become the only cue. Prefer target reactions and impact effects alongside any hit marker. (Snippet-level; UNVERIFIED beyond headline.)
13. **Directional damage indicator.** Common implementation: a screen-edge indicator that points toward the attacker, relative to the camera's facing. Sources are implementation guides and mod pages, not design studies. (Unreal forum, Unity guide, search snippets.)
14. **Customisation that is explicit.** Dying Light 2 said "user preference comes first" and added HUD toggles after complaints. Build the toggles in from the start, rather than adding them after complaints. (NME.)
15. **Accessibility toggles that reduce visual noise.** Days Gone's persistent centre dot and high-contrast HUD option (snippet, unverified wording). Captions size and colour options also appear in DL2's accessibility list (VideoGamer).

---

## Could not verify

- **Game UI Database (gameuidatabase.com) and interfaceingame.com**: not reached in the searches. No HUD screenshots or annotations used.
- **Can I Play That**: only the Dying Light 2 page was reached. No reviews checked for other titles.
- **GDC or developer talks**: none found for Dying Light, L4D2, DRG (terrain scanner, radial wheel), or Dead Island 2. The DRG radial wheel is not sourced.
- **Dying Light 2 Survivor Sense**: no mechanic explanation found; the only item is a bug-guide title.
- **Dying Light 2 stamina and durability complaints on Reddit or Steam**: none reached.
- **Dead Island 2 Reddit complaints**: not found; Dot Esports and PCInvasion/Prima fetches returned 403 or redirect loops.
- **Days Gone Bend accessibility page and Reddit threads**: fetch returned 403. Option names are from search snippets.
- **Metro Exodus Lemmy post**: fetch failed with a TLS error. The no-HUD claims are from search snippets.
- **Escape from Tarkov limb UI**: the on-screen appearance of a blacked-out limb and Reddit limb-UI feedback are not verified.
- **L4D2 official teammate outlines, prompts and temp-health rules**: no official source reached. Memory only, UNVERIFIED.
- **State of Decay 2 and STALKER 2 HUD reviews**: no full-text fetch; only snippets.
- **Hit markers, compass strip, objective distance markers, minimap vs compass debate, limb health diagrams, HUD auto-hide options across games**: only the general sources above touch these. Not sourced per game.
- **The Division Resurgence reviews**: attribution to UI alone is not established; the same reporting cites monetisation and performance.
- **Quote count**: 25+ quotes are listed above with URLs and handle or date where visible. Each was taken from a WebFetch extraction and should be spot-checked.
