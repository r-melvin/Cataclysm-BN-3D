# Sprite-to-voxel asset pipeline for vehicles and characters

Originally researched 2026-10-19; reconciled with the renderer and the separate
asset CLI at `b4e0493`. Current outstanding asset trials and character work are
tracked only in the [project backlog](../../../../BACKLOG.md), R3D-05/06.
This document describes tool roles, handoff conventions and acceptance gates.

## Tool roles and the current boundary

The separate [Bright Nights asset CLI](https://github.com/r-melvin/Cataclysm-BN-3D-Asset-Pipeline)
is one Go application, `sprite-pipeline`. Its adapted
[2d-to-3d-voxelizer](https://github.com/GazPrash/2d-to-3d-voxelizer) backend accepts
four sprite views and writes MagicaVoxel VOX directly. It also packs four-view
sheets, inspects/validates VOX files and records manifests. The original
voxelizer's OBJ-only output is therefore not a blocker for this workflow, and
adding an OBJ importer to BN is not a prerequisite.

[Kenney Shape](https://kenney.nl/tools/kenney-shape) is a separate authoring
application for per-pixel depth and VOX export. The CLI's optional MelonLoader
adapter runs Kenney Shape headlessly to load/export an existing `.kenshape`
project; it does not convert PNGs into Kenney projects. Unity/MelonLoader hooks
belong to the asset tool, not Bright Nights. Install the editor and loader
separately; do not ship their applications, archives or bundled sample projects
in BN assets.

The [asset CLI README at the audited revision](https://github.com/r-melvin/Cataclysm-BN-3D-Asset-Pipeline/blob/b4e0493c72136fdfb3ffe805496fe8347bb70099/README.md)
records a Linux Unity 2021.3.45f2 Mono batch-export smoke test and Go/path tests
for the supported platforms. Native Windows/macOS Kenney application export
remains unverified there. This documentation audit inspected that README; it
did not repeat the external tool's tests or establish new platform support.

The original Kenney research described 64x64 art, 16 colors and equal-looking
front/back extrusion. Treat it as useful manual volume authoring/cleanup for
compact parts, not an inference engine for arbitrary vehicles or character
faces. Four separately authored character views carry information one sprite
cannot provide. The CLI documents one-color-per-voxel limitations when converting
view-specific colors; judge the generated model visually, not just by parser
acceptance.

## Renderer handoff

- Item models use `data/raw/r3d/items/<item-type-id>.vox` and the optional voxel
  item setting. The CO2 tank is a bundled example. Source proportions normally
  fit a tile; shared presentation profiles can override dimensions and support.
- Vehicle models use `data/raw/r3d/vehicle_parts/<native-part-id>.vox`.
  `wheel.vox` and `wheel_wide.vox` are bundled. Matching wheel and non-wheel main
  parts can use authored geometry; missing models use procedural/sprite paths,
  and open/broken main parts keep state-specific rendering.
- The runtime already reads VOX and meshes exposed surfaces greedily. Keep
  palette index zero transparent and follow the existing size/occupancy limits.
  See the [item](../../../../data/raw/r3d/items/README.md) and
  [vehicle-part](../../../../data/raw/r3d/vehicle_parts/README.md) authoring guides
  for coordinates and filename conventions.
- Character models do not have a runtime provider yet. The current body,
  clothing, hair, armor and wielded layers are sprites. Exporting a character
  VOX alone does not implement directional character rendering.

## Authoring and comparison procedure

Use the bundled wheel/tank as ingestion references, then prepare one candidate
panel/fender or prop. Preserve the source project, source sprite IDs, licensing,
tool version/options, palette, pivot and scale in its manifest/provenance.
Compare generated and manually authored versions with the existing fallback
before adopting the model. BN part IDs, assembly, paint, state and interactions
remain authoritative; an asset describes reusable shape/material only.

For a directional trial, prepare equal-sized centered views ordered
`[LEFT, FRONT, RIGHT, BACK]`, then use the CLI's `voxelize` command. Preserve
compatible support anchors across equipment layers. A missing directional
layer needs a sprite fallback; do not bake every equipment combination into one
static body. Travel/facing relative to the camera needs a renderer convention,
not a change to BN's movement rules.

Use the normal [comparison workflow](../../../../tools/r3d_spike/COMPARE.md) with
saved assemblies, both shipped tilesets and all eight orbit angles. Review
front/back/side readability, proportions, palette, support, wheel damage,
open/broken parts and equipment alignment. Geometry snapshots supplement those
captures but cannot prove image quality or material correctness.

## Acceptance gates

A candidate must improve readability at normal gameplay scale over the existing
fallback, preserve game-driven state/layering and remain editable with recorded
provenance. A character must show a recognizable back, face and left/right
profiles without breaking clothing or wielded layers. If reconstruction needs
more repair than it saves, keep the renderer fallback rather than committing to
that tool route. Check input-art and model redistribution rights separately
from the tool's own license.

For current task status, use R3D-05/06 in the backlog; this procedure is not an
additional work queue.
