#pragma once
#ifndef CATA_SRC_R3D_MESH_H
#define CATA_SRC_R3D_MESH_H

#if defined(TILES)

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "coordinates.h"
#include "r3d_furniture_profiles.h"
#include "r3d_prop_geometry.h"
#include "r3d_fixture_geometry.h"
#include "r3d_gpu.h"
#include "r3d_presentation_profiles.h"
#include "r3d_sprite_voxels.h"
#include "r3d_shapes.h"
#include "r3d_wall_rules.h"
#include "type_id.h"

class cata_tiles;
class submap;
enum TILE_CATEGORY : int;

namespace r3d
{

/// A tileset sprite as sampled by the 3D renderer: its sheet and the uv rectangle in it.
struct sprite_ref {
    SDL_Texture *sheet = nullptr;
    // Holds the sheet alive while it is cached, so a tileset reload can never leave a dangling
    // pointer here or in a chunk's mesh_batch (chunks are dropped whenever the cache is cleared).
    std::shared_ptr<SDL_Texture> owner{};
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float size_x = 1.0f; // sprite size in tiles; tall sprites (size_y > size_x) overflow upwards
    float size_y = 1.0f;
    int quarter_turns = 0; // clockwise, applied to top faces like the flat view turns the sprite
    int facing = 0;        // clockwise quarter turns the front of the thing faces from south
    // how far the flat view shifts the sprite from its tile, in tiles (x right, y down), so
    // layers of different sheet heights line up as they do there
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    bool flip = false;     // mirrored horizontally
    uint32_t top_color = 0;  // average colours from the tileset loader (see sprite_scan)
    uint32_t body_color = 0;
    std::optional<std::array<float, 2>> solid_uv{};
    bool placeholder = false;
    std::shared_ptr<const std::vector<uint8_t>> alpha_mask{};
    int alpha_width = 0;
    int alpha_height = 0;
    std::shared_ptr<const sprite_voxel_surface> voxel_surface{};
    // opaque part of the sprite as fractions of it (whole sprite when unknown)
    float opaque_x0 = 0.0f;
    float opaque_y0 = 0.0f;
    float opaque_x1 = 1.0f;
    float opaque_y1 = 1.0f;
    bool valid = false;
};

struct tile_info {
    shape sh;
    solid_prop prop = solid_prop::none;
    bool radio_tower = false;
    bool stair_flight = false;
    bool stairs_down = false;
    sprite_ref backdrop; // opaque background layer, drawn first when the tileset has one
    sprite_ref sprite;   // foreground layer (alpha tested, may be sparse)
    furniture_proportions proportions;
    float base_offset = 0.0f; // vertical contact offset for authored furniture
    float depth_scale = 1.0f; // furniture depth, anchored at its back after wall fitting
    bool tree  = false;   // the level above holds its treetop, which this sprite already shows
    bool stairs = false;   // native vertical transition, including ladders and hatches
    sprite_ref side;     // texture for the sides of a wall (see wall_sides), else invalid
    sprite_ref cap;      // the top of such a wall: a plain patch of the same texture
    sprite_ref front;    // optional authored furniture front; preserves native art when absent
    // >= 0: the wall's sprite is a front elevation (tilesets draw painted walls as a band of
    // framing over coving, paint and skirting); rows below this fraction of the sprite are the
    // side, from coving to skirting, and the rows above it top the wall
    float elevation_split = -1.0f;
    // the sprite those rows come from: the tileset's east-west edge piece of the wall, which
    // shows the wall edge-on (a top-down tileset draws it as cavity, coving, paint, skirting)
    sprite_ref elevation;
    bool brick_wall = false; // masonry can have a separate finish on room-facing sides
    bool wall_connection = false; // walls and door/window openings, excluding solid props
    bool thin_wall = false; // constructed walls share the door/window profile
    bool window = false; // a pane with a wall surround rather than a full-height door
    bool door = false;   // a door in a wall line, open or closed
    bool open_door = false; // an open door or empty frame
    ter_id closed_door_id; // closed counterpart for open doors
    uint32_t side_tint = 0xffffffu; // 0xRRGGBB colours for the side texture and the cap
    uint32_t cap_tint = 0xffffffu;
    bool see_through = false; // transparent terrain (fences, bars): the floor shows through it

