# Bright Nights 3D vehicle rendering: findings and next steps

> Historical baseline review. Current outstanding work is maintained only in the
> [project backlog](../../../../BACKLOG.md). Role lists, faceted/authored wheels,
> shared windshields and door/panel classification fixes have since landed; the
> baseline findings below are evidence, not a current defect or task list.

Research date: 2026-10-18. Source baseline: `dbb2cd0712baea3639ec03abcebb005b81ad904f` (`main`). This is a source and capture review; no game code or external repository code was run or changed.

## Finding

The renderer now keeps a whole visible vehicle together, gives half-height panels the lower body profile, shapes native quarterpanel corners, slopes end windshields, separates the roof cap from rooftop equipment, and keeps vehicle orientation through comparison captures. Those address specific defects reported in [fork issue #4](https://github.com/r-melvin/Cataclysm-BN-3D/issues/4) and delivered in [PR #5](https://github.com/r-melvin/Cataclysm-BN-3D/pull/5).

Vehicles can still look like gray, block-built arrangements of tiles. That is a limitation of the current reconstruction approach, rather than one isolated wrong flag: the renderer collapses the installed parts at each mount into one selected tile plus a small number of hand-built surfaces. It uses that tile both as an art reference and as a material sample for faces that do not exist in the source sprite. The most productive next step is to make the per-mount 3D shape and material roles explicit, then author a few reusable profiles and verify them with a repeatable vehicle gallery. Keep dynamic assembly, part state, movement, collision, and vehicle interaction in BN's existing simulation.

## Evidence reviewed

The fork's issue #4 described cars as too boxy, said quarterpanels and frames should be lower than full attachments, reported an incorrectly blue roof and misplaced windscreen, and noted that vehicles disappeared when only some mounts were visible. Its three attached screenshots show an unusually large vehicle with an almost flat roof, a saturated blue roof section, dark flat side windows, and a mostly cuboid body. These are direct reports and visual observations from those attachments, not general claims about every vehicle or tileset.

The post-fix comparison bundle at `.port-validation/2026-10-18/issue-4/comparison.zip` contains a saved stock-car scene, a 2D reference, the default 3D view, and eight 3D yaw captures. In that bundle, the bright blue placeholder is gone and the full car remains in the image at all sampled angles. The car is still dominated by broad gray planes and dark rectangular glass; the exported scene does not yet read as a distinctive car silhouette at the normal gameplay scale. The screenshots are manual visual evidence, not an image-difference test. The initial issue screenshots are also preserved at `.port-validation/2026-10-18/issue-4/vehicle-{1,2,3}.png`.

Current tests verify several important invariants: real installed parts determine half/full height, all four native half-panel variants trim the corresponding corner, roof art is not overlaid as a blue placeholder, and partial mount lighting does not drop vehicle mesh geometry. See [r3d_vehicle_rules_test.cpp](../../../../tests/r3d_vehicle_rules_test.cpp#L14-L151). They do not compare vehicle appearance to a visual target, enumerate material coverage, or check knowledge/occlusion for partially visible vehicles.

## Implementation at the reviewed baseline and why it reads as blocky

The vehicle renderer is in [r3d_entities.cpp](../../../../src/r3d_entities.cpp#L477-L535). It transforms native mount positions around the actual vehicle pivot and heading, groups the mounts into cells, and asks [vehicle_profile](../../../../src/r3d_vehicle_rules.cpp#L12-L63) to classify each cell from the installed BN part flags. This is a sound foundation: simulation data remains authoritative, and the renderer can represent vehicles built from arbitrary parts.

The profile returns a single `main_part_idx`, a single roof index, a single accessory index, aggregate booleans, one corner enum, and one height for each mount. It chooses the highest-z-order installed part below an internal z threshold as the main part ([r3d_vehicle_rules.cpp](../../../../src/r3d_vehicle_rules.cpp#L17-L24)); the rest of the geometry path receives that one tile as the mount's primary sprite. A vehicle can have several visually meaningful parts mounted in the same cell, but the profile does not preserve a render-layer list or define their shape and material contributions separately. This limits detail and makes part-overlap outcomes depend on one z-order choice.

For a mount with a roof, the renderer emits a square cap at cabin height. It samples a solid color from a selected body sprite rather than mapping the actual roof part's top artwork onto a roof surface ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L998-L1024)). The accessory sprite is drawn as a separate top overlay. This fixed the blue `vp_roof` placeholder reported in issue #4, but it also means the rendered roof is intentionally generic and flat. At edges, a small fixed tumblehome is applied, rather than a profile that follows a chosen vehicle's actual roof and windshield shape ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L786-L809)).

Side faces use the same sprite's opaque-pixel bounds to guess a narrow solid UV sample ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L328-L337)); upper cabin sides are synthesized from two fixed-width pillars and a recessed dark pane ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L1120-L1190)). The mapping is an approximation: a tile sprite is not a catalogue of separately addressable body paint, trim, glass, and roof materials. Flat sampled color avoids stretching a 2D side-view image over every 3D face, but it erases variation; a wrong or transparent sample can still produce a poor face. The reported black-looking windows are consistent with the current deliberately dark, flat pane treatment, though the screenshots alone do not establish that the shader or lighting is at fault.

Wheels are emitted as small rectangular prisms with a center strip for a hubcap ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L1215-L1324)). This provides a visible wheel volume without representing the installed wheel's actual tire shape, radius, rim, or spoke art. Body ends use a fixed bevel and corner offsets, and front/rear windshield slopes are inferred from neighboring mount heights ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L804-L809), [r3d_entities.cpp](../../../../src/r3d_entities.cpp#L893-L997)). Those generic dimensions help across arbitrary assemblies but cannot encode different proportions for a van, sedan, pickup, or motorcycle.

Vehicle visibility currently takes the first non-black mount light and uses it as a fallback for every unlit mount of that vehicle ([r3d_entities.cpp](../../../../src/r3d_entities.cpp#L518-L535)). This preserves a complete model when local lighting is uneven and fixes the disappearance described in issue #4. Before changing it, verify that the fallback does not reveal mounts that BN has not made visible or known. A renderer improvement must respect the game's visibility and memory rules; mesh completeness alone is not sufficient evidence for that behavior.

The source also has the data needed for more precise presentation: vehicle part IDs and flags (`HALF_BOARD`, `FULL_BOARD`, `WINDSHIELD`, `ROOF`, `WHEEL`, `OPENABLE`, `OPAQUE`), state (`open`, `broken`, `removed`), mount, direction, color, and the installed tileset sprite. Directional board shapes are generated from the `shapes[].direction` values in [veh_type.cpp](../../../../src/vehicle/veh_type.cpp#L528-L537), and standard tileset entries are already organized by native part IDs in [vehicle_parts.json](../../../../data/json/external_tileset/vehicle_parts.json#L10-L60). A renderer-owned description can add shape/material intent without introducing new vehicle rules.

## Original ranked improvement proposals

Implementation status and remaining work now live in the backlog. Preserve this
section as rationale and acceptance criteria rather than a parallel plan.

### 1. Establish a visual acceptance gallery before changing geometry

Extend the existing comparison workflow described in [renderer_3d.md](../../../../docs/en/dev/guides/renderer_3d.md#L48-L72) with a small, saved vehicle gallery. Capture 2D isometric and 3D at the same world location and zoom, then capture eight fixed yaw angles. Include one stock sedan, one van, a pickup or flatbed, a motorcycle/bicycle, and one unusual modifiable vehicle. For each, include closed/intact, one open part, a broken panel or window, a roof/accessory, and a partial-visibility setup. Keep game version, tileset, mods, vehicle construction, and camera settings with the captures. This reveals whether a fix improves common forms while damaging other assemblies.

Add a human review sheet that labels the required silhouette and materials per vehicle: body-height band, cabin/roof profile, glass placement, wheel centers, lights, doors, and attachment points. The 2D view is a reference for identity and major proportions, not a pixel-perfect texture target: a 3D model necessarily exposes faces absent from one isometric view.

### 2. Replace the single main sprite with per-mount render layers

Change the renderer's intermediate vehicle profile to preserve the installed render-relevant parts at each mount, grouped by semantic role and state. A profile should provide a geometry family and parameters, not a new part/gameplay flag. Candidate roles are frame/chassis, body panel, half/quarterpanel, windshield/window, door leaf, wheel/tire, roof, roof accessory, light, bumper, and interior. Preserve the part ID, direction, open/broken state, chosen tileset sprite and tint, mount, and vehicle yaw through mesh generation.

Keep an untouched generic fallback for parts without a profile. Start with 3 to 5 high-frequency standard shapes, not a model hard-coded for one stock vehicle. Avoid adding renderer-only fields to heavily used core headers if a separate pure profile type and resolver can carry the first iteration. If the profile must be data-driven, put it in a renderer-specific data file keyed by native part ID, validate IDs and enum/range fields on load, and allow mods to override it; do not overload simulation flags with render-only meaning.

### 3. Define explicit material donors and a small part-shape library

Make material lookup explicit: painted body, glass, rubber, metal/trim, light/lens, and interior. Prefer existing source sprites and their tint/mask colors when they have the needed pixels. Where they do not, add small renderer-owned material swatches or documented per-tileset sprite-region mappings, with safe fallback colors. Do not infer all face materials from one opaque pixel and then reuse the result on every face.

Build a compact set of mesh templates in mount units: wheel cylinder/low-poly tire with rim; body side panel with controlled corner cut and sill; windshield plane with explicit slope; roof panel/cap with selectable contour; and bumper/hood transitions. Fit dimensions and anchors from installed part semantics and neighboring mounts. Continue clipping geometry according to actual neighboring parts and open/broken states. This will give the renderer control over shape while it keeps BN's tile art for the surface.

### 4. Make assembly state and vehicle visibility first-class checks

For each profile, define what open, broken, removed, folded, and roofed parts contribute. An open door should use its actual hinge, swing direction and full leaf footprint; broken glass should not be silently replaced by an intact pane; a removed body part must open a hole where the simulation has one. Render order needs deterministic layering for overlapping parts on a mount.

Audit the lighting/visibility fallback at [r3d_entities.cpp](../../../../src/r3d_entities.cpp#L518-L535) against `entity_view` and BN's map visibility contracts. Preserve full vehicle geometry only when the corresponding positions are legitimately renderable/known. Test dark-but-visible terrain separately from unknown terrain so zero light is not confused with unseen information. If complete silhouette retention is intentional, the evidence should show that it cannot reveal hidden mounts through walls or memory boundaries.

### 5. Evaluate the 2D-to-3D voxelizer only as an offline authoring experiment

The [GazPrash 2D-to-3D voxelizer README](https://github.com/GazPrash/2d-to-3d-voxelizer/blob/main/README.md) describes single-image extrusion, repeated single sprites, dual-view, four-view and six-view generation, with OBJ export. It explicitly warns that its simple sprite-to-voxel method may not suit art with varying depth, and that generated meshes may need editor cleanup. The [v1.0.4 release](https://github.com/GazPrash/2d-to-3d-voxelizer/releases/tag/v1.0.4) identifies commit `c30cb82`. The currently published [LICENSE](https://github.com/GazPrash/2d-to-3d-voxelizer/blob/main/LICENSE) is MIT for the tool code; it is not permission to reuse BN or third-party tileset artwork.

For vehicles, treat the tool as a way to test whether a deliberately prepared front/side/rear/top sprite sheet can become a useful, editable static template for a common part family. Do not feed the live vehicle as one sprite and expect it to infer arbitrary parts, paint, state, or attachment logic. The voxelizer has no BN vehicle-part or save-state integration, and generated OBJ output would need a separate importer, fixed pivot/scale rules, BN-compatible texture/material handling, animation/state variants, and asset licensing. A focused trial should compare a generated wheel or one simple body-panel profile against a hand-built renderer mesh; stop if it loses source-art readability or needs more cleanup than the reusable mesh template.

The license and content provenance of every input image and exported texture must be checked independently. Review the voxelizer at the named release/commit before deciding whether to execute or incorporate its code; this research did not run it or inspect its implementation.

### Kenney Shape as a per-part authoring tool

Install [Kenney Shape](https://kenney.nl/tools/kenney-shape) separately. Its official
documentation describes authoring per-pixel depth and exporting OBJ, glTF, or
MagicaVoxel `.vox` files. The documented workflow focuses on drawing and depth
editing; it does not describe an automatic conversion of an existing PNG sprite.
The current tool page also limits art to 64×64 and 16 colors, and notes that the
extruded art looks the same from both sides. Those constraints make it a possible
authoring aid for an isolated wheel or other compact vehicle part, not a way to
replace the live vehicle assembled from BN parts.

The renderer now has a pure `.vox` reader and exposed-face extractor in
[`r3d_vox.cpp`](../../../../src/r3d_vox.cpp), and can load palette-colored `.vox`
models named by native part ID from `data/raw/r3d/vehicle_parts/`. That path is
currently used by bundled `wheel.vox` and `wheel_wide.vox`, with procedural geometry
as the fallback. Matching main parts replace their procedural
body mesh with the authored model, scaled to that mount and modulated by the
vehicle's current light; broken or open parts keep their state-specific sprite
path. Kenney Shape's `.vox` output can exercise this path, but source-art
licensing still applies to any tileset sprite used to author the model.

Kenney Shape is a Unity application, so the [MelonLoader project](https://github.com/LavaGang/MelonLoader)
is the automation route used by the separate
[Go asset CLI](https://github.com/r-melvin/Cataclysm-BN-3D-Asset-Pipeline).
That project's README at `b4e0493` records a Linux Mono headless export smoke test
and unverified Windows/macOS application exports. This audit did not rerun those
tests. The adapter exports existing Kenney projects; it does not create them from
PNGs. These are offline authoring tools, not renderer runtime dependencies.

The related [2d-to-3d-voxelizer](https://github.com/GazPrash/2d-to-3d-voxelizer)
offers a complementary experiment: its documented quad mode accepts `[LEFT,
FRONT, RIGHT, BACK]` sprite views and outputs OBJ. That could help create true
four-way character geometry, while Kenney Shape's `.vox` output fits the current
renderer ingestion path for compact vehicle parts. OBJ does not currently plug
into this renderer, and neither tool replaces BN vehicle assembly or character
equipment layering. The adapted Go CLI now exports VOX directly; OBJ import is
not required for that route. See the [asset workflow](sprite_to_voxel_pipeline.md)
for the integration boundary and the backlog for remaining trials.

## Validation matrix

| Dimension | Cases | Pass condition |
| --- | --- | --- |
| View | 2D isometric reference plus eight 3D yaw angles; near and ordinary gameplay zoom | Vehicle identity and orientation remain clear; no missing mounts, holes, inverted sides, or yaw-dependent UV swaps. |
| Construction | sedan, van, pickup/flatbed, bicycle/motorcycle, roofed utility vehicle, user/mod-built asymmetric frame | Dimensions follow installed parts; full panels stay tall, half panels stay low, frame-only cells do not become body cubes, and uncommon parts still use fallback geometry. |
| State | intact, open doors/cargo doors, broken windshield/panel, removed panel, folded or raised accessory where supported | Visual state matches the actual installed part state and simulation; no visual state changes collision or interactions. |
| Materials | several body colors, black/transparent/dark glass, wood/metal panel, roof colors, headlights/taillights, rooftop equipment | Source palette/tint is retained on the correct surfaces; missing art falls back predictably; no placeholder tile is mistaken for a material. |
| Spatial assembly | straight side, front/rear, corner mounts, adjacent roofs, disconnected accessory mounts, arbitrary vehicle yaw | Seams meet, corners clip only the intended panel, accessories stay over their mount, and body proportions do not depend on tile iteration order. |
| Visibility/light | bright and night scenes; lit and unlit occupied mounts; partial line of sight; opaque blocker; map edge; remembered/unknown region | Lighting stays coherent and the model never reveals unknown mounts or unseen contents. |
| Regression | movement, turning, collision, repair, disassembly, save/load, comparison capture with and without camera following | Vehicle simulation and controls are unchanged; captures preserve vehicle yaw and camera state. |

Use geometry tests for profile resolution, mount transforms, corner clipping, winding, material-role selection, and state transitions. Use native saved-world captures for visual judgment; the existing mesh-count tests cannot establish image quality. Compare the final screenshot gallery at identical camera settings and inspect source sprite/material differences under both shipped tilesets. Keep benchmark and comparison saves reproducible so each mesh revision can be judged against the same assembly.

## Reference links

- [Issue #4: vehicle visuals](https://github.com/r-melvin/Cataclysm-BN-3D/issues/4)
- [PR #5: complete vehicles, panels and roofs](https://github.com/r-melvin/Cataclysm-BN-3D/pull/5)
- [GazPrash 2D-to-3D voxelizer README](https://github.com/GazPrash/2d-to-3d-voxelizer/blob/main/README.md)
- [GazPrash voxelizer v1.0.4, commit `c30cb82`](https://github.com/GazPrash/2d-to-3d-voxelizer/releases/tag/v1.0.4)
- [Voxelizer tool license](https://github.com/GazPrash/2d-to-3d-voxelizer/blob/main/LICENSE)
- [Kenney Shape documentation](https://kenney.nl/knowledge-base/tools/kenney-shape-documentation)
- [Kenney Shape tool and export formats](https://kenney.nl/tools/kenney-shape)
