# Project backlog

Ideas parked for later work. These are not active tasks and should not be
implemented unless requested.

## Vehicle-free play mode for the 3D renderer

Explore an optional mode for players who want to use the 3D renderer while
vehicle visuals are being improved. This would temporarily remove vehicles as
playable content, rather than hide their models in the renderer.

Before implementation, define how the mode handles vehicle world spawns,
construction and parts, driving and interaction, and vehicles already present
in existing saves. Prefer an opt-in, clearly labelled mode that does not change
the default game or the 2D renderer. Determine which parts can be safely
controlled by data or Lua and which require core C++ hooks; a partial mod must
not imply that all vehicle gameplay is disabled.

## Evaluate Kenney Shape and Unity automation for vehicle voxels

Try Kenney Shape as a source of authored per-part voxel models, beginning with
a single wheel or body panel based on tileset art. The renderer can read
MagicaVoxel files, extract exposed faces, and load optional `.vox` files from
`data/raw/r3d/vehicle_parts/` keyed by native vehicle-part ID. No authored part
models are bundled yet, so the procedural wheel remains the fallback. Compare
an authored part with the existing geometry before deciding whether a full
asset pipeline is worthwhile.

Investigate whether Kenney Shape can be automated or extended through a
documented import/export interface. If not, check whether its Unity version and
scripting backend work with MelonLoader on native Linux, or whether a comparable
Unity plugin can import a sprite, set per-pixel depth, and export `.vox`
repeatably. Prefer a documented or stable automation surface over depending on
private UI internals. Keep the workflow optional and out of the game runtime;
verify the tool license and the source-art license separately.

## Add directional character models to the 3D renderer

The current renderer draws player and NPC layers as camera-facing sprites. The
built-in tilesets inspected so far provide a single character image rather than
front, back, and side views, so mirroring or rotating that image cannot produce
correct directional poses. Define a direction convention from character travel
relative to the camera, then provide 3D character geometry or authored four-way
pixel-art views. Preserve the existing layered clothing and wielded-item look,
and fall back to the current sprite when a tileset has no directional assets.

## Add fog of war and a time-of-day sky to the 3D renderer

Research how to use the game's visibility and lighting state to render fog of
war that distinguishes unseen areas from visible darkness. Explore a sky and
ambient horizon lighting driven by the in-game day/night cycle so the exposed
world does not disappear into a uniformly black background. Preserve gameplay
visibility rules, and account for indoor, underground, weather, and renderer
cutaway views when choosing fog and sky behavior.