    /// The layer that represents the tile when only one can be drawn (upright sprites).
    auto main() const -> const sprite_ref & {
        return sprite.valid ? sprite : backdrop;
    }
};

/// A sprite that stands on a tile and turns towards the camera; emitted per frame.
struct upright_sprite {
    uint8_t lx = 0; // chunk-local tile
    uint8_t ly = 0;
    float z_base = 0.0f;
    const tile_info *info = nullptr; // owned by the tile_info_cache, like the chunk's sheets
};

/// Side textures for walls: an atlas on the renderer and which wall gets which cell.
struct wall_sides {
    std::shared_ptr<SDL_Texture> atlas;
    int width = 0;
    int height = 0;
    int cell = 0;
    std::vector<std::string> names; // atlas cells in order
    wall_texture_rules rules;
    // The cells are grey; a wall takes its colours from the tileset's sprite, or else from here
    // (0xRRGGBB by cell name).
    std::unordered_map<std::string, uint32_t> colors;
};

/// Per terrain/furniture id lookups (shape + sprite), so the mesher does not walk the tileset
/// once per tile. Must be cleared whenever the tileset or the renderer's textures change.
class tile_info_cache
{
    public:
        /// Forgets every lookup; wall sides and the renderer-owned model palette stay.
        auto clear() -> void;
        auto fixture_palette() -> const sprite_ref &; // *NOPAD*
        auto set_fixture_palette( sprite_ref palette ) -> void;
        /// Side textures for walls from now on (nullptr for the tileset sprite); clears.
        auto set_wall_sides( std::shared_ptr<const wall_sides> sides ) -> void;
        /// Whether painted and plain walls take the atlas's plaster texture (true) or their own
        /// sprite read as a front elevation (false). Clears the cache when it changes.
        auto set_plaster_atlas( bool on ) -> void;
        /// Whether item sprites keep their alpha mask and voxel surface. Clears when it changes.
        auto set_voxel_items( bool on ) -> void;
        /// Switch wall profiles and clear cached geometry inputs.
        auto set_thin_walls( bool on ) -> void;
        /// Install newly prepared wall-face donors and invalidate derived texture lookups.
        auto set_presentation_profiles( presentation_profile_map profiles ) -> void;
        auto wall_half_width() const -> float {
            return thin_walls_ ? wall_half_thickness : 0.5f;
        }
        /// With subtile/rota (see cata_tiles::get_orientation) the sprites are those the flat view
        /// draws for that connection; the shape never depends on them.
        struct terrain_options {
            ter_id id;
            cata_tiles &tiles;
            int subtile = -1;
            int rota = 0;
        };

        auto terrain( const terrain_options &options ) -> const tile_info &; // *NOPAD*
        struct furniture_options {
            furn_id id;
            cata_tiles &tiles;
            int subtile = -1;
            int rota = 0;
        };

        auto furniture( const furniture_options &options ) -> const tile_info &; // *NOPAD*
        /// Sprite layers by tileset id, e.g. a monster type or "player_male".
        /// Subtile, rotation and variant are part of the cache key (vehicle part state).
        struct named_options {
            TILE_CATEGORY category;
            const std::string &id;
            cata_tiles &tiles;
            int subtile = -1;
            int rota = 0;
            const std::string &variant = "";
            bool allow_unknown = true;
        };

        auto named( const named_options &options ) -> const tile_info &; // *NOPAD*
        /// Ground drawn under trees and bushes, which are terrain with a transparent background.
        auto ground( cata_tiles &tiles ) -> const tile_info &;
        /// Plain plaster for ceilings: the middle of the wall texture "plaster". Invalid when wall
        /// textures are off. tint is 0xRRGGBB, multiplied into the (grey) texture.
        auto ceiling( uint32_t &tint ) const -> sprite_ref;

