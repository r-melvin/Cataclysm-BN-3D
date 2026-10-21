# UI redesign: executive summary

Design and plan for a from-scratch player interface for Cataclysm: Bright Nights 3D. Design
and planning only; no source was changed. Written 2026-10-21 on `feat/ui-overhaul`.

| Document | Contents |
|---|---|
| [01_pain_points.md](01_pain_points.md) | Ranked, sourced complaints about Cataclysm menus and comparable games' UIs |
| [02_patterns.md](02_patterns.md) | Patterns from comparable games with adopt / adapt / reject verdicts and evidence grades |
| [03_design.md](03_design.md) | Principles, budgets (before/after counts), information architecture, sidebar replacement, every screen with wireframes, input model, accessibility, config file, cuts |
| [04_architecture.md](04_architecture.md) | Toolkit decision, layers, text engine, input abstraction, narrator, theming, curses coexistence, testing |
| [05_plan.md](05_plan.md) | Milestones with files, verification, risks, effort; curses retirement; UI-01 guided start; options migration; open questions |
| [06_frontend_options.md](06_frontend_options.md) | Stay on SDL3 or move to Godot / Unity / Unreal / Bevy / web: verified accessibility facts, embedding, r3d reuse, cost, recommendation; curses retirement scope |
| [research/](research/) | Twenty-two raw research files (a-p, q_*), one per agent, with URLs and verification notes |

The HTML pages are the reading format (inline SVG diagrams, light and dark). The Markdown files
and `diagrams/*.svg` are the sources; rebuild with
`python3 tools/ui_redesign_docs/build_html.py docs/en/dev/research/ui_redesign`.

## What the research found

- Players' top complaints about Cataclysm are overloaded nested menus (16 sources), inconsistent
  keybindings per screen (15), text that does not fit (14) and an inventory family of
  fifteen screens. Upstream CDDA's ImGui migration added a long defect tail (15 sources) and
  "can I turn it off" reactions; it is a lesson, not a model.
- The codebase has 61 player-facing screens, 585 actions in 61 contexts, 252 options, fixed
  character-cell layout with restart-only font sizes, no mouse hit-testing in lists, a
  gamepad path whose buttons cannot match their bindings, no colour-vision support and no
  accessibility code.
- Reddit, YouTube and the Steam discussion board were unreachable from the research tools;
  the brief's subreddits were not mined. All quotes come from GitHub, Discourse, Steam reviews
  (API) and fetched pages, and are graded.

## The design in one paragraph

Twenty-three screens instead of sixty-one, with a twelve-item / two-level menu budget held per
screen. One contextual interaction verb with a hold-list of secondary verbs; two eight-slice
radial wheels and a hotbar; one Inventory with four modes; Craft and Build as twins; one
Character screen; Map and Journal; Settings cut to 39 in-game options on six tabs with the
rest in a self-documenting `config/advanced.json`. In 3D mode the sidebar is removed and
replaced by a HUD (vitals strip with a body figure, badges with icon + word + trend, three-line
ticker, compass, minimap, prompt), with every item still available as text in the F4 report
and the character screen. Accessibility: one remapping editor for all devices with hold/toggle
per action; Okabe-Ito semantic tokens plus palette remaps per colour-vision mode and a rule
that no state is colour-only; a self-voicing narrator (NVDA/SAPI, AVSpeech, speech-dispatcher)
fed by the widget tree, with AccessKit as a gated second phase; live UI scale 75-200 %;
reduced motion and flashing; sound cues as text; one-handed presets and a command palette.

## Architecture in one line

An in-house retained widget tree on `SDL_Renderer` + the SDL3_ttf text engine, where the tree
is also the accessibility tree and the controller focus model, bridged to NVDA/VoiceOver/Orca
through `accesskit-c`. RmlUi and ImGui were rejected on accessibility and single-text-path
grounds. Curses is retired (user decision, UI-02): phase 1 removes the terminal and console
backends, the build option, presets and CI jobs in one week; the cell canvas goes with the last
legacy screen.

## Frontend decision (06)

Moving to Godot, Unity, Unreal, Bevy or a web frontend was evaluated on verified facts. Godot
4.7 has the most real screen-reader support (AccessKit, experimental, Linux/macOS/Windows/
Android, exposed to GDExtension), but a move costs 20-30 weeks of renderer, compute and
embedding work for about 10 weeks of accessibility saving, and the game core's blocking input
loops would have to be bridged into a scene tree that is not thread-safe. Recommendation: stay
on SDL3, with a decision gate at the end of M1 (the `accesskit-c` SDL spike must work with two
of three screen readers).

## Decisions needed first

1. Whether upstream Bright Nights merges continue (drives how fast `TILES` guards are removed).
2. Approve the HUD design before the 3D sidebar removal milestone.
3. Keep the legacy key group on by default, or ship the clean 32-key default.
4. Defense mode and minigames: legacy path or cut.
5. Fund the AccessKit phase (and accept the M1 gate) or stop at self-voicing.

Full list: `05_plan.md`, "Open questions".

## Unverified claims to be aware of

Okabe-Ito hex values (secondary source), Steam Deck speech-dispatcher access in Flatpak/SteamOS,
macOS and Windows speech APIs (not run here), several [snippet]-graded game quotes, the LOC
estimates for the widget layer, Godot export/Steam Deck specifics, Unreal EULA text, Unity's
desktop screen-reader matrix, and the engine effort estimates (no comparable case study found).
