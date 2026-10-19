# Renderer scene fixtures

Use a private world or a capture session's cloned save. These scripts replace
nearby terrain and furniture; they are developer scene builders, not gameplay mods.
Run the game in a virtual window for automated testing.

## Opening states

Start on clear, level ground in daylight, away from map boundaries, vehicles and
creatures. Open the Lua console (default: backtick), press Enter to edit, paste
the contents of `openings.lua`, then press Ctrl+S. Paste the source itself;
`dofile` is unavailable in BN's sandboxed console. Confirm the "Opening fixture
ready" message and press Escape to return to play.

The script replaces a 17 × 13 area centered on the avatar. A wall three tiles
north holds these states from west to east:

| Window | Window | Window | Window | Door | Door | Door | Door |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Intact | Open | Broken glass/frame | Empty frame | Closed | Open | Broken | Empty frame |

Inspect with cutaways off, then on, and with thin walls off, then on. Rotate the
camera to inspect both sides; switch to first person and 2D for the same save.
The broken/empty windows must stand in the wall opening, retain the sill/lintel,
and keep their native passage and visibility behavior. Door leaves must remain
visible in closed and open states. Wall-face profiles must affect only their
selected surround materials, leaving panes, leaves, hinges and geometry intact.

Save this private scene and pass its user directory to `compare_session.py` using
`--userdir` and `--world`. See [the comparison guide](../COMPARE.md). Capture
reports retain the scene save, source profiles, logs and camera views; the gallery
marks which states were actually captured. The script constructs native terrain
states directly; it does not establish that every bash/open interaction has been
tested. Vehicles, props, roofs, lighting and platform fixtures remain in the
[project backlog](../../../BACKLOG.md#r3d-07-reproducible-visual-fixtures-and-gpu-acceptance-checks).

## Furniture proportions and contact points

Paste `furniture.lua` into the same private Lua console. It replaces a 13 × 11
area with four columns (bin, chair, locker, table). Each column has a piece backed
by the north wall, a separate piece, and a joined pair. Use the native unprofiled
view as a baseline, then add furniture profiles as described in the
[renderer guide](../../../docs/en/dev/guides/renderer_3d.md). Reload the tileset
or restart to install changed profiles. Work on a private data copy as well as a
private save; do not modify installed artwork during normal play.

Check exact dimensions, anchors, wall contact, front orientation, and legs at
several camera angles. Repeat with one authored axis at a time, thin walls both
on and off, and with profiles removed. Omitted dimensions must keep native joins
and wall fitting. Explicit spans stay exact, so a short authored width can leave
a visible gap in a joined row. Wall contact takes precedence over horizontal
anchors. The script does not alter the furniture's native gameplay properties.
