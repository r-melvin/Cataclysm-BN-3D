# Guided onboarding in Bright Nights

This fork includes optional lessons and screen explanations around native BN
play. It uses BN actions, event subscribers, dialogue conditions, and sidebar
panels. Lesson titles, descriptions and conditional variants are included in BN's
JSON translation extraction. Hints do not spend turns or modify combat, movement, or survival rules.

Select Guided hints in the Interface options:

- Off disables the overlay.
- Play Now only (default) enables hints for quickly generated characters.
- Hints only enables messages and the checklist for all characters. Popup lessons
  finish after their text is logged, so later lessons remain available.
- Hints and popups also allows lesson popups.

F1 opens the lesson checklist. F2 explains a supported screen, including inventory,
crafting, construction, and map screens. The Next sidebar
panel shows the current step. Lesson progress is stored with the avatar and
survives saving and loading. Existing saves without progress start cleanly.
BN's built-in Tutorial Game keeps its own teaching flow.

The optional Guided start kit chooses an ordinary additional Evacuee (guided)
scenario for Play Now (Default Scenario), with a Prepared Survivor profession.
It provides a backpack, flashlight, water and basic tools. Disable the option for
the normal start. Custom character creation and random scenarios stay native.

Lesson and screen data live in `data/json/onboarding/`. The pure tracker in
`onboarding_core.cpp` has no game dependencies. Native events feed it directly;
a small adapter reports successful consumption, activity starts, sleep, and
changes of overmap tile where BN lacks the corresponding CDDA event. Hunger,
thirst, and fatigue checks use BN's own state and descriptions. Lessons for
unavailable optional UI actions are disabled rather than inventing game actions.

Build both `cataclysm-bn-tiles` and `cata_test-tiles`, then run the `[onboarding]`
filter. Use copies of saves for gameplay checks. CDDA saves and mods are not
compatible inputs. Native macOS and Windows execution remains unverified.