    private:
        presentation_profile_map profiles_;
        std::unordered_map<int, tile_info> terrain_;
        std::unordered_map<int, tile_info> furniture_;
        std::unordered_map<std::string, tile_info> named_;
        tile_info ground_;
        sprite_ref fixture_palette_;
        bool ground_loaded_ = false;
        std::shared_ptr<const wall_sides> wall_sides_;
        bool plaster_atlas_ = false;
        bool voxel_items_ = false;
        bool thin_walls_ = true;
};

/// Where a vertex gets its light from: the tile at (lx + dx, ly + dy) on level (chunk z + dz).
struct light_ref {
    uint8_t lx = 0;
    uint8_t ly = 0;
    int8_t dx = 0;
    int8_t dy = 0;
    int8_t dz = 0;
    uint8_t shade = 255; // directional shading baked into the face, 255 = full brightness
    // colour multiplied into the light (wall textures take the tileset's colours), 255 = none
    uint8_t tint_r = 255;
    uint8_t tint_g = 255;
    uint8_t tint_b = 255;
    // a floor top: shown as dark floor rather than left out where its tile was never seen,
    // so unexplored rooms are a dark floor between their seen walls, not a hole
    bool floor = false;
    // a wall's face: drawn dark like a floor where its tile was never seen but borders known
    // ground, so the walls at the edge of what is known do not end in mid-air
    bool wall = false;
};

struct mesh_batch {
    SDL_Texture *sheet = nullptr;
    uint32_t first_index = 0;
    uint32_t index_count = 0;
};

/// CPU-side geometry of one submap at one z-level, in chunk-local world coordinates.
struct chunk_mesh {
    std::vector<vertex> vertices;
    std::vector<uint32_t> light;      // RGBA8 per vertex, parallel to vertices
    std::vector<light_ref> light_refs; // parallel to vertices
    std::vector<uint32_t> indices;
    std::vector<mesh_batch> batches;
    std::vector<upright_sprite> uprights;

    auto empty() const -> bool {
        return indices.empty() && uprights.empty();
    }
};

using corners_t = std::array<std::array<float, 3>, 4>;
using uvs_t = std::array<std::array<float, 2>, 4>;

/// Collects textured quads and turns them into one indexed mesh grouped by sprite sheet. Corners
/// are given in tile space (x east, y south, z up) and flipped into the world frame here.
class quad_sink
{
    public:
        /// Quad lit by its shade only; the light stream can later be recomputed from ref.
        struct add_options {
            const sprite_ref &sprite;
            const corners_t &corners;
            const uvs_t &uvs;
            const light_ref &ref;
        };

        auto add( const add_options &options ) -> void;
        /// Quad with an explicit light colour (used by the dynamic entity geometry).
        struct add_colored_options {
            const sprite_ref &sprite;
            const corners_t &corners;
            const uvs_t &uvs;
            const light_ref &ref;
            uint32_t color;
        };

        auto add_colored( const add_colored_options &options ) -> void;
        /// Same with a colour per corner (the alpha carries the billboards' depth pull).
        struct add_colored_options2 {
            const sprite_ref &sprite;
            const corners_t &corners;
            const uvs_t &uvs;
            const light_ref &ref;
            const std::array<uint32_t, 4> &colors;
        };

        auto add_colored( const add_colored_options2 &options ) -> void;
        auto finish() -> chunk_mesh;

    private:
        struct quad {
            SDL_Texture *sheet = nullptr;
            std::array<vertex, 4> v;
            std::array<light_ref, 4> r;
            std::array<uint32_t, 4> colors{};
        };
        std::vector<quad> quads_;
};

using submap_lookup = std::function<const submap *( const tripoint_abs_sm & )>;

/// Combined change stamp of a submap and the neighbours whose shapes affect its faces.
auto chunk_stamp( const tripoint_abs_sm &pos, const submap_lookup &lookup ) -> uint64_t;

/// Packs a grey level into the RGBA8 light format of stream 1.
auto pack_light( uint8_t r, uint8_t g, uint8_t b ) -> uint32_t;

/// What a chunk's mesh contains besides the basics; changing it means rebuilding every chunk.
struct mesh_options {
    /// The first-person view: stairs get an upright copy of their sprite (visible at eye level).
    bool first_person = false;
    /// Ceilings under storeys above, which are only seen from below (so only first person).
    bool ceilings = false;
    /// The first visible storey owns the remaining tower column above it.
    int lowest_visible_z = std::numeric_limits<int>::lowest();
};

/// Meshes the terrain and furniture of one submap. Returns an empty mesh if it is not loaded.

/// Meshes the terrain and furniture of one submap. Returns an empty mesh if it is not loaded.
struct build_chunk_mesh_options {
    const tripoint_abs_sm &pos;
    const submap_lookup &lookup;
    tile_info_cache &cache;
    cata_tiles &tiles;
    mesh_options config{};
};

auto build_chunk_mesh( const build_chunk_mesh_options &options ) -> chunk_mesh;

} // namespace r3d

#endif // defined(TILES)

#endif // CATA_SRC_R3D_MESH_H
