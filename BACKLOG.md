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
