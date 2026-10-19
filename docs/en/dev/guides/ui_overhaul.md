# Native BN UI and 3D overlays

The fork ports the fork's interface work to Bright Nights' native SDL and
curses interface. Game simulation, visibility, actions, and saves remain BN's.

- Graphics → Modern menu borders selects muted borders and padded headings.
  Disable it for the classic appearance. Character creation choices use clearer
  descriptions. This native BN port does not depend on CDDA's ImGui system.
- F4 reads a status report in a popup and the message log: time when known,
  weather, temperature when measurable, location, needs, and visible threats.
- Graphics → 3D weather enables cloud/sky colour, distance fog, rain and snow,
  wind drift, and sun/moon sprites. These are presentation effects; they do not
  alter weather, time, visibility, or gameplay RNG.
- Graphics → 3D HUD enables combat text, status badges, heard-sound markers,
  threat pointers, compass destinations/notes, route and zone markers, damage
  indication, and location notices. Information follows native BN perception.

Use the normal [renderer and capture guide](renderer_3d.md). The first-person
camera and guided onboarding remain separate feature ports; main uses orbit view.

Build both `cataclysm-bn-tiles` and `cata_test-tiles`. The pure overlay/camera
checks can also run through `bash tools/r3d_spike/run_pure_tests.sh`.
Native macOS and Windows execution still requires platform validation.

Capture redraws suppress weather-change notices. Outdoor precipitation follows
native tile perception; HUD shapes and text stay inside the map rectangle.
The F4 report is also registered in the curses interface.
Rain, snow and transient HUD fades continue to redraw while waiting for input;
these presentation frames do not spend turns or process gameplay actions.
