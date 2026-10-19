# Dramatic Shape Voxel reference review

Research date: 2026-10-18. Reference: `absol89/DramaticShapeVoxelMod` at
[`a07146b45aee0b8525efc40062d73b426f7668df`](https://github.com/absol89/DramaticShapeVoxelMod/tree/a07146b45aee0b8525efc40062d73b426f7668df).
BN baseline: `dbb2cd0712baea3639ec03abcebb005b81ad904f`.

The most useful lessons are explicit art-to-geometry profiles, carefully placed
sprite anchors, restrained contact shading, and time-bounded mesh preparation.
BN already has a suitable native renderer foundation. Preserve BN's simulation,
visibility decisions, turns, inputs, saves, and arbitrary vehicle construction;
apply these ideas inside the presentation layer.

This was source inspection of a private temporary clone, not a runtime or visual
benchmark. No reference code, generators, tests, installers, or hooks were run.
“Verified” below means implemented in the inspected source, not demonstrated on
our hardware. Recommendations and expected benefits are engineering judgments.

## What the reference actually is

The repository is a Lua/LÖVE presentation mod for the Pokémon Gen 1 recompilation
engine. It combines a voxel overworld with battle presentation, authored city
scenery, sprite providers, first-person controls, shadows, and water effects.
Its [entry point][main] registers render pipelines, and its [manifest][manifest]
declares a graphics mod with engine-internals access and conflicts with other
world renderers. This is not an engine-neutral voxel library.

Documentation is useful but unevenly current. The inspected manifest is version
1.11.1 with an engine range ending before 3.0.0; the [README][readme] still describes
1.9.0 and pre-2.0 support in its introduction. The [Astra art report][astra]
explicitly distinguishes historical captures, mocked tests, missing external
fixtures, and remaining runtime validation. Its success claims are not results
of this review.

## Findings and fit with BN

| Area | Verified reference implementation | Implication for BN |
| --- | --- | --- |
| Terrain and structural classification | [TileShape][tile-shape] resolves authored profiles alongside map walkability/water; [Structures][structures] analyzes connected art regions, repeated rows and background pixels to form volumes or objects. | Keep BN terrain/furniture flags and IDs authoritative. Add presentation overrides for ambiguous art; do not import Pokémon's collision-derived classification or its four-shade background assumptions. |
| Walls, windows and roofs | [Buildings][buildings] constructs a sampled voxel volume with separate facade and roof regions, roof bands/eaves, recessed facade pixels, exposed-shell extraction and compatible face-strip merging. | Explicit side/top texture donors and small relief features can improve readability. BN already builds thin walls, window surrounds, open door leaves and tiled wall finishes in [r3d_mesh.cpp](../../../../src/r3d_mesh.cpp); extend those paths instead of introducing full-building recognition. |
| Doors | [CasinoDoors][doors] authors recessed panels, bevels, hardware and jambs as actual geometry, with the entrance opening kept clear. | Use real thickness and recesses to avoid coplanar flicker. Its fixed entrance joinery is not a general open/close/destroyed-door system; BN door state and opening geometry remain the source of truth. |
| Props | [Structures][structures] supports thin silhouette prisms, rounded hull templates, relief, shelves, stairs and mounted masks. [Buildings][buildings] dispatches specialized models and multipart furniture. | High-value candidates are bins, chairs, shelves, lamps and machinery. Prefer a few reusable shape families plus explicit overrides to hundreds of hard-coded location models. |
| Characters and items | [SpriteBillboards][sprites] keeps characters as alpha-tested cards, caches frame meshes, honors frame dimensions and anchors, and shares geometry between ordinary, shadow and silhouette passes. [CharacterRenderers][providers] provides priority callbacks with failure isolation and fallback. | Keep existing BN layered sprites as the broad-coverage default. Improve feet/support anchors and optional per-asset representation. A future model provider should fall back to the native sprite and receive only already-approved visible entities. |
| Vehicles | [BikeDisplay][bikes] and [ShipHull][ship] are bespoke bicycle and ship models; the latter follows an engine-owned departure snapshot. No counterpart to BN's arbitrary assembled vehicle parts was identified in the inspected modules. | Learn material placement and silhouette detail, not vehicle rules. Preserve [r3d_vehicle_rules.cpp](../../../../src/r3d_vehicle_rules.cpp) and [r3d_entities.cpp](../../../../src/r3d_entities.cpp): quarterpanel/body versus cabin height, native corner variants, roof panel material and separate rooftop equipment. |
| Scale and camera | [Voxel3D.viewProjection][voxel3d] derives the orbit FOV/distance from flat-view framing and supports external camera data. [SpriteBillboards][sprites] pivots cards using explicit source anchors. | Define an art contract in tile units: footprint, base/support point, width, height and depth. BN already separates camera storey stretch from billboard size in [r3d_camera.cpp](../../../../src/r3d_camera.cpp) and [r3d_entities.cpp](../../../../src/r3d_entities.cpp). Do not infer physical height directly from transparent sprite-sheet dimensions. |
| Contact shading | [Buildings.emit][buildings] samples occupancy outside each exposed face, shades corners gently, and only merges runs whose shading interpolation remains valid. | A local, multiplicative contact-shade factor could distinguish furniture feet, wall joins and window recesses without changing gameplay illumination. Avoid seams at submap boundaries and avoid shading a plane by its own occupied cells. |
| Cast shadows and occlusion | [ShadowMap][shadows] renders a depth-tested light-space pass. [Voxel3D][voxel3d] also has an inverted-depth player silhouette with depth writes disabled. | Contact shading is the lower-cost first step. An optional avatar silhouette can supplement BN cutaways; never extend it to unseen enemies/items. Shadow casters and reflections must not expose actors outside BN visibility. |
| Chunking and lifetime | [ChunkMesher][chunks] queues prioritized coroutine jobs, checks generation numbers before publishing results, uses a shared time deadline, and releases GPU/analysis resources outside current and previous live map sets. | BN already uses submap chunks, stamps, frustum culling, eviction and independent light uploads in [r3d.cpp](../../../../src/r3d.cpp). The useful addition is measured time budgeting and prioritization, not a replacement cache. |

The reference's architectural unit is often a whole connected Pokémon map or an
authored landmark. BN has a mutable multilevel world, destructible terrain and
moving vehicles. Full-building pattern recognition, fixed location decorations,
canonical persistent map caches and assumed immutable connections would be poor
defaults here.

## Recommended next work, ordered by value and effort

1. **Create a small presentation-profile format and a representative art gallery
   (high impact, medium effort).** Start with a bin, chair, locker, window, door,
   tree, loose item and vehicle corner. Specify geometry family, footprint,
   support anchor, source face regions, depth and material roles. Resolve explicit
   profiles before the existing renderer heuristics, with unchanged fallback for
   all other content. Bind source-region overrides to a tileset or sprite identity:
   a crop valid for one tileset is not portable to another. Preserve all gameplay
   flags and transitions. This addresses recurring proportions and texture
   placement problems more directly than more isolated ID checks.

2. **Make art review reproducible (high impact, low-to-medium effort).** The
   reference has per-building source inventories and independent offline geometry
   previews ([example inventory][inventory], [generator source][generator]). Adapt
   the workflow using BN assets: source sprite next to front/side/top and eight
   orbit views, with footprint and support markers. Extend the existing
   [comparison capture workflow](../guides/renderer_3d.md), retaining actual saves,
   options and the normal refresh path. Include open/closed/broken states,
   neighboring walls, different storey scales and both shipped tilesets. Prefer
   geometry invariants plus real captures over maintaining a duplicate mesher.

3. **Add subtle local contact shading (medium-to-high impact, medium effort).**
   Prototype window recesses and furniture-to-floor contacts first. Carry the
   factor in the existing per-vertex shade/light path so BN light remains the
   controlling input. Review bright daylight, darkness, remembered tiles, cutaways
   and chunk seams. Measure vertex growth before enabling it broadly: preserving
   corner shading can limit face merging.

4. **Instrument chunk work, then replace count-only scheduling if needed
   (high responsiveness value, medium effort).** BN currently limits later builds
   by count but builds all initial visible chunks synchronously. An opt-in
   `CATA_R3D_DEBUG_CHUNKS=1` diagnostic now records per-second renderer, mesh,
   upload and light times; visible, rebuilt, deferred and failed chunk counts;
   geometry and upload volume; and resident chunk-buffer bytes. The reference's
   pump uses roughly 4.75 ms foreground slices, 6.5 ms current-body bootstrap and
   30 ms while covered; these are reference choices, not BN targets. Collect and
   compare diagnostic samples on cold start, travel, destruction and tileset
   changes before prioritizing nearby visible chunks or using a time allowance.
   Preserve BN's complete initial view before waiting
   indefinitely for input: progressive work needs redraws while idle and a safe
   interim presentation. A budget checked only between whole chunks cannot cap a
   single expensive chunk. Snapshot data and reject stale generations if work is
   later moved off-thread; never read live mutable map state unsafely.

5. **Offer an avatar-only occlusion silhouette (medium impact, low-to-medium
   effort).** Reuse the avatar's exact rendered layers/transforms with reversed
   depth comparison and no depth writes. Keep it optional alongside the existing
   cutaway. Test clothing layers, overhead levels and camera pitch. This improves
   orientation when geometry hides the player without changing world visibility.

6. **Add selected solid props and vehicle surface detail (medium impact,
   medium-to-high effort).** Use cached templates for hollow bins, chair legs,
   machinery and wheel silhouettes. Keep characters as sprites initially.
   For vehicles, use BN part flags/orientation and the current material selection;
   add visual wheel/body/window detail only after damaged parts, open doors,
   rotation, roofs, accessories and partial observation remain correct. The
   reference offers no evidence that its visibility model should replace BN's.

7. **Defer shadow maps, reflections, tilt-shift and first-person movement
   (high effort, lower immediate value).** These are substantial separate features
   with visibility, performance, UI and navigation implications. Decorative lights
   must not create gameplay light, and reflected scenes must obey BN knowledge.
   First-person analog movement in the reference is especially unsuitable for
   direct adoption into BN's turn-based movement rules. Camera-only experimentation
   is a separate presentation proposal.

## Integration boundaries and validation

Keep the native C++/SDL GPU/HLSL path. Translate concepts into small renderer-owned
helpers and data, not the reference's Lua runtime, engine wrappers or source-map
identifiers. Preserve existing entity fallback, UI overlays and 2D mode. Authored
geometry must not add collision, move actors, alter interaction targets, conceal
required indicators or modify map content to fit the artwork.

For a future implementation, test profile resolution and fallback; source crop
bounds; anchors; winding; open and broken states; vehicle material/shape decisions;
chunk invalidation after changes; and absence of newly revealed unseen entities.
Use the normal gameplay refresh path for visual regression captures. Follow BN's
required formatting and game-plus-test build workflow. This research-only change
does not require building the game, and no runtime correctness claim is made.

## Reuse and licensing

No tracked `LICENSE`, `COPYING` or similarly named license file was found in the
reference checkout; the inspected README and manifest contain no project-wide
reuse grant. This is a finding about this revision, not a complete provenance
audit of the upstream fork or every asset. GitHub explains that a public
repository without a license does not by itself grant reproduction, distribution
or derivative-work rights ([GitHub licensing guidance][licensing]).

Treat this repository as a design reference. Implement generic rendering ideas
independently using BN's own APIs and properly licensed art. Do not translate or
copy source, shaders, profiles, generated meshes or artwork into BN without
establishing permission and compatibility for those particular materials.
The reference's original/ROM/imported-art distinction also requires separate asset
provenance checks; source visibility and a statement that ROM assets are absent
are not redistribution licenses. BN's existing [license notices](../../../../LICENSE.txt)
do not grant rights to this third-party repository.

[main]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/main.lua
[manifest]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/manifest.json
[readme]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/README.md
[astra]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/docs/ASTRA_WORLD_ART.md
[tile-shape]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/TileShape.lua
[structures]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/Structures.lua
[buildings]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/Buildings.lua#L1003-L1265
[doors]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/CasinoDoors.lua
[sprites]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/SpriteBillboards.lua#L41-L130
[providers]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/CharacterRenderers.lua
[bikes]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/BikeDisplay.lua
[ship]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/ShipHull.lua
[voxel3d]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/Voxel3D.lua
[shadows]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/ShadowMap.lua#L547-L626
[chunks]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/lib/ChunkMesher.lua#L4355-L5061
[inventory]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/assets/docs/buildings/B01-unnamed-building.md
[generator]: https://github.com/absol89/DramaticShapeVoxelMod/blob/a07146b45aee0b8525efc40062d73b426f7668df/tools/building_voxels.py
[licensing]: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository
