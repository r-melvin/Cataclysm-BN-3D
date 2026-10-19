# Optional 3D vehicle part models

Place MagicaVoxel `.vox` models in this directory and name each file with the
native Bright Nights vehicle part ID, for example `wheel_wide.vox`. The
renderer loads these files at 3D renderer initialization and uses them when a
matching installed part is present. Missing, malformed, or unsupported models
fall back to the regular tileset and procedural geometry.

Models can replace wheels or non-wheel main vehicle parts when their file stem
matches the native vehicle part ID. In the model, X runs along the vehicle's
forward/rear axis, Y runs across its width, and Z is vertical. The renderer
scales each model into its mount-space bounds, so source proportions are
normalized. For wheels, center the model in the XY footprint and fill its Z
height. The MagicaVoxel palette is used directly; palette index zero is
transparent and occupied voxels should use nonzero indices. A custom model
replaces the part's procedural body geometry while the vehicle's game state,
mounts, parts, and separate wheel geometry remain game-driven.

Before adding an asset, check that its source artwork and model license allow
redistribution with Bright Nights 3D. Keep the `.vox` source file in the repo so
the asset remains editable.

`wheel_wide.vox` is an original low-poly tire with separate tread, sidewall,
hub, and rim colors. The native `wheel_wide` part ID selects it automatically;
other wheel types keep their existing renderer geometry unless a matching model
is added.
