# Experimental 3D renderer fork

This fork ports the renderer from
[r-melvin/Cataclysm-DDA-3D](https://github.com/r-melvin/Cataclysm-DDA-3D),
renderer commit `00d5825a2585bc58cc46c4cfe5d72f7c7f46d37a`, onto Bright Nights.
The original Cataclysm contributors retain their existing attribution and license.
BN remains responsible for gameplay, world generation, visibility, turns, and saves.
CDDA worlds and mods are not migration inputs for this port; start with a BN world.

## Build and play

Use BN's Clang/CMake build instructions. Renderer shaders use BN's existing HLSL
and shadercross build pipeline; backend artifacts are generated in `data/shaders`
for the requested `SHADER_TARGETS`. No separate GLSL compiler is required.

```sh
cmake --preset linux-slim
cmake --build --preset linux-slim --target cataclysm-bn-tiles cata_test-tiles
./out/build/linux-slim/src/cataclysm-bn-tiles
```

In Options > Graphics, select the `gpu` renderer, restart if changing renderers,
and enable **Voxelised 3D renderer (experimental)**. This setting is off by
default; disable it in the same menu to return to the standard isometric view.
New configurations prefer `gpu` when SDL provides it. Unsupported graphics
backends retain BN's ordinary 2D view.

- **Ctrl+B** toggles 3D.
- With **3D WASD controls** enabled, W/A/S/D walk relative to the camera and arrows
  rotate/tilt it. These keys replace their ordinary gameplay bindings only in the
  main 3D gameplay view. Menus retain their normal controls.
- The wheel zooms; **Ctrl+R** resets the camera.
- Middle drag orbits; Shift+middle drag pans.
- All camera actions can be rebound or selected through the action menu.

Graphics options control thin walls, wall finishes, cutaways, storey height, and
view depth. The port includes wall corner trimming, wall surrounds for windows,
wall-mounted boards, shallower lockers fitted against walls, upright loose items,
solid natural objects, and visible vehicles. Movement, combat, vehicle physics,
and item behavior remain BN's own.

Vehicle quarterpanels (`HALF_BOARD`) stay at body height even beneath a roof;
full panels and windshields keep cabin height. Roof caps take the exterior panel
material, while rooftop equipment keeps its own sprite. Native `shapes` direction
variants ending in `_nw`, `_ne`, `_sw`, or `_se` trim the corresponding body corner.
These are generated part IDs, not new map or vehicle rules. Once a vehicle has a
lit or known mount, its remaining mounts use that light instead of disappearing
individually; a model with no lit or known mounts remains hidden.
Installed wheel parts now use faceted elliptical tire meshes with raised hubs,
and map the installed wheel sprite onto the tire face. BN remains authoritative
for wheel damage, removal, and vehicle behavior.

The optional **3D voxel-style item models** graphics setting extrudes loose-item
sprites that have a transparent foreground and no separate backdrop. The source
art stays on the front and back, while shaded faces follow opaque-pixel
boundaries, preserving transparent corners and notches. Sprite rotation and
mirroring are applied to the silhouette. Generic items use a shallow volume;
cylinders use deeper profiles, and authored `.vox` items take precedence when
available. Items without a supported transparent sprite keep the upright
billboard fallback. The setting is off by default. This adapts the general
single-sprite extrusion idea from
[2d-to-3d-voxelizer](https://github.com/GazPrash/2d-to-3d-voxelizer) to the
renderer; no external code or art is included.

Authored asset proportions and contact points are defined in
`data/raw/r3d/presentation_profiles.json`. A profile names a category and asset
ID, gives optional forward/right/up dimensions in map tiles, and marks the
normalized point that should touch its support surface. Zero dimensions retain
the inferred sprite or VOX size. Sprite depth can be specified directly with a
right-axis size or as a width-relative `depth_fraction`. Material-region names
and VOX palette slots are included for authoring and comparison; they are
currently shown as annotations in the gallery rather than changing palette
colors. Comparison packages preserve the profile file and render a scaled top
footprint and side elevation beside the source sprite and captured views.

## Renderer diagnostics

Set `CATA_R3D_DEBUG_CHUNKS=1` before launching to log a chunk-work sample after
the initial visible chunks are built and once per second afterward. `frame_ms`
is renderer CPU time; `mesh_ms` measures CPU meshing;
`upload_ms` measures chunk buffer creation and geometry upload; and `light_ms`
measures light sampling and light-buffer upload. The sample also reports the
visible, rebuilt, deferred, and failed chunk counts, geometry size, uploaded
bytes, and estimated bytes held by resident chunk buffers. The initial visible
scene is built synchronously, so its first sample shows that cost even though
there is no deferred queue. These measurements separate mesh work, light work,
and queued rebuilds before changing the existing scheduling policy.

## Comparison reports

BN ships the `HitButton_iso` reference and `UltimateCataclysm` tilesets; no external
CDDA tileset composition is needed.

```sh
python3 tools/r3d_spike/compare_session.py 'My BN world' \
  --binary out/build/linux-slim/src/cataclysm-bn-tiles --output /tmp/bn-report
```

Play and quit normally. A private world copy is used; session progress remains in
`/tmp/bn-report/user`, and the original world is untouched. Five checkpoints are
recorded at least 30 seconds apart at normal gameplay input boundaries. Each has
one fixed 2D isometric reference, eight 3D angles, the current world snapshot,
active options, map/state dumps, build/mod information, and selected logs.
Captures briefly pause play without charging realtime moves or consuming queued
input. SQLite snapshots include committed WAL pages and do not require copying
live journal files.

Attach every generated `comparison-NNN.zip` to a fork issue. Files stay below
GitHub's 25 MB limit; oversized individual saves are chunked without dropping data.
The included `restore.py` reconstructs them after extraction. Review the archive
before sharing: character names, notes, explored maps, and paths can be included.
Saves reproduce a scene with matching BN assets/mods, not deterministic future RNG.
See `tools/r3d_spike/COMPARE.md` for controls and crash recovery.

The CDDA UI-overhaul and first-person branches are separate work from this renderer
port. BN does not have CDDA's ImGui frontend, so those interfaces need their own
BN implementation rather than copying CDDA's UI files.
