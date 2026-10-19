# 3D renderer review and remediation plan (2026-10)

Review of `45ba1a3~1..0e05f91` (the r3d vehicle, voxel and silhouette work). Static review only; ✔ marks
findings confirmed against source.

## Defects

1. ✔ Broken wheels are never drawn: `r3d_vehicle_rules.cpp` skips broken parts before setting `has_wheel`.
   The wheel layer lookup also picks the first wheel layer, broken or not.
2. ✔ Authored `depth_fraction` is overwritten by `size[1]/size[0]` in `r3d_entities.cpp`. **Decision:** an
   explicit `depth_fraction` wins.
3. Roof cap is emitted at `vehicle_cabin_h` over half-height panels whose walls stop at `body_h`.
4. Vehicle cells are keyed by `point(mount.x, mount.y)`; z is dropped.
5. Lighting: unlit mounts inherit the first visible light; monsters in the dark draw full-bright; silhouette
   ignores light, shows through creatures and while driving.
6. Shaped-panel side walls are emitted on every outline edge, including internal ones.
7. `surface_height` raises every vehicle tile, including wheel-only/frame tiles.
8. Camera yaw is overwritten on taking vehicle control and never restored.
9. `item_geometry::billboard` is unreachable.

## Performance

- ✔ Vehicle mesh rebuilt and uploaded every frame; ✔ silhouette pass copies the entity view and rebuilds a mesh.
- `add_vehicles` per-frame allocations, string flag lookups, `"vp_" + id` strings, substring/neighbour scans.
- ✔ Item sprites voxelised even with `R3D_VOXEL_ITEMS` off; every sprite category keeps two alpha masks.
- Item loop: string building, two map lookups and two `get_option` calls per item tile.
- `steady_clock` instrumentation runs when the debug flag is off.

## Simplification / dead code

- ✔ `best_body_color`, `add_vehicles_options::you`, `color_to_packed_light` (tests only) are dead.
- Sprite lookup duplicated 4×; mirrored trim/windshield/bumper pairs; three dynamic buffer trios.
- `material_regions` parsed in C++ but unused by the renderer; duplicated VOX occupancy validation.
- `r3d.cpp` formatting still mangled; AGENTS.md violations (9- and 7-parameter lambdas, explicit types, missing
  `<limits>`).

## Tests

- Tautological tests in `r3d_test.cpp`: yaw (re-implements production), cutaway (`if(true)`), height constants.
- Gaps: broken/open/multi-panel vehicle parts, malformed VOX inputs, size/depth precedence, panel/bumper/windshield
  geometry, silhouette, asset loading.
- `r3d_door_test.cpp` and `r3d_item_profiles_test.cpp` contain unrelated tests.

## Plan

Atomic commits; build `cataclysm-bn-tiles` + `cata_test-tiles` before each lands.

1. **Hygiene:** format `r3d.cpp`; remove dead code and tautological tests.
2. **Correctness (test first):** broken wheels and wheel layer choice, depth precedence, z-aware cell key, roof
   height, shaped-panel neighbour check, `surface_height` floor parts.
3. **Lighting decisions (owner):** dim unlit parts/monsters; silhouette while driving, behind creatures, unlit.
4. **Performance:** gate voxelisation on the option and drop unused masks; hoist item-loop lookups; cache vehicle
   meshes by change; emit silhouette during the main entity build. Measure with chunk instrumentation.
5. **Refactor:** sprite lookup helper, mirrored pair functions, `dynamic_mesh_buffers`, options structs, pure
   yaw/cutaway functions with tests.
6. **Test backfill:** malformed VOX cases, `vehicle_profile` broken/open/multi-panel, geometry bounds, split
   misnamed test files.

Phase 4 follows phase 5 (both touch `r3d_entities.cpp`).
