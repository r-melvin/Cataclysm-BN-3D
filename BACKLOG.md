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
a single wheel or body panel based on tileset art. The renderer now has a
MagicaVoxel reader and exposed-face extraction, but does not yet load `.vox`
assets into vehicle meshes or draw their palette directly. Compare an authored
part with the existing procedural geometry before deciding whether a full asset
pipeline is worthwhile.

Investigate whether Kenney Shape can be automated or extended through a
documented import/export interface. If not, check whether its Unity version and
scripting backend work with MelonLoader on native Linux, or whether a comparable
Unity plugin can import a sprite, set per-pixel depth, and export `.vox`
repeatably. Prefer a documented or stable automation surface over depending on
private UI internals. Keep the workflow optional and out of the game runtime;
verify the tool license and the source-art license separately.
