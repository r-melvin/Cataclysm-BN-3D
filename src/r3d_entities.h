#pragma once
#ifndef CATA_SRC_R3D_ENTITIES_H
#define CATA_SRC_R3D_ENTITIES_H

#if defined(TILES)

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "coordinates.h"
#include "r3d_camera.h"
#include "r3d_mesh.h"
#include "r3d_presentation_profiles.h"
#include "r3d_vox.h"

class cata_tiles;

namespace r3d
{

struct vox_asset {
    vox_model model;
    vox_surface surface;
    sprite_ref palette;
};

/// Largest depth pull of a billboard top, in stretched units; the draw passes it as pull.w.
constexpr auto max_billboard_pull = static_cast<float>( 8.0f );

struct entity_view {
    cata_tiles &tiles;
    tile_info_cache &cache;
    const camera &cam;
    tripoint_bub_ms center{}; // the viewed tile, which is the origin of the scene frame
    int z_lo = 0;
    int z_hi = 0;
    /// Presentation colour (RGBA8); may include remembered or unexplored terrain tints.
    std::function<uint32_t( const tripoint_bub_ms & )> light{};

    /// UI marker (cursor, highlight, line) laid on top of a tile's surface, always unlit.
    struct marker {
        tripoint_bub_ms pos;
        float top = 0.0f;           // surface height within the tile, see column_height
        const tile_info *sprite = nullptr;
        bool box = false;           // also draw the sprite on the sides of a box above the tile
        // also draw the sprite on the sides of a box above the tile
        bool large = false;         // first person: a full-height box rather than a small post
        // First person: a sound the player heard but cannot see, drawn as a post with normal
        // depth (walls and creatures in front hide it; the compass points at hidden ones).
        bool heard = false;
        uint32_t color = 0xFFFFFFFFu;
    };
    std::vector<marker> markers{};

    /// Upright sprites of the visible chunks (trees, lockers), at a tile of the map.
    struct standing {
        tripoint_bub_ms pos;
        float z_base = 0.0f; // above the tile's level
        const tile_info *info = nullptr;
    };
    std::vector<standing> uprights{};

    /// Height within its level of what stands on a tile: the floor, or a bench or counter top.

    /// Small camera-facing sprites (rain, snow), unlit by the map: their colour is given.
    struct particle {
        float x = 0.0f; // scene-frame tile space (x east, y south, z up in levels)
        float y = 0.0f;
        float z = 0.0f;
        float size = 0.3f; // width and height in tiles
        const sprite_ref *sprite = nullptr;
        uint32_t color = 0;
    };
    std::vector<particle> particles{};

    /// Height within its level of what stands on a tile: the floor, or a bench or counter top.
    std::function<float( const tripoint_bub_ms & )> surface{};
    /// Optional MagicaVoxel models keyed by native vehicle part ID.
    const std::unordered_map<std::string, vox_asset> *vehicle_vox_parts = nullptr;
    /// Optional MagicaVoxel models keyed by item type ID.
    const std::unordered_map<std::string, vox_asset> *item_vox_models = nullptr;
    /// Optional shared proportions, support anchors, and material-region annotations.
    const presentation_profile_map *presentation_profiles = nullptr;
    /// Build only the avatar's existing sprite layers for the occlusion-silhouette pass.
    bool avatar_only = false;
    /// Current BN perception, separate from terrain tint. Required by live vehicle views;
    /// synthetic mesh fixtures may omit this and use their supplied light instead.
    std::function<bool( const tripoint_bub_ms & )> vehicle_mount_visible = {};

    /// First person: sprites stand upright facing the screen (no lean, no depth pull), items are
    /// small stands, markers are posts, and the viewer's own sprite is left out.
    bool upright_billboards = false;
    /// Tile currently hovered by the mouse cursor for 3D item highlighting.
    std::optional<tripoint_bub_ms> hovered{};
    /// Tile of the currently targeted creature in ranged targeting mode (for VATS red highlight).
    std::optional<tripoint_bub_ms> targeted_creature{};
    /// Build only the 4-way expanded silhouette outline of the targeted creature.
    bool target_outline_only = false;
    /// Build only the targeted creature's sprite layers for VATS red highlight blending.
    bool target_body_only = false;
    /// Tile of a creature briefly flashing on projectile impact.
    std::optional<tripoint_bub_ms> hit_flash_creature{};
    uint32_t hit_flash_color = 0xFFFFFFFFu;

    /// Elevated 3D laser sightlines and transient bullet tracer ribbons in scene-frame tile space
    /// (x east, y south, z up in levels).
    struct beam_segment {
        vec3 from{};
        vec3 to{};
        float width = 0.032f;
        uint32_t color = 0xFFFFFFFFu;
        const sprite_ref *sprite = nullptr;
    };
    std::vector<beam_segment> beams{};
};

/// Vehicles in the scene frame, rebuilt each frame for rigid-body rotation and animation.
auto build_vehicle_mesh( const entity_view &view ) -> chunk_mesh;

/// Billboards for creatures, the player and loose items near the viewed tile, in the scene frame
/// (so drawn with a zero offset). Rebuilt every frame since they move.
auto build_entity_mesh( const entity_view &view ) -> chunk_mesh;

struct sprite_opaque_bounds {
    float local_r_min = -0.5f;
    float local_r_max = 0.5f;
    float local_f_min = -0.5f;
    float local_f_max = 0.5f;
    bool has_trim = false;
};

/// Local tile-space footprint, accounting for sprite size, anchor offsets, flip and rotation.
auto get_sprite_opaque_bounds( const sprite_ref &s ) -> sprite_opaque_bounds;

/// Camera-facing quads for view.particles, in the scene frame. Kept apart from the entities: they
/// are drawn with dithered transparency (block_uniforms::fog2).
auto build_particle_mesh( const entity_view &view ) -> chunk_mesh;

} // namespace r3d

#endif // defined(TILES)

#endif // CATA_SRC_R3D_ENTITIES_H
