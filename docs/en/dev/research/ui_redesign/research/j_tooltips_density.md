# Tooltips, item comparison and information density in games

Research note for the UI redesign. Scope: how games with complex items present tooltips, comparisons and stat density, with sourced quotes where I could fetch the text.

Verification legend: **[F]** = fetched page, quote checked against page text. **[S]** = appears only in search-result snippet, not fetched; treat as unverified wording. **UNVERIFIED** = memory or a search summary with no source text checked.

Coverage caveat: the search tool returned mostly secondary coverage and few primary forum threads. Reddit was not reachable through the tools used. Many games in scope have no sourced finding below; see "Could not verify".

---

## Per-game notes

### Diablo 4
- Setting: "Advanced Tooltip Compare" in the Gameplay tab. Shift (PC) / Y (Xbox) / Triangle (PS) while an unequipped item is selected. [F] Den of Geek (2023-06-06) says the setting lets you "compare every stat and bonus on a piece of gear", used "while you have an unequipped item selected".
- Separate "Advanced Tooltip Info" setting exposes roll ranges (low/average/max). [S] Icy Veins snippet; the page returned 403 on fetch, so the wording is unverified.
- Colour coding: the search summary says gains are green and losses red. The fetched Den of Geek article does **not** mention colours. Treat green/red as **UNVERIFIED**.
- Delta arrows: no source found. **UNVERIFIED / not found.**

### Path of Exile (PoE 1 and PoE 2)
- PoE 1 official help page: holding Alt over an inventory item shows it against equipped gear, and also explains complex or obscure terms. [F] poedb mirror of the official Helpful Tips page.
- PoE 2: Alt (or R3) reveals descriptions for underlined keywords; clicking pins the box so further terms can be hovered. [S] Steam discussion 596260925367969924 (snippet only).
- PoE 2 bugs: Alt-hover tooltip chaining stops after the second layer after the Third Edict patch. [S] official forum bug report, snippet only.
- Player complaint: holding Alt during Chaos rolling now shows modifier detail, disrupting crafting, and players want that detail moved to another key. [S] forum thread (March 2026, per snippet).

### World of Warcraft (Pawn addon and native tooltips)
- Pawn computes a numeric score from user-set stat weights, ships with Wowhead-derived defaults, and shows an arrow on the icon and tooltip when an item is an upgrade. [S] Pawn CurseForge changelog and an external article, both as snippets.
- Recent breakage: CurseForge changelog notes that Pawn 2.13.2 disabled item-level upgrade features, 2.13.3 ignored "currently equipped" tooltips because they triggered tooltip bugs under WoW 12.0 combat-addon restrictions, and 2.13.9 (dated 2026-04-21) restored scores on equipped tooltips. [S] Snippet-level summary; not fetched.
- Lesson: a comparison addon that depends on tooltip hooks is fragile when the host restricts tooltip access.

### Baldur's Gate 3 and Divinity
- No sourced finding on equipment comparison or shift-to-compare behaviour. A community "Better Tooltips" mod adds item category labels and readable lore text. [S] PCGamesN article snippet.
- An expanded-tooltip mode (Alt, inventory only) is described by Alcasthq. [S] snippet.
- Spell save type appears in the tooltip only after the spell is learnt, not during level-up choices. [S] Steam discussion snippet.

### Escape from Tarkov
- No sourced finding on the inspect window or stat bars. Search returned only third-party tools (RatScanner, Tarkov Companion). **Could not verify.**

### Monster Hunter (Wilds)
- No sourced finding on an equipment comparison grid. The closest complaint found is about the split between weapon and armour skills. [S] Steam thread snippet: the division is called "one of the worst gimmicks in the whole series". This is a skill-design complaint, not a UI complaint.

### Dark Souls / Elden Ring
- No sourced finding on stat-delta arrows on the equip screen. A 2022 Steam thread [S] says you can compare items in the inventory but not in shops. Players complained that status icons are never explained (Reddit, via an Italian news article, snippet only).
- Green/red arrows are described in a non-souls game (Brighter Shores) and a 2011 FFXIV forum proposal, both as snippets. Not evidence for Elden Ring.

### Stardew Valley / Terraria
- Not searched in this pass. **Could not verify.**

### Factorio
- Dev blog Friday Facts #318 (2019-10-25), "New Tooltips" [F]:
  - Root category: "Properties that are directly related to the selected entity type are placed in the "root" category that has no name."
  - Rationale: "This is to avoid having pointless categories like "Inserter" and "Transport belt"."
  - "The place where the categorization really shines is in the tooltips of power generating entities."
- Player reaction [S] forum: one reply likes the cleaned-up information; another suggests linking tooltips into an in-game encyclopedia.
- Criticism [S]: in the 1.1.46 UI update, content is cropped at higher UI scale; players point to the Expand Tooltip mod.
- Note: the search found no outside design-critic piece praising Factorio density. The praise here is the developer's own account.

### Dwarf Fortress (Steam release)
- PCGamesN preview (2020-08-14) [F]:
  - Old: "the old menus, which look like classic DOS file system readouts"
  - New: "the new menus, which are still a bit information-dense but now have scrollbars and clickable tabs and buttons"
- Reading: the density was kept deliberately; the change was in navigation affordances (scroll, click), not in cutting content.
- PCGamer review snippet [S]: the new UI is "rather plain" but "a large step up for veterans".

### Caves of Qud
- Not searched in this pass. **Could not verify.**

### Cataclysm: Dark Days Ahead (item info)
- Searches found no player complaint or praise about the item info window in CDDA. The CleverRaven/Cataclysm-DDA GitHub issue tracker was reachable but the search result showed only label names. **Could not verify.** A direct search of the tracker for "item info" or "info window" is the next step.

### RimWorld
- Steam discussion, 2018 [S]: "Too much options in the architect menu. Need UI mod!" The poster says that once mods added options, the panel no longer had room to show info about the selected item.
- Steam comment on the Relevant Stats mod, 1.6 [S]: "Hiding it made more sense before 1.6, but now it actually displays useful information that doesn't make sense to hide away by default."
- RimWorld Wiki [S]: the inspect pane appears when the player selects a pawn, item or structure and shows details about that object.

---

## Concrete rules (with sources)

1. Compare on demand, not by default. D4 hides comparison behind a held key [F, Den of Geek]. PoE hides it behind Alt [F, poedb].
2. Comparison should name the slot or the equipped counterpart. PoE [F]: "display it against what you have equipped". Pawn [S] attaches an upgrade arrow to the item itself.
3. Hide complex term definitions behind a modifier, and offer a pinned or persistent mode if the text must be read across multiple terms. PoE 2 [S] pins the box on click.
4. Progressive disclosure: "Progressive disclosure defers advanced or rarely used features to a secondary screen" [F, NN/g, page dated 2006-12-03]. "If the user clicks the advanced options button, the system discloses the additional features." [F, same page].
   - Conflict: a search summary cited Nielsen introducing the idea in 1983 or 1995. The fetched NN/g page is dated 2006. Not resolved.
5. Group properties by relevance: Factorio's root-category rule removes redundant headers [F].
6. Density is acceptable when navigation is good: DF kept density and added scroll and clickable tabs [F].
7. Do not let an addon depend on fragile tooltip hooks. Pawn's breakage [S] shows the cost.
8. Keep the equipped-item tooltip reachable; Pawn's bug fix pulled equipped-item scoring back in [S].
9. Expose roll ranges as an optional layer (D4 "Advanced Tooltip Info") [S].
10. Scaling needs testing: Factorio tooltips were cropped at higher UI scale [S].

---

## Quotes (verbatim, with source, date)

Fetched and checked [F]:
1. "compare every stat and bonus on a piece of gear" - Den of Geek, "Diablo 4's Best Hidden Setting Lets You Automatically Compare Gear", https://www.denofgeek.com/games/diablo-4-best-hidden-setting-compare-gear , 2023-06-06
2. "while you have an unequipped item selected" - same page, same date
3. "Progressive disclosure defers advanced or rarely used features to a secondary screen" - Nielsen Norman Group, "Progressive Disclosure", https://www.nngroup.com/articles/progressive-disclosure/ , 2006-12-03
4. "If the user clicks the advanced options button, the system discloses the additional features." - same page, same date
5. "Holding Alt while hovering over an item in your inventory will display it against what you have equipped." - PoE official Helpful Tips (mirror), https://poedb.tw/us/HelpfulTips , undated
6. "Hold Alt while viewing items in your inventory to compare them with their equipped counterpart." - same page
7. "This is to avoid having pointless categories like "Inserter" and "Transport belt"." - Factorio, "Friday Facts #318 - New Tooltips", https://cf-www.factorio.com/blog/post/fff-318 , 2019-10-25
8. "The place where the categorization really shines is in the tooltips of power generating entities." - same post, same date
9. "the old menus, which look like classic DOS file system readouts" - PCGamesN, "Dwarf Fortress has a much better UI on Steam", https://www.pcgamesn.com/dwarf-fortress/menus , 2020-08-14
10. "the new menus, which are still a bit information-dense but now have scrollbars and clickable tabs and buttons" - same page, same date

Search snippet only [S], not fetched (wording not verified against page):
11. "Too much options in the architect menu. Need UI mod!" - RimWorld, Steam discussion, https://steamcommunity.com/app/294100/discussions/0/1736594593597361468 , 2018 (per snippet summary)
12. "Hiding it made more sense before 1.6, but now it actually displays useful information that doesn't make sense to hide away by default." - Steam Workshop comment, Relevant Stats mod, https://steamcommunity.com/workshop/filedetails/discussion/2573728367/4737210531931346994 , date not confirmed
13. "one of the worst gimmicks in the whole series" - Monster Hunter Wilds Steam thread, https://steamcommunity.com/app/2246340/discussions/0/596270875262654304 , date not confirmed (about skill split)
14. "a large step up for veterans" - PC Gamer review, "The new Dwarf Fortress UI looks so much better", https://pcgamer.com/the-new-dwarf-fortress-ui-looks-so-much-better , date not confirmed (wording from search summary, not checked)
15. "Im unable to open close my architect tab after trying to use only these mods." - RimWorld Steam comment, Vehicle Framework mod, https://steamcommunity.com/sharedfiles/filedetails/comments/2692669482 , 2026 per snippet
16. "Advanced Tooltip Compare" / advanced tooltip info descriptions - Icy Veins, https://www.icy-veins.com/d4/news/how-to-make-diablo-4-even-better-7-hidden-things-you-should-do/ , page returned 403 on fetch; wording not verified

Only 10 quotes are fetched and checked. The target of 15 verbatim quotes is not met. Items 11-16 are snippet-level and should be re-checked before quoting in any document.

---

## Patterns worth adopting

- Modifier-held comparison against the equipped item in the same slot (PoE, D4). Show the equipped item named in the comparison.
- Keep the default tooltip short; put full stat and term detail behind a hold key (D4, PoE). Offer a pin for long text (PoE 2).
- Group stat rows by category, and drop headers that repeat the item name (Factorio root category).
- Use an explicit "advanced" layer for roll ranges and numeric breakdowns (D4 Advanced Tooltip Info, unverified).
- Use a scoring layer only as an optional overlay (Pawn). Do not make the base tooltip depend on it.
- Make dense panels scrollable and tabbed rather than cutting content (Dwarf Fortress).
- Test tooltips at high UI scale and with mod-extended lists (Factorio cropping, RimWorld architect overflow).
- Reserve bars or colour for deltas only if colour-blind-safe and paired with sign or arrow. This is a design principle; no game source checked.

---

## Could not verify

- Diablo 4 delta arrows and green/red colour coding (search summary only; fetched article silent).
- Diablo 2 and Diablo 3 compare behaviour (not searched in this pass).
- Elden Ring or Dark Souls equip-screen stat-delta arrows.
- Monster Hunter equipment comparison grid UI.
- Escape from Tarkov inspect window and stat bars.
- BG3 and Divinity shift-to-compare behaviour.
- Stardew Valley, Terraria, Caves of Qud tooltips (not searched).
- Cataclysm: Dark Days Ahead item info complaints or praise.
- Celia Hodent GDC talk on inventory or information density: search found only general talks. The Gamer's Brain (book) may cover it; unverified.
- Gamedeveloper.com and GDC Vault articles on inventory UX: not found in this pass.
- Reddit threads on any of the above: Reddit was not reachable via search results.
- Nielsen Norman Group date of the progressive disclosure article: fetched page says 2006-12-03; a search summary cited 1983 or 1995 for the idea. Unresolved.
- Pawn version history beyond the snippet-level changelog.
- "Tooltip hell" as a named term in games UX literature: not confirmed with a source.
