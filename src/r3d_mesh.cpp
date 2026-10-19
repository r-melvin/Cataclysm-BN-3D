#if defined(TILES)

#include "r3d_mesh.h"
#include "r3d_door_geometry.h"
#include "r3d_sprite_voxels.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>

#include "cata_tiles.h"
#include "debug.h"
#include "map/map.h"
#include "game_constants.h"
#include "map/mapdata.h"
#include "point.h"
#include "map/submap.h"

namespace r3d
{

namespace
{

constexpr auto side = static_cast<int>( SEEX ); // SEEX == SEEY
constexpr auto border = static_cast<int>( side + 2 );

// Light multiplier that turns a grey wall texture cell (whose average is about grey_mean, see
// make_wall_textures.py) into the given colour: the tileset's (0xAARRGGBB, alpha 0 when unknown)
// or else the preset (0xRRGGBB).
// A packed colour scaled by factor, each channel clamped to 255.
auto lighten( uint32_t rgb, float factor ) -> uint32_t
{
    auto out = static_cast<uint32_t>( 0 );
    for( auto shift = static_cast<int>( 0 ); shift <= 16; shift += 8 ) {
        const auto c = static_cast<float>( static_cast<float>( ( rgb >> shift ) & 255u ) * factor );
        out |= static_cast<uint32_t>( std::clamp( c, 0.0f, 255.0f ) + 0.5f ) << shift;
    }
    return out;
}

auto wall_tint( uint32_t tileset_color, uint32_t preset ) -> uint32_t
{
    constexpr auto grey_mean = static_cast<float>( 200.0f );
    // The material's own colour, leaning a little towards the tileset's sprite: a painted wall
    // keeps a hint of its paint without turning a whole room lime green.
    constexpr auto sprite_share = static_cast<float>( 0.3f );
    const auto has_sprite = static_cast<bool>( ( tileset_color >> 24 ) != 0 );
    auto out = static_cast<uint32_t>( 0 );
    for( int shift : {
             16, 8, 0
         } ) {
        const auto base = static_cast<float>( ( preset >> shift ) & 255u );
        const auto sprite = static_cast<float>( ( tileset_color >> shift ) & 255u );
        const auto mixed = static_cast<float>( has_sprite ? base * ( 1.0f - sprite_share ) + sprite
                                               * sprite_share : base );
        const auto c = static_cast<float>( mixed * 255.0f / grey_mean );
        out |= static_cast<uint32_t>( std::min( c, 255.0f ) ) << shift;
    }
    return out;
}

auto traits_of( const map_data_common_t &d ) -> tile_traits
{
    auto t = tile_traits{};
    t.no_floor = d.has_flag( TFLAG_NO_FLOOR );
    t.wall = d.has_flag( TFLAG_WALL );
    t.transparent = d.transparent;
    t.tree = d.has_flag( TFLAG_TREE );
    t.shrub = d.has_flag( TFLAG_SHRUB );
    t.flat = d.has_flag( TFLAG_FLAT );
    t.tiny = d.has_flag( "TINY" );
    t.low = d.has_flag( "SHORT" );
    t.flat_surface = d.has_flag( "FLAT_SURF" );
    t.window = d.has_flag( "WINDOW" );
    t.container = d.has_flag( "CONTAINER" );
    t.seat = d.has_flag( "CAN_SIT" );
    // Locked and damaged doors lack DOOR but are barricadable as doors.
    t.door = d.has_flag( "DOOR" ) || d.has_flag( "BARRICADABLE_DOOR" ) ||
             d.has_flag( "BARRICADABLE_DOOR_DAMAGED" ) || d.has_flag( "BARRICADABLE_DOOR_REINFORCED" );
    t.open_door = t.door && ( t.flat || d.movecost > 0 );
    t.move_cost = d.movecost;
    return t;
}

auto to_sprite_ref( const texture_draw_data &data ) -> sprite_ref
{
    if( !data.texture ) {
        return {};
    }
    auto tex_w = static_cast<float>( 0.0f );
    auto tex_h = static_cast<float>( 0.0f );
    if( !SDL_GetTextureSize( data.texture, &tex_w, &tex_h ) || tex_w <= 0.0f || tex_h <= 0.0f ) {
        return {};
    }
    // Half a texel in from each edge so nearest sampling never picks up a neighbouring sprite.
    const auto half_u = static_cast<float>( 0.5f / tex_w );
    const auto half_v = static_cast<float>( 0.5f / tex_h );
    auto ref = sprite_ref{};
    ref.sheet = data.texture;
    ref.owner = data.owner;
    ref.u0 = data.uv0.first + half_u;
    ref.v0 = data.uv0.second + half_v;
    ref.u1 = data.uv1.first - half_u;
    ref.v1 = data.uv1.second - half_v;
    ref.size_x = data.size_x;
    ref.size_y = data.size_y;
    ref.top_color = data.top_color;
    ref.body_color = data.body_color;
    if( data.alpha_mask ) {
        auto alpha = orient_sprite_alpha( { .width = data.dimensions.w, .height = data.dimensions.h,
                                            .alpha = *data.alpha_mask, .quarter_turns = data.quarter_turns,
                                            .flip = data.flip } );
        if( alpha.width > 0 && alpha.height > 0 ) {
            ref.alpha_width = alpha.width;
            ref.alpha_height = alpha.height;
            ref.alpha_mask = std::make_shared<const std::vector<uint8_t>>( std::move( alpha.pixels ) );
        }
    }
    ref.quarter_turns = data.quarter_turns;
    ref.facing = data.facing;
    // size is in tiles and dimensions in pixels, so a tile is dimensions / size pixels.
    const auto tile_w = static_cast<float>( static_cast<float>( std::max( 1,
                                            data.dimensions.w ) ) / std::max( data.size_x,
                                                    0.01f ) );
    const auto tile_h = static_cast<float>( static_cast<float>( std::max( 1,
                                            data.dimensions.h ) ) / std::max( data.size_y,
                                                    0.01f ) );
    ref.offset_x = static_cast<float>( data.offset_x ) / tile_w;
    ref.offset_y = static_cast<float>( data.offset_y ) / tile_h;
    ref.flip = data.flip;
    if( data.opaque.w > 0 && data.opaque.h > 0 && data.dimensions.w > 0 && data.dimensions.h > 0 ) {
        const auto w = static_cast<float>( data.dimensions.w );
        const auto h = static_cast<float>( data.dimensions.h );
        ref.opaque_x0 = static_cast<float>( data.opaque.x ) / w;
        ref.opaque_y0 = static_cast<float>( data.opaque.y ) / h;
        ref.opaque_x1 = static_cast<float>( data.opaque.x + data.opaque.w ) / w;
        ref.opaque_y1 = static_cast<float>( data.opaque.y + data.opaque.h ) / h;
    }
    ref.valid = true;
    return ref;
}

struct resolve_layers_options {
    cata_tiles &tiles;
    const std::string &id;
    TILE_CATEGORY category;
    tile_info &out;
    int subtile = -1;
    int rota = 0;
    const std::string &variant = "";
    bool allow_unknown = true;
};

auto resolve_layers( const resolve_layers_options &options ) -> void
{
    auto &tiles = options.tiles;
    const auto &id = options.id;
    auto category = options.category;
    auto &out = options.out;
    auto subtile = options.subtile;
    auto rota = options.rota;
    const auto &variant = options.variant;

    const auto origin = tripoint_bub_ms( 0, 0, 0 );
    if( const auto bg = std::optional<texture_draw_data>( tiles.get_sprite_draw_data( { .id = id, .category = category, .pos = origin, .background = true, .subtile = subtile, .rotation = rota, .variant = variant } ) ) ) {
        out.backdrop = to_sprite_ref( *bg );
    }
    if( const auto fg = std::optional<texture_draw_data>( tiles.get_sprite_draw_data( { .id = id, .category = category, .pos = origin, .background = false, .subtile = subtile, .rotation = rota, .variant = variant } ) ) ) {
        out.sprite = to_sprite_ref( *fg );
    }
    if( options.allow_unknown && !out.backdrop.valid && !out.sprite.valid ) {
        if( const auto unknown = std::optional<texture_draw_data>( tiles.get_sprite_draw_data( { .id = std::string( "unknown" ), .category = C_NONE, .pos = origin } ) ) ) {
            out.sprite = to_sprite_ref( *unknown );
        }
    }
    if( category == C_ITEM && out.sprite.alpha_mask ) {
        const auto geometry = voxelize_sprite_alpha( out.sprite.alpha_width, out.sprite.alpha_height,
                              *out.sprite.alpha_mask );
        out.sprite.voxel_surface = std::make_shared<const sprite_voxel_surface>( geometry );
    }
}

auto combine( const shape &terrain, const shape &furniture ) -> shape
{
    if( terrain.kind == shape_kind::block ) {
        return terrain;
    }
    if( furniture.kind == shape_kind::prism || furniture.kind == shape_kind::block ) {
        return furniture;
    }
    return terrain;
}

// Light from above and the south-west, with clearly different faces as isometric art shades
// them: that contrast is what makes boxes read as solid. top N S W E
struct point_f {
    float x;
    float y;
};

constexpr auto face_shade = std::array<uint8_t, 5> { 255, 165, 212, 150, 185 };

// Faces are wound clockwise as seen from outside (in the world frame, see quad_sink::add), so
// the pipeline can leave their backs out. A face meant to be seen from both sides is added
// twice, the second time turned with this.
template<typename T>
auto reversed_winding( const T &quad ) -> T
{
    auto out = T( quad );
    std::swap( out[1], out[3] );
    return out;
}

// All faces use the full sprite horizontally. Vertically the sprite spans one tile height starting
// at z_base, so a shorter box shows only the lower part of it.
// front: for a 3/4-view furniture sprite, the fraction at its bottom that shows the front; the
// top face then uses the rest. Negative for ordinary top-down sprites.
struct emit_top_options {
    quad_sink &sink;
    const sprite_ref &s;
    int x;
    int y;
    float z;
    const light_ref &ref;
    float front = -1.0f;
};

auto emit_top( const emit_top_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x = options.x;
    auto y = options.y;
    auto z = options.z;
    const auto &ref = options.ref;
    auto front = options.front;

    const auto x0 = static_cast<float>( x );
    const auto y0 = static_cast<float>( y );
    const auto c = corners_t{{ { x0, y0, z }, { x0 + 1, y0, z }, { x0 + 1, y0 + 1, z }, { x0, y0 + 1, z } }};
    // A tall sprite stands on its bottom square and overflows upwards; only that square is the
    // tile seen from above.
    auto v0 = static_cast<float>( s.size_y > s.size_x ? s.v1 - ( s.v1 - s.v0 ) * s.size_x / s.size_y :
                                  s.v0 );
    auto v1 = static_cast<float>( s.v1 );
    if( front >= 0.0f ) {
        v0 = s.v0;
        v1 = s.v1 - ( s.v1 - s.v0 ) * front;
    }
    const auto ul = static_cast<float>( s.flip ? s.u1 : s.u0 );
    const auto ur = static_cast<float>( s.flip ? s.u0 : s.u1 );
    const auto base = uvs_t{{ { ul, v0 }, { ur, v0 }, { ur, v1 }, { ul, v1 } }};
    // Turning the sprite clockwise shows, at each corner, the sprite corner a quarter turn back.
    auto uv = uvs_t{};
    for( auto i = size_t( 0 ); i < 4; ++i ) {
        uv[i] = base[( i + 4 - static_cast<size_t>( s.quarter_turns & 3 ) ) % 4];
    }
    sink.add( { .sprite = s, .corners = c, .uvs = uv, .ref = ref } );
}

auto is_tall( const sprite_ref &s ) -> bool
{ return s.valid && s.size_y > s.size_x; }

// Rows of a sprite shown on a side, as fractions of its height from the top (0) to the bottom
// (1). Unset (negative) means the bottom of the sprite, one tile per unit of box height.
struct sprite_band {
    float top = -1.0f;
    float bottom = -1.0f;
};

// The corners of a side face of the cell at (x0, y0): top-left, top-right, bottom-right,
// bottom-left as seen from outside the box, so sprites are never mirrored.
struct side_corners_options {
    face_bit face;
    float x0;
    float y0;
    float z_base;
    float z_top;
    corners_t &c;
};

auto side_corners( const side_corners_options &options ) -> bool
{
    auto face = options.face;
    auto x0 = options.x0;
    auto y0 = options.y0;
    auto z_base = options.z_base;
    auto z_top = options.z_top;
    auto &c = options.c;

    auto lx = static_cast<float>( 0.0f );
    auto ly = static_cast<float>( 0.0f );
    auto rx = static_cast<float>( 0.0f );
    auto ry = static_cast<float>( 0.0f );
    switch( face ) {
        case face_south:
            lx = x0;
            ly = y0 + 1;
            rx = x0 + 1;
            ry = y0 + 1;
            break;
        case face_north:
            lx = x0 + 1;
            ly = y0;
            rx = x0;
            ry = y0;
            break;
        case face_east:
            lx = x0 + 1;
            ly = y0 + 1;
            rx = x0 + 1;
            ry = y0;
            break;
        case face_west:
            lx = x0;
            ly = y0;
            rx = x0;
            ry = y0 + 1;
            break;
        default:
            return false;
    }
    c = {{ { lx, ly, z_top }, { rx, ry, z_top }, { rx, ry, z_base }, { lx, ly, z_base } }};
    return true;
}

struct emit_side_options {
    quad_sink &sink;
    const sprite_ref &s;
    int x;
    int y;
    face_bit face;
    float z_base;
    float z_top;
    const light_ref &ref;
    float shift_x = 0.0f;
    float shift_y = 0.0f;
    const sprite_band &band = {};
};

auto emit_side( const emit_side_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x = options.x;
    auto y = options.y;
    auto face = options.face;
    auto z_base = options.z_base;
    auto z_top = options.z_top;
    const auto &ref = options.ref;
    auto shift_x = options.shift_x;
    auto shift_y = options.shift_y;
    const auto &band = options.band;

    const auto x0 = static_cast<float>( static_cast<float>( x ) + shift_x );
    const auto y0 = static_cast<float>( static_cast<float>( y ) + shift_y );
    const auto dv = static_cast<float>( s.v1 - s.v0 );
    const auto banded = static_cast<bool>( band.top >= 0.0f );
    const auto v_top = static_cast<float>( banded ? s.v0 + dv *band.top : s.v1 - dv *
                                           ( z_top - z_base ) );
    const auto v_bot = static_cast<float>( banded ? s.v0 + dv *band.bottom : s.v1 );
    auto c = corners_t{};
    if( !side_corners( { .face = face, .x0 = x0, .y0 = y0, .z_base = z_base, .z_top = z_top, .c = c } ) ) {
        return;
    }
    const auto uv = uvs_t{{ { s.u0, v_top }, { s.u1, v_top }, { s.u1, v_bot }, { s.u0, v_bot } }};
    sink.add( { .sprite = s, .corners = c, .uvs = uv, .ref = ref } );
}

// Upright panel through the middle of the cell, along x (along_x) or along y, one tile high.
// Door or window standing in the wall line: a thin slab through the middle of the cell, along
// x (along_x) or y, with the sprite on both broad faces.
struct emit_pane_options {
    quad_sink &sink;
    const sprite_ref &s;
    int x;
    int y;
    bool along_x;
    float z_base;
    float height;
    const light_ref &ref;
    float half_thickness = wall_half_thickness;
    float half_width = 0.5f;
};

auto emit_pane( const emit_pane_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x = options.x;
    auto y = options.y;
    auto along_x = options.along_x;
    auto z_base = options.z_base;
    auto height = options.height;
    const auto &ref = options.ref;
    auto half_thickness = options.half_thickness;
    auto half_width = options.half_width;

    const auto cx = static_cast<float>( static_cast<float>( x ) + 0.5f );
    const auto cy = static_cast<float>( static_cast<float>( y ) + 0.5f );
    const auto z_top = static_cast<float>( z_base + height );
    const auto uv = uvs_t{{ { s.u0, s.v0 }, { s.u1, s.v0 }, { s.u1, s.v1 }, { s.u0, s.v1 } }};
    for( const float side : {
             -half_thickness, half_thickness
             } ) {
        const auto c = corners_t( along_x ?
        corners_t{{ { cx - half_width, cy + side, z_top }, { cx + half_width, cy + side, z_top }, { cx + half_width, cy + side, z_base }, { cx - half_width, cy + side, z_base } }} :
        corners_t{{ { cx + side, cy + half_width, z_top }, { cx + side, cy - half_width, z_top }, { cx + side, cy - half_width, z_base }, { cx + side, cy + half_width, z_base } }} );
        // Seen from either side of the wall line.
        sink.add( { .sprite = s, .corners = c, .uvs = uv, .ref = ref } );
        sink.add( { .sprite = s, .corners = reversed_winding( c ), .uvs = reversed_winding( uv ), .ref = ref } );
    }
    // The top edge, in the colour of the sprite's top rows.
    const auto v_edge = static_cast<float>( s.v0 + ( s.v1 - s.v0 ) * 0.08f );
    const auto edge_uv = uvs_t{{ { s.u0, s.v0 }, { s.u1, s.v0 }, { s.u1, v_edge }, { s.u0, v_edge } }};
    const auto top = corners_t( along_x ?
    corners_t{{ { cx - half_width, cy - half_thickness, z_top }, { cx + half_width, cy - half_thickness, z_top }, { cx + half_width, cy + half_thickness, z_top }, { cx - half_width, cy + half_thickness, z_top } }} :
    corners_t{{ { cx - half_thickness, cy - half_width, z_top }, { cx + half_thickness, cy - half_width, z_top }, { cx + half_thickness, cy + half_width, z_top }, { cx - half_thickness, cy + half_width, z_top } }} );
    sink.add( { .sprite = s, .corners = top, .uvs = edge_uv, .ref = ref } );
}

// Quarter turns (front from south, clockwise) for a piece with its back to a wall: of the sides
// across its row (any side when it stands alone), the first with a wall; 0 when none has one.
template<typename IsWall>
auto back_to_wall( bool row_x, bool row_y, IsWall is_wall ) -> int
{
    auto walls = uint8_t( 0 );
    if( is_wall( point( 0, -1 ) ) ) {
        walls |= face_north;
    }
    if( is_wall( point( 0, 1 ) ) ) {
        walls |= face_south;
    }
    if( is_wall( point( 1, 0 ) ) ) {
        walls |= face_east;
    }
    if( is_wall( point( -1, 0 ) ) ) {
        walls |= face_west;
    }
    return furniture_wall_facing( row_x, row_y, walls );
}

// A furniture box laid out by furniture_box_layout, mirrored like the flat view mirrors the
// sprite and turned clockwise facing quarter turns (its front from south). refs: top, north,
// south, west, east. faces says which sides neighbours leave open; a side that stops short of
// its tile edge is always drawn. joined (face bits) are the sides where the same furniture
// continues: the box reaches the tile edge there, so a row of lockers or a bench is one piece.
struct emit_box_options {
    quad_sink &sink;
    const sprite_ref &s;
    int x;
    int y;
    float z0;
    float z1;
    const box_layout &b;
    uint8_t faces;
    const std::array<light_ref, 5> &refs;
    int facing;
    uint8_t joined;
    uint8_t wall_contacts;
    float slab = 0.0f;
    float depth_scale = 1.0f;
};

auto emit_box( const emit_box_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x = options.x;
    auto y = options.y;
    auto z0 = options.z0;
    auto z1 = options.z1;
    const auto &b = options.b;
    auto faces = options.faces;
    const auto &refs = options.refs;
    auto facing = options.facing;
    auto joined = options.joined;
    auto wall_contacts = options.wall_contacts;
    auto slab = options.slab;
    auto depth_scale = options.depth_scale;

    if( !s.valid ) {
        return;
    }
    const auto bounds = box_layout( place_furniture_box( { .box = b, .flip = s.flip, .facing = facing, .joined = joined, .wall_contacts = wall_contacts,
                                    .depth_scale = depth_scale } ) );
    const auto fx0 = static_cast<float>( bounds.x0 );
    const auto fx1 = static_cast<float>( bounds.x1 );
    const auto fy0 = static_cast<float>( bounds.y0 );
    const auto fy1 = static_cast<float>( bounds.y1 );
    const auto ox = static_cast<float>( x );
    const auto oy = static_cast<float>( y );
    const auto du = static_cast<float>( s.u1 - s.u0 );
    const auto dv = static_cast<float>( s.v1 - s.v0 );
    const auto u0 = static_cast<float>( s.u0 + du * b.col0 );
    const auto u1 = static_cast<float>( s.u0 + du * b.col1 );

    // Top: the lid rows, turned like emit_top turns a whole sprite.
    if( ( faces & face_top ) != 0 ) {
        // A slab's top is the middle of the lid: the lid's edge rows are the sprite's outline
        // and shadow, which would read as a raised rim on a seat.
        // A slab's top is the middle of the lid in both directions: the lid's edge rows and
        // columns are the sprite's outline, shadow and the legs peeking out at its sides (a
        // top-down bench sprite draws them mid-tile), which would read as marks on the seat.
        const auto lid_span = static_cast<float>( b.lid_bottom - b.lid_top );
        const auto lv0 = static_cast<float>( s.v0 + dv * ( slab > 0.0f ? b.lid_top + lid_span * 0.2f :
                                             b.lid_top ) );
        const auto lv1 = static_cast<float>( s.v0 + dv * ( slab > 0.0f ? b.lid_bottom - lid_span * 0.15f :
                                             b.lid_bottom ) );
        const auto inset_u = static_cast<float>( slab > 0.0f ? ( u1 - u0 ) * 0.22f : 0.0f );
        const auto ul = static_cast<float>( s.flip ? u1 - inset_u : u0 + inset_u );
        const auto ur = static_cast<float>( s.flip ? u0 + inset_u : u1 - inset_u );
        const auto base = uvs_t{{ { ul, lv0 }, { ur, lv0 }, { ur, lv1 }, { ul, lv1 } }};
        auto uv = uvs_t{};
        for( auto i = size_t( 0 ); i < 4; ++i ) {
            uv[i] = base[( i + 4 - static_cast<size_t>( s.quarter_turns & 3 ) ) % 4];
        }
        const auto c = corners_t{{ { ox + fx0, oy + fy0, z1 }, { ox + fx1, oy + fy0, z1 },
                { ox + fx1, oy + fy1, z1 }, { ox + fx0, oy + fy1, z1 }
            }
        };
        sink.add( { .sprite = s, .corners = c, .uvs = uv, .ref = refs[0] } );
    }

    // A slab (bench seat, table top) only has sides as thick as itself, an underside, and legs.
    const auto zs = static_cast<float>( slab > 0.0f ? std::max( z1 - slab, z0 ) : z0 );

    // Sides: the side rows, never turned; left/right as seen from outside. Only the front (the
    // south face of the art, turned with the thing) shows them whole; the other sides repeat a
    // thin strip from the edge of the front, so locker doors appear on the front only.
    const auto sv0 = static_cast<float>( s.v0 + dv * b.side_top );
    const auto sv1 = static_cast<float>( s.v0 + dv * b.side_bottom );
    const auto side_uv = uvs_t{{ { u0, sv0 }, { u1, sv0 }, { u1, sv1 }, { u0, sv1 } }};
    // A patch from the middle of the lid: the thing's own colour without doors or handles.
    const auto lid_v0 = static_cast<float>( s.v0 + dv * b.lid_top );
    const auto lid_v1 = static_cast<float>( s.v0 + dv * b.lid_bottom );
    const auto pu0 = static_cast<float>( u0 + ( u1 - u0 ) * 0.3f );
    const auto pu1 = static_cast<float>( u0 + ( u1 - u0 ) * 0.7f );
    const auto pv0 = static_cast<float>( lid_v0 + ( lid_v1 - lid_v0 ) * 0.35f );
    const auto pv1 = static_cast<float>( lid_v0 + ( lid_v1 - lid_v0 ) * 0.65f );
    const auto plain_uv = uvs_t{{ { pu0, pv0 }, { pu1, pv0 }, { pu1, pv1 }, { pu0, pv1 } }};
    // south, west, north, east: where the front ends up after each clockwise quarter turn
    static constexpr auto front_after = std::array<face_bit, 4> {{ face_south, face_west, face_north, face_east }};
    const auto front = face_bit( front_after[static_cast<size_t>( facing & 3 )] );
    constexpr auto edge = static_cast<float>( 0.01f );
    const struct {
        face_bit face;
        bool short_of_edge;
        float lx, ly, rx, ry;
        size_t ref;
    } sides[] = {
        { .face = face_north, .short_of_edge = fy0 > edge, .lx = fx1, .ly = fy0, .rx = fx0, .ry = fy0, .ref = 1 },
        { .face = face_south, .short_of_edge = fy1 < 1.0f - edge, .lx = fx0, .ly = fy1, .rx = fx1, .ry = fy1, .ref = 2 },
        { .face = face_west, .short_of_edge = fx0 > edge, .lx = fx0, .ly = fy0, .rx = fx0, .ry = fy1, .ref = 3 },
        { .face = face_east, .short_of_edge = fx1 < 1.0f - edge, .lx = fx1, .ly = fy1, .rx = fx1, .ry = fy0, .ref = 4 },
    };
    for( const auto &sd : sides ) {
        if( ( faces & sd.face ) == 0 && !sd.short_of_edge ) {
            continue;
        }
        const auto c = corners_t{{ { ox + sd.lx, oy + sd.ly, z1 }, { ox + sd.rx, oy + sd.ry, z1 },
                { ox + sd.rx, oy + sd.ry, zs }, { ox + sd.lx, oy + sd.ly, zs }
            }
        };
        // A slab's sides are its own wood (the lid's colour), lit like any side.
        sink.add( { .sprite = s, .corners = c, .uvs = slab > 0.0f ? plain_uv : sd.face == front ? side_uv : plain_uv, .ref = refs[sd.ref] } );
    }
    if( zs <= z0 ) {
        return;
    }

    // Underside of the slab, in the shade of a side.
    const auto under = corners_t{{ { ox + fx0, oy + fy1, zs }, { ox + fx1, oy + fy1, zs },
            { ox + fx1, oy + fy0, zs }, { ox + fx0, oy + fy0, zs }
        }
    };
    sink.add( { .sprite = s, .corners = under, .uvs = side_uv, .ref = refs[3] } );

    // Legs: every tile of a bench or table stands on one pair of legs, one at each side
    // across its row (the row runs along x before the turn), in the middle of the tile.
    constexpr auto leg = static_cast<float>( 0.1f );
    constexpr auto inset = static_cast<float>( 0.05f );
    const auto across_y = static_cast<bool>( ( facing & 1 ) == 0 );
    const auto a0 = static_cast<float>( ( ( across_y ? fx0 : fy0 ) + ( across_y ? fx1 : fy1 ) - leg ) *
                                        0.5f );
    const auto s0 = static_cast<float>( ( across_y ? fy0 : fx0 ) + inset );
    const auto s1 = static_cast<float>( ( across_y ? fy1 : fx1 ) - inset - leg );
    const auto legs = std::array<std::array<float, 2>, 2> {{
            { a0, s0 }, { a0, s1 }
        }
    };
    for( const auto &pos : legs ) {
        const auto l = point_f{ .x = across_y ? pos[0] : pos[1], .y = across_y ? pos[1] : pos[0] };
        const auto lx0 = static_cast<float>( ox + l.x );
        const auto ly0 = static_cast<float>( oy + l.y );
        const auto lx1 = static_cast<float>( lx0 + leg );
        const auto ly1 = static_cast<float>( ly0 + leg );
        const auto post = std::array<std::array<float, 4>, 4> {{
                { lx1, ly0, lx0, ly0 }, { lx0, ly1, lx1, ly1 }, { lx0, ly0, lx0, ly1 }, { lx1, ly1, lx1, ly0 }
            }
        };
        for( auto i = size_t( 0 ); i < post.size(); ++i ) {
            constexpr auto foot_shade_factor = 0.72f;
            const auto foot_height = static_cast<float>( std::min( 0.07f, ( zs - z0 ) * 0.45f ) );
            const auto foot_top = static_cast<float>( z0 + foot_height );
            const auto uv_split = static_cast<float>( ( zs - foot_top ) / ( zs - z0 ) );
            auto upper_uv = side_uv;
            upper_uv[2][1] = static_cast<float>( sv0 + ( sv1 - sv0 ) * uv_split );
            upper_uv[3][1] = upper_uv[2][1];
            auto foot_uv = side_uv;
            foot_uv[0][1] = upper_uv[2][1];
            foot_uv[1][1] = upper_uv[2][1];
            const auto upper = corners_t{{ { post[i][0], post[i][1], zs }, { post[i][2], post[i][3], zs },
                    { post[i][2], post[i][3], foot_top }, { post[i][0], post[i][1], foot_top }
                }
            };
            // Legs stand in the seat's shadow: darker than the sides above them.
            auto leg_ref = light_ref( refs[i + 1] );
            leg_ref.shade = static_cast<uint8_t>( leg_ref.shade * 2 / 3 );
            sink.add( { .sprite = s, .corners = upper, .uvs = upper_uv, .ref = leg_ref } );
            const auto foot = corners_t{{ { post[i][0], post[i][1], foot_top }, { post[i][2], post[i][3], foot_top },
                    { post[i][2], post[i][3], z0 }, { post[i][0], post[i][1], z0 }
                }
            };
            leg_ref.shade = static_cast<uint8_t>( leg_ref.shade * foot_shade_factor );
            sink.add( { .sprite = s, .corners = foot, .uvs = foot_uv, .ref = leg_ref } );
        }
    }
}

// Foreground decals sit this far in front of the backdrop so the depth test lets them through.
constexpr auto layer_lift = static_cast<float>( 0.004f );

// Emits the backdrop (if the tile has one) and then the foreground slightly in front of it.
struct emit_top_layers_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    float z;
    const light_ref &ref;
    float front = -1.0f;
};

auto emit_top_layers( const emit_top_layers_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto z = options.z;
    const auto &ref = options.ref;
    auto front = options.front;

    if( t.backdrop.valid ) {
        emit_top( { .sink = sink, .s = t.backdrop, .x = x, .y = y, .z = z, .ref = ref, .front = front } );
    }
    emit_top( { .sink = sink, .s = t.sprite, .x = x, .y = y, .z = t.backdrop.valid ? z + layer_lift : z, .ref = ref, .front = front } );
}

// Height, in levels, of one repeat of a tileset sprite up a side: about one tile width at the
// usual storey height, so a brick or plank sprite keeps its scale instead of being stretched
// over the whole storey.
constexpr auto side_repeat = static_cast<float>( 0.6f );

// A side textured with the tile's own sprite, repeated up the face from its base (strips of
// side_repeat; the last one shows the bottom of the next repeat). Walls that have no entry in
// the wall-side atlas, or all of them when the option is off, take their look from the tileset
// this way.
struct emit_side_tiled_options {
    quad_sink &sink;
    const sprite_ref &s;
    int x;
    int y;
    face_bit face;
    float z_base;
    float z_top;
    const light_ref &ref;
    float shift_x;
    float shift_y;
};

auto emit_side_tiled( const emit_side_tiled_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x = options.x;
    auto y = options.y;
    auto face = options.face;
    auto z_base = options.z_base;
    auto z_top = options.z_top;
    const auto &ref = options.ref;
    auto shift_x = options.shift_x;
    auto shift_y = options.shift_y;

    for( auto z0 = static_cast<float>( z_base ); z0 < z_top - 0.001f; z0 += side_repeat ) {
        const auto z1 = static_cast<float>( std::min( z0 + side_repeat, z_top ) );
        const auto shown = static_cast<float>( ( z1 - z0 ) / side_repeat );
        emit_side( { .sink = sink, .s = s, .x = x, .y = y, .face = face, .z_base = z0, .z_top = z1, .ref = ref, .shift_x = shift_x, .shift_y = shift_y, .band = sprite_band{ .top = 1.0f - shown, .bottom = 1.0f } } );
    }
}

struct emit_side_layers_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    face_bit face;
    float z_base;
    float z_top;
    const light_ref &ref;
    const sprite_band &band = {};
};

auto emit_side_layers( const emit_side_layers_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto face = options.face;
    auto z_base = options.z_base;
    auto z_top = options.z_top;
    const auto &ref = options.ref;
    const auto &band = options.band;

    const auto whole = static_cast<bool>( band.top < 0.0f );
    if( t.backdrop.valid ) {
        if( whole ) {
            emit_side_tiled( { .sink = sink, .s = t.backdrop, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref, .shift_x = 0.0f, .shift_y = 0.0f } );
        } else {
            emit_side( { .sink = sink, .s = t.backdrop, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref, .shift_x = 0.0f, .shift_y = 0.0f, .band = band } );
        }
    }
    // Pushing the foreground outwards means shifting the whole face by its normal.
    auto dx = static_cast<float>( 0.0f );
    auto dy = static_cast<float>( 0.0f );
    if( t.backdrop.valid ) {
        dx = face == face_east ? layer_lift : face == face_west ? -layer_lift : 0.0f;
        dy = face == face_south ? layer_lift : face == face_north ? -layer_lift : 0.0f;
    }
    if( whole ) {
        emit_side_tiled( { .sink = sink, .s = t.sprite, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref, .shift_x = dx, .shift_y = dy } );
    } else {
        emit_side( { .sink = sink, .s = t.sprite, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref, .shift_x = dx, .shift_y = dy, .band = band } );
    }
}

// A side of a wall block: its own side texture when it has one, else the tileset sprite.
auto tinted( light_ref ref, uint32_t rgb ) -> light_ref
{
    ref.tint_r = static_cast<uint8_t>( ( rgb >> 16 ) & 255u );
    ref.tint_g = static_cast<uint8_t>( ( rgb >> 8 ) & 255u );
    ref.tint_b = static_cast<uint8_t>( rgb & 255u );
    return ref;
}

struct emit_wall_side_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    face_bit face;
    float z_base;
    float z_top;
    const light_ref &ref;
};

auto emit_wall_side( const emit_wall_side_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto face = options.face;
    auto z_base = options.z_base;
    auto z_top = options.z_top;
    const auto &ref = options.ref;

    if( t.side.valid ) {
        emit_side( { .sink = sink, .s = t.side, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = tinted( ref, t.side_tint ) } );
    } else if( t.elevation_split >= 0.0f ) {
        // Coving, paint and skirting board read from the edge-on sprite, once over the storey.
        emit_side( { .sink = sink, .s = t.elevation, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref, .shift_x = 0.0f, .shift_y = 0.0f,
                     .band = sprite_band{ .top = t.elevation_split, .bottom = 1.0f } } );
    } else {
        emit_side_layers( { .sink = sink, .t = t, .x = x, .y = y, .face = face, .z_base = z_base, .z_top = z_top, .ref = ref } );
    }
}

// A portion of a full-height wall face, retaining the original texture coordinates so
// sill, lintel and jambs line up with the neighbouring wall's paint and trim.
struct emit_wall_section_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    face_bit face;
    float bottom;
    float top;
    float left;
    float right;
    const light_ref &ref;
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 1.0f;
    float y1 = 1.0f;
};

auto emit_wall_section( const emit_wall_section_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto face = options.face;
    auto bottom = options.bottom;
    auto top = options.top;
    auto left = options.left;
    auto right = options.right;
    const auto &ref = options.ref;
    auto x0 = options.x0;
    auto y0 = options.y0;
    auto x1 = options.x1;
    auto y1 = options.y1;

    const auto &art = t.side.valid ? t.side :
                      t.elevation.valid ? t.elevation : t.main();
    const auto split = static_cast<float>( t.elevation.valid ? t.elevation_split : 0.0f );
    auto c = corners_t{};
    if( !side_corners( { .face = face, .x0 = float( x ), .y0 = float( y ), .z_base = bottom, .z_top = top, .c = c } ) ) {
        return;
    }
    for( auto axis = size_t( 0 ); axis < 2; ++axis ) {
        const auto across = static_cast<float>( c[1][axis] - c[0][axis] );
        c[1][axis] = c[0][axis] + across * right;
        c[0][axis] += across * left;
        c[2][axis] = c[3][axis] + across * right;
        c[3][axis] += across * left;
    }
    for( auto &corner : c ) {
        corner[0] = x + x0 + ( corner[0] - x ) * ( x1 - x0 );
        corner[1] = y + y0 + ( corner[1] - y ) * ( y1 - y0 );
    }
    const auto du = static_cast<float>( art.u1 - art.u0 );
    const auto dv = static_cast<float>( art.v1 - art.v0 );
    const auto u0 = static_cast<float>( art.u0 + du * left );
    const auto u1 = static_cast<float>( art.u0 + du * right );
    const auto v0 = static_cast<float>( art.v0 + dv * ( split + ( 1.0f - top ) * ( 1.0f - split ) ) );
    const auto v1 = static_cast<float>( art.v0 + dv * ( split + ( 1.0f - bottom ) *
                                        ( 1.0f - split ) ) );
    const auto uv = uvs_t{{ { u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 } }};
    sink.add( { .sprite = art, .corners = c, .uvs = uv, .ref = t.side.valid ? tinted( ref, t.side_tint ) : ref } );
}

struct emit_wall_cap_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    float z;
    float x0;
    float y0;
    float x1;
    float y1;
    const light_ref &ref;
};

auto emit_wall_cap( const emit_wall_cap_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto z = options.z;
    auto x0 = options.x0;
    auto y0 = options.y0;
    auto x1 = options.x1;
    auto y1 = options.y1;
    const auto &ref = options.ref;

    auto art = sprite_ref( t.cap.valid ? t.cap : t.elevation.valid ? t.elevation : t.main() );
    if( t.elevation.valid && !t.cap.valid ) {
        const auto dv = static_cast<float>( art.v1 - art.v0 );
        art.v0 += dv * ( t.elevation_split + ( 1.0f - t.elevation_split ) * 0.35f );
        art.v1 = art.v0 + dv * 0.1f;
    }
    const auto c = corners_t{{ { x + x0, y + y0, z }, { x + x1, y + y0, z },
            { x + x1, y + y1, z }, { x + x0, y + y1, z }
        }
    };
    const auto uv = uvs_t{{ { art.u0, art.v0 }, { art.u1, art.v0 },
            { art.u1, art.v1 }, { art.u0, art.v1 }
        }
    };
    sink.add( { .sprite = art, .corners = c, .uvs = uv, .ref = t.cap.valid ? tinted( ref, t.cap_tint ) : ref } );
}

struct wall_ref_options {
    int lx;
    int ly;
    int dx;
    int dy;
    int dz;
    uint8_t shade;
};

auto wall_ref( const wall_ref_options &options ) -> light_ref;

struct emit_thin_wall_options {
    quad_sink &sink;
    const tile_info &t;
    int x;
    int y;
    uint8_t connections;
    const std::array<const tile_info *, 4> &side_materials;
};

auto emit_thin_wall( const emit_thin_wall_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &t = options.t;
    auto x = options.x;
    auto y = options.y;
    auto connections = options.connections;
    const auto &side_materials = options.side_materials;

    const auto occupied = std::array<bool, 9>( wall_footprint( connections ) );
    const float edges[] = { 0.0f, 0.5f - wall_half_thickness,
                            0.5f + wall_half_thickness, 1.0f
                          };
    for( auto row = static_cast<int>( 0 ); row < 3; ++row ) {
        for( auto col = static_cast<int>( 0 ); col < 3; ++col ) {
            if( !occupied[row * 3 + col] ) {
                continue;
            }
            const auto x0 = static_cast<float>( edges[col] );
            const auto y0 = static_cast<float>( edges[row] );
            const auto x1 = static_cast<float>( edges[col + 1] );
            const auto y1 = static_cast<float>( edges[row + 1] );
            emit_wall_cap( { .sink = sink, .t = t, .x = x, .y = y, .z = t.sh.height, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1,
                             .ref = wall_ref( { .lx = x, .ly = y, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
            const auto faces = std::array<face_bit, 4> {{ face_north, face_south, face_west, face_east }};
            const int dx[] = { 0, 0, -1, 1 };
            const int dy[] = { -1, 1, 0, 0 };
            for( auto i = size_t( 0 ); i < faces.size(); ++i ) {
                const auto nx = static_cast<int>( col + dx[i] );
                const auto ny = static_cast<int>( row + dy[i] );
                const auto outside = static_cast<bool>( nx < 0 || nx > 2 || ny < 0 || ny > 2 );
                if( outside ? ( connections & faces[i] ) != 0 : occupied[ny * 3 + nx] ) {
                    continue;
                }
                emit_wall_section( { .sink = sink, .t = *side_materials[i], .x = x, .y = y, .face = faces[i], .bottom = 0.0f, .top = t.sh.height, .left = 0.0f, .right = 1.0f,
                                     .ref = wall_ref( { .lx = x, .ly = y, .dx = dx[i], .dy = dy[i], .dz = 0, .shade = face_shade[i + 1] } ),
                                     .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
            }
        }
    }
}

// How much of a 3/4-view furniture sprite is its front, for a box of the given height (in
// z-levels): taller furniture shows more front. Storeys are drawn about 1.8 tiles tall.
auto front_fraction( float height ) -> float
{
    // Tall storage (lockers, fridges, bookcases) is mostly front in the art: doors and shelves
    // under a thin lid.
    constexpr auto tall = static_cast<float>( 0.6f );
    if( height >= tall ) {
        return 0.78f;
    }
    const auto seen = static_cast<float>( height * 1.8f );
    return std::clamp( seen / ( seen + 1.0f ), 0.25f, 0.65f );
}

struct make_ref_options {
    int lx;
    int ly;
    int dx;
    int dy;
    int dz;
    uint8_t shade;
};

auto make_ref( const make_ref_options &options ) -> light_ref
{
    auto lx = options.lx;
    auto ly = options.ly;
    auto dx = options.dx;
    auto dy = options.dy;
    auto dz = options.dz;
    auto shade = options.shade;

    auto r = light_ref{};
    r.lx = static_cast<uint8_t>( lx );
    r.ly = static_cast<uint8_t>( ly );
    r.dx = static_cast<int8_t>( dx );
    r.dy = static_cast<int8_t>( dy );
    r.dz = static_cast<int8_t>( dz );
    r.shade = shade;
    return r;
}

auto wall_ref( const wall_ref_options &options ) -> light_ref
{
    auto lx = options.lx;
    auto ly = options.ly;
    auto dx = options.dx;
    auto dy = options.dy;
    auto dz = options.dz;
    auto shade = options.shade;

    auto r = light_ref( make_ref( { .lx = lx, .ly = ly, .dx = dx, .dy = dy, .dz = dz, .shade = shade } ) );
    r.wall = true;
    return r;
}

} // namespace

auto pack_light( uint8_t r, uint8_t g, uint8_t b ) -> uint32_t
{
    return static_cast<uint32_t>( r ) | ( static_cast<uint32_t>( g ) << 8 ) |
           ( static_cast<uint32_t>( b ) << 16 ) | ( 255u << 24 );
}

auto quad_sink::add( const add_options &options ) -> void
{
    const auto &sprite = options.sprite;
    const auto &corners = options.corners;
    const auto &uvs = options.uvs;
    const auto &ref = options.ref;

    add_colored( { .sprite = sprite, .corners = corners, .uvs = uvs, .ref = ref, .color = pack_light( ref.shade, ref.shade, ref.shade ) } );
}

auto quad_sink::add_colored( const add_colored_options &options ) -> void
{
    const auto &sprite = options.sprite;
    const auto &corners = options.corners;
    const auto &uvs = options.uvs;
    const auto &ref = options.ref;
    auto color = options.color;

    add_colored( { .sprite = sprite, .corners = corners, .uvs = uvs, .ref = ref, .colors = std::array<uint32_t, 4> {{ color, color, color, color }} } );
}

auto quad_sink::add_colored( const add_colored_options2 &options ) -> void
{
    const auto &sprite = options.sprite;
    const auto &corners = options.corners;
    const auto &uvs = options.uvs;
    const auto &ref = options.ref;
    const auto &colors = options.colors;

    if( !sprite.valid ) {
        return;
    }
    auto q = quad{};
    q.sheet = sprite.sheet;
    q.colors = colors;
    for( auto i = size_t( 0 ); i < 4; ++i ) {
        // Tile space (x east, y south, z up) is flipped into the right-handed world frame.
        q.v[i] = vertex{ .x = corners[i][0], .y = -corners[i][1], .z = corners[i][2], .u = uvs[i][0], .v = uvs[i][1] };
        q.r[i] = ref;
    }
    quads_.push_back( q );
}

auto quad_sink::finish() -> chunk_mesh
{
    std::stable_sort( quads_.begin(), quads_.end(), []( const quad & a, const quad & b ) -> bool {
        return a.sheet < b.sheet;
    } );
    auto mesh = chunk_mesh{};
    mesh.vertices.reserve( quads_.size() * 4 );
    mesh.light.reserve( quads_.size() * 4 );
    mesh.light_refs.reserve( quads_.size() * 4 );
    mesh.indices.reserve( quads_.size() * 6 );
    for( const quad &q : quads_ ) {
        if( mesh.batches.empty() || mesh.batches.back().sheet != q.sheet ) {
            auto b = mesh_batch{};
            b.sheet = q.sheet;
            b.first_index = static_cast<uint32_t>( mesh.indices.size() );
            mesh.batches.push_back( b );
        }
        const auto base = static_cast<uint32_t>( mesh.vertices.size() );
        for( auto i = size_t( 0 ); i < 4; ++i ) {
            mesh.vertices.push_back( q.v[i] );
            mesh.light_refs.push_back( q.r[i] );
            mesh.light.push_back( q.colors[i] );
        }
        for( uint32_t idx : {
                 0u, 1u, 2u, 0u, 2u, 3u
             } ) {
            mesh.indices.push_back( base + idx );
        }
        mesh.batches.back().index_count += 6;
    }
    return mesh;
}

auto tile_info_cache::set_thin_walls( bool on ) -> void
{
    if( on != thin_walls_ ) {
        thin_walls_ = on;
        clear();
    }
}

auto tile_info_cache::set_plaster_atlas( bool on ) -> void
{
    if( on != plaster_atlas_ ) {
        plaster_atlas_ = on;
        clear();
    }
}

auto tile_info_cache::set_wall_sides( std::shared_ptr<const wall_sides> sides ) -> void
{
    wall_sides_ = std::move( sides );
    clear();
}

auto tile_info_cache::clear() -> void
{
    terrain_.clear();
    furniture_.clear();
    named_.clear();
    ground_ = {};
    ground_loaded_ = false;
}

namespace
{
// One cache entry per id and orientation; subtile is -1..7 and rota -1..3.
auto orientation_key( int id, int subtile, int rota ) -> int
{ return id * 128 + ( subtile + 1 ) * 8 + ( rota + 1 ); }
} // namespace

auto tile_info_cache::terrain( const terrain_options &options ) -> const tile_info &
{
    auto id = options.id;
    auto &tiles = options.tiles;
    auto subtile = options.subtile;
    auto rota = options.rota;

    const auto key = static_cast<int>( orientation_key( id.to_i(), subtile, rota ) );
    auto it = terrain_.find( key );
    if( it != terrain_.end() ) {
        return it->second;
    }
    auto info = tile_info{};
    const auto &t = id.obj();
    auto traits = tile_traits( traits_of( t ) );
    if( !traits.window ) {
        const auto id_str = std::string( t.id.str() );
        const auto looks_like = std::string( t.looks_like );
        const auto has_name_match = [&id_str, &looks_like]( const std::string &sub ) -> bool {
            return id_str.find( sub ) != std::string::npos || looks_like.find( sub ) != std::string::npos;
        };
        const auto is_excluded = has_name_match( "manhole" ) || has_name_match( "hatch" ) ||
                                 has_name_match( "grate" ) || has_name_match( "ramp" );
        if( !is_excluded && ( traits.door || has_name_match( "door" ) || has_name_match( "gate" ) ) ) {
            traits.door = true;
            traits.open_door = traits.flat || traits.move_cost > 0;
        }
    } else {
        traits.door = false;
        traits.open_door = false;
    }
    info.sh = classify_terrain( traits );
    info.tree = traits.tree;
    info.window = traits.window;
    info.door = traits.door;
    info.open_door = traits.open_door;
    if( traits.open_door && !t.close.is_empty() && t.close.is_valid() ) {
        info.closed_door_id = t.close.id();
    } else {
        info.closed_door_id = t_null;
    }
    info.thin_wall = traits.wall && thin_walls_;
    info.wall_connection = traits.wall || traits.window || traits.door;
    info.see_through = traits.transparent;
    if( info.sh.kind != shape_kind::empty ) {
        resolve_layers( { .tiles = tiles, .id = t.id.str(), .category = C_TERRAIN, .out = info, .subtile = subtile, .rota = rota } );
    }
    if( info.sh.kind == shape_kind::block && wall_sides_ ) {
        // The id and what it looks like, nearest first, pick the side texture.
        auto chain = std::vector<std::string> { t.id.str() };
        auto like = std::string( t.looks_like );
        while( !like.empty() && chain.size() < 8 ) {
            chain.push_back( like );
            const auto next = ter_str_id( like );
            like = next.is_valid() ? next.obj().looks_like : std::string();
        }
        const auto name = std::string( match_wall_texture( wall_sides_->rules, chain ) );
        info.brick_wall = name == "brick";
        const auto found = std::find( wall_sides_->names.begin(), wall_sides_->names.end(), name );
        // A wall of a patterned material (brick, wood, logs, stone, metal...) keeps the
        // tileset's own look: its sprite is repeated up the sides and tops the wall. Painted
        // and plain walls, whose sprites only give a colour, take the atlas texture instead.
        const auto patterned_material = static_cast<bool>( !name.empty() && name != "plaster" &&
                                        info.sprite.valid );
        // A painted or plain wall's sprite is drawn as a front elevation: a band of framing at
        // the top, then coving, the paint and a skirting board. Read that way it is the side,
        // and the framing band tops the wall; the atlas plaster is the alternative.
        // Of the wall's multitile pieces, the east-west edge shows it edge-on: a cavity (the
        // top rows), a pale coving line, the paint and a skirting board. That piece is the
        // elevation of every side, whatever piece the tile itself is drawn with from above.
        constexpr auto edge_subtile = static_cast<int>( 2 ); // "edge" in cata_tiles' multitile_keys
        constexpr auto edge_east_west = static_cast<int>( 0 );
        constexpr auto cavity_rows = static_cast<float>( 0.125f );
        if( !patterned_material && !plaster_atlas_ && info.sprite.valid ) {
            info.elevation = info.sprite;
            if( const auto edge = std::optional<texture_draw_data>( tiles.get_sprite_draw_data( { .id = t.id.str(), .category = C_TERRAIN, .pos = tripoint_bub_ms( 0, 0, 0 ), .subtile = edge_subtile, .rotation = edge_east_west } ) ) ) {
                info.elevation = to_sprite_ref( *edge );
            }
            const auto &art = info.elevation;
            info.elevation_split = art.opaque_y0 + ( art.opaque_y1 - art.opaque_y0 ) * cavity_rows;
            // Debug aid: CATA_R3D_DEBUG_WALLS=1 logs the sprite geometry of each painted wall.
            static const auto debug_walls = static_cast<bool>( std::getenv( "CATA_R3D_DEBUG_WALLS" ) !=
                                            nullptr );
            if( debug_walls ) {
                DebugLog( DL::Info, DC::Main ) << "r3d wall " << t.id.str() << " sub=" << subtile << " rot=" << rota
                                               << " uv=" << art.u0 << "," << art.v0 << "-" << art.u1 << "," << art.v1
                                               << " size=" << art.size_x << "x" << art.size_y
                                               << " opaque=" << art.opaque_x0 << "," << art.opaque_y0 << "-"
                                               << art.opaque_x1 << "," << art.opaque_y1
                                               << " backdrop=" << info.backdrop.valid << " side=" << info.side.valid;
            }
        } else if( !name.empty() && found != wall_sides_->names.end() && !patterned_material ) {
            const auto uv = uv_rect( atlas_cell( { .index = static_cast<int>( found - wall_sides_->names.begin() ),
                                                   .cell = wall_sides_->cell, .atlas_w = wall_sides_->width, .atlas_h = wall_sides_->height } ) );
            if( uv.valid ) {
                info.side.sheet = wall_sides_->atlas.get();
                info.side.owner = wall_sides_->atlas;
                info.side.u0 = uv.u0;
                info.side.v0 = uv.v0;
                info.side.u1 = uv.u1;
                info.side.v1 = uv.v1;
                info.side.valid = true;
                // The cap is the middle of the texture, scaled up: an even surface in the wall's
                // colour, as isometric tilesets draw the top of a wall.
                info.cap = info.side;
                const auto du = static_cast<float>( ( uv.u1 - uv.u0 ) * 0.4f );
                const auto dv = static_cast<float>( ( uv.v1 - uv.v0 ) * 0.4f );
                info.cap.u0 += du;
                info.cap.u1 -= du;
                info.cap.v0 += dv;
                info.cap.v1 -= dv;
                // Colours: the tileset's own wall sprite (its front for the sides, its top for
                // the cap), else the texture's default.
                const auto &art = info.main();
                const auto fallback = wall_sides_->colors.find( name );
                const auto preset = static_cast<uint32_t>( fallback != wall_sides_->colors.end() ?
                                    fallback->second :
                                    0xb4b4b4u );
                info.side_tint = wall_tint( art.body_color, preset );
                // The cap a shade lighter than the sides, so the top of a wall reads as a top
                // even where the paint is one colour.
                info.cap_tint = lighten( wall_tint( art.top_color, preset ), 1.18f );
            }
        }
    }
    return terrain_.emplace( key, info ).first->second;
}

auto tile_info_cache::named( const named_options &options ) -> const tile_info &
{
    auto category = options.category;
    const auto &id = options.id;
    auto &tiles = options.tiles;
    auto subtile = options.subtile;
    auto rota = options.rota;
    const auto &variant = options.variant;

    const auto key = std::string( std::to_string( static_cast<int>( category ) ) + ":" +
                                  std::to_string( subtile ) + ":" + std::to_string( rota ) + ":" +
                                  ( options.allow_unknown ? "1:" : "0:" ) +
                                  std::to_string( id.size() ) + ":" + id + variant );
    auto it = named_.find( key );
    if( it != named_.end() ) {
        return it->second;
    }
    auto info = tile_info{};
    resolve_layers( { .tiles = tiles, .id = id, .category = category, .out = info, .subtile = subtile, .rota = rota, .variant = variant, .allow_unknown = options.allow_unknown } );
    return named_.emplace( key, info ).first->second;
}

auto tile_info_cache::furniture( const furniture_options &options ) -> const tile_info &
{
    auto id = options.id;
    auto &tiles = options.tiles;
    auto subtile = options.subtile;
    auto rota = options.rota;

    const auto key = static_cast<int>( orientation_key( id.to_i(), subtile, rota ) );
    auto it = furniture_.find( key );
    if( it != furniture_.end() ) {
        return it->second;
    }
    auto info = tile_info{};
    const auto &f = id.obj();
    if( f.id != furn_str_id::NULL_ID() ) {
        auto traits = tile_traits( traits_of( f ) );
        // Furniture has a "move cost mod": negative means impassable, anything else walkable.
        traits.move_cost = f.movecost < 0 ? 0 : 100;
        // Notice boards keep their complete front elevation; lockers have a shallow
        // footprint. Follow artwork inheritance so variants keep the same geometry.
        auto art_id = std::string( f.id.str() );
        for( auto depth = static_cast<int>( 0 ); depth < 8 && !art_id.empty(); ++depth ) {
            if( art_id.compare( 0, 10, "f_boulder_" ) == 0 ) {
                traits.boulder = true;
            }
            if( art_id == "f_locker" || art_id == "f_vitrified_locker" ) {
                info.depth_scale = 0.5f;
            }
            if( art_id == "f_bulletin" ) {
                traits.panel = true;
                break;
            }
            const auto candidate = furn_str_id( art_id );
            art_id = candidate.is_valid() ? candidate.obj().looks_like : std::string();
        }
        info.sh = classify_furniture( traits );
        resolve_layers( { .tiles = tiles, .id = f.id.str(), .category = C_FURNITURE, .out = info, .subtile = subtile, .rota = rota } );
        if( info.sh.kind == shape_kind::block ) {
            // A supplied backdrop may itself have transparent holes. Keep the object's
            // artwork as detail over a solid material, including background-only tiles.
            // Use the centre tile as a continuous rock material.
            const auto detail = sprite_ref( info.main() );
            info.backdrop = traits.boulder ?
                            terrain( { .id = ter_str_id( "t_rock" ).id(), .tiles = tiles, .subtile = 0 } ).main() : ground(
                                tiles ).main();
            info.sprite = detail;
        }
    }
    return furniture_.emplace( key, info ).first->second;
}

auto tile_info_cache::ground( cata_tiles &tiles ) -> const tile_info &
{
    if( !ground_loaded_ ) {
        resolve_layers( { .tiles = tiles, .id = "t_grass", .category = C_TERRAIN, .out = ground_ } );
        ground_loaded_ = true;
    }
    return ground_;
}

auto chunk_stamp( const tripoint_abs_sm &pos, const submap_lookup &lookup ) -> uint64_t
{
    const submap *self = lookup( pos );
    if( !self ) {
        return 0;
    }
    auto stamp = static_cast<uint64_t>( self->render_stamp() );
    const tripoint_rel_sm neighbours[] = { { 0, -1, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    for( const tripoint_rel_sm &d : neighbours ) {
        const submap *n = lookup( pos + d );
        stamp = stamp * 1099511628211ull ^ ( n ? n->render_stamp() : 0ull );
    }
    return stamp;
}

namespace
{

// Queries and geometry shared by the chunk's terrain and furniture passes.
struct chunk_tile_lookup {
    const submap *self;
    const submap *north;
    const submap *south;
    const submap *west;
    const submap *east;
    tile_info_cache &cache;
    cata_tiles &tiles;
    const map &here;

    static constexpr auto wall_dirs = std::array<point, 4> {
        {
            point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 )
        }
    };
    static constexpr auto wall_bits = std::array<uint8_t, 4> {{ face_north, face_south, face_west, face_east }};

    auto terrain_at( int lx, int ly ) const -> const tile_info* {
        const submap *sm = self;
        if( lx < 0 ) {
            sm = west;
            lx = side - 1;
        } else if( lx >= side ) {
            sm = east;
            lx = 0;
        } else if( ly < 0 ) {
            sm = north;
            ly = side - 1;
        } else if( ly >= side ) {
            sm = south;
            ly = 0;
        }
        return sm ? &cache.terrain( { .id = sm->get_ter( point_sm_ms( lx, ly ) ), .tiles = tiles } ) :
               nullptr;
    }

    auto furniture_at( int lx, int ly ) const -> furn_id {
        const submap *sm = self;
        if( lx < 0 ) {
            sm = west;
            lx = side - 1;
        } else if( lx >= side ) {
            sm = east;
            lx = 0;
        } else if( ly < 0 ) {
            sm = north;
            ly = side - 1;
        } else if( ly >= side ) {
            sm = south;
            ly = 0;
        }
        return sm ? sm->get_furn( point_sm_ms( lx, ly ) ) : furn_str_id::NULL_ID().id();
    }

    auto occupancy_at( const submap *sm, int sx, int sy ) const -> shape {
        if( !sm ) {
            return {};
        }
        const auto p = point_sm_ms( sx, sy );
        const auto t = shape( cache.terrain( { .id = sm->get_ter( p ), .tiles = tiles } ).sh );
        const auto f = shape( cache.furniture( { .id = sm->get_furn( p ), .tiles = tiles } ).sh );
        return combine( t, f );
    }

    struct emit_floor_under_options {
        quad_sink &sink;
        int lx;
        int ly;
        bool wall_along_y;
    };

    auto emit_floor_under( const emit_floor_under_options &options ) const -> void {
        auto &sink = options.sink;
        auto lx = options.lx;
        auto ly = options.ly;
        auto wall_along_y = options.wall_along_y;

        const tile_info *under = nullptr;
        const auto order = std::array<point, 4>( wall_along_y ?
                           std::array<point, 4> { point( -1, 0 ), point( 1, 0 ), point( 0, -1 ), point( 0, 1 ) } :
                           std::array<point, 4> { point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 ) } );
        for( const point &d : order ) {
            const tile_info *n = terrain_at( lx + d.x, ly + d.y );
            if( n && n->sh.kind == shape_kind::floor && !is_tall( n->sprite ) ) {
                under = n;
                break;
            }
        }
        emit_top_layers( { .sink = sink, .t = under ? *under : cache.ground( tiles ), .x = lx, .y = ly, .z = floor_height,
                           .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
    }

    auto wall_at( const tripoint_bub_ms &p ) const -> const tile_info* {
        return here.inbounds( p ) ? &cache.terrain( { .id = here.ter( p ), .tiles = tiles } ) : nullptr;
    }

    auto connections_at( const tripoint_bub_ms &p ) const -> uint8_t {
        auto mask = uint8_t( 0 );
        for( auto i = size_t( 0 ); i < wall_dirs.size(); ++i ) {
            const auto &d = wall_dirs[i];
            const tile_info *t = wall_at( p + tripoint_rel_ms( d.x, d.y, 0 ) );
            if( t && t->wall_connection ) {
                mask |= wall_bits[i];
            }
        }
        return mask;
    }

    auto trim_corner_at( const tripoint_bub_ms &p ) const -> bool {
        const tile_info *self_wall = wall_at( p );
        if( !self_wall || !self_wall->thin_wall ) {
            return false;
        }
        const auto connections = uint8_t( connections_at( p ) );
        if( connections == 0 || ( connections & ( connections - 1 ) ) != 0 ) {
            return false;
        }
        auto indoor = uint8_t( 0 );
        auto neighbour = uint8_t( 0 );
        for( auto i = size_t( 0 ); i < wall_dirs.size(); ++i ) {
            const auto &d = wall_dirs[i];
            const auto next = tripoint_bub_ms( p + tripoint_rel_ms( d.x, d.y, 0 ) );
            const tile_info *t = wall_at( next );
            if( t && t->sh.kind == shape_kind::floor &&
                here.has_flag( "INDOORS", next ) ) {
                indoor |= wall_bits[i];
            }
            if( ( connections & wall_bits[i] ) != 0 ) {
                neighbour = connections_at( next );
            }
        }
        return trim_wall_spur( connections, indoor, neighbour );
    }

    auto indoor_floor( const tripoint_bub_ms &p ) const -> bool {
        return here.inbounds( p ) &&
               here.has_flag( "INDOORS", p ) &&
               cache.terrain( { .id = here.ter( p ), .tiles = tiles } ).sh.kind == shape_kind::floor;
    }

    struct face_material_options {
        const tripoint_bub_ms &bub;
        const tile_info &wall;
        int dx;
        int dy;
    };

    auto face_material( const face_material_options &options ) const -> const tile_info& {
        const auto &bub = options.bub;
        const auto &wall = options.wall;
        auto dx = options.dx;
        auto dy = options.dy;

        if( !wall.brick_wall ) {
            return wall;
        }
        const auto adjacent = tripoint_bub_ms( bub + tripoint_rel_ms( dx, dy, 0 ) );

        auto inside = static_cast<bool>( indoor_floor( adjacent ) );
        // At an L or T junction the immediate tile can itself be wall. The
        // exposed inner corner then faces the floor diagonally beyond that wall.
        if( !inside && here.inbounds( adjacent ) &&
            cache.terrain( { .id = here.ter( adjacent ), .tiles = tiles } ).sh.kind == shape_kind::block ) {
            inside = indoor_floor( adjacent + tripoint_rel_ms( dy, dx, 0 ) ) ||
                     indoor_floor( adjacent - tripoint_rel_ms( dy, dx, 0 ) );
        }
        if( inside ) {
            return cache.terrain( { .id = ter_str_id( "t_wall" ).id(), .tiles = tiles, .subtile = 2, .rota = 0 } );
        }
        return wall;
    }
};

} // namespace

auto build_chunk_mesh( const build_chunk_mesh_options &options ) -> chunk_mesh
{
    const auto &pos = options.pos;
    const auto &lookup = options.lookup;
    auto &cache = options.cache;
    auto &tiles = options.tiles;

    const submap *self = lookup( pos );
    if( !self ) {
        return {};
    }
    const submap *north = lookup( pos + tripoint_rel_sm( 0, -1, 0 ) );
    const submap *south = lookup( pos + tripoint_rel_sm( 0, 1, 0 ) );
    const submap *west = lookup( pos + tripoint_rel_sm( -1, 0, 0 ) );
    const submap *east = lookup( pos + tripoint_rel_sm( 1, 0, 0 ) );
    const submap *above = lookup( pos + tripoint_rel_sm( 0, 0, 1 ) );
    const submap *below = lookup( pos + tripoint_rel_sm( 0, 0, -1 ) );
    const auto chunk_tiles = chunk_tile_lookup{ .self = self, .north = north, .south = south, .west = west, .east = east, .cache = cache, .tiles = tiles, .here = get_map() };

    // Terrain of a tile of this level, which may lie in a horizontal neighbour (one tile out).


    // Furniture of a tile of this level, which may lie in a horizontal neighbour (one tile out).




    // Occupancy of this level including a one tile border, and of the level above.
    std::array<shape, border *border> level;
    std::array<shape, side *side> up;
    for( auto ly = static_cast<int>( -1 ); ly <= side; ++ly ) {
        for( auto lx = static_cast<int>( -1 ); lx <= side; ++lx ) {
            const submap *sm = self;
            auto sx = static_cast<int>( lx );
            auto sy = static_cast<int>( ly );
            const auto out_x = static_cast<bool>( lx < 0 || lx >= side );
            const auto out_y = static_cast<bool>( ly < 0 || ly >= side );
            auto sh = shape{};
            if( out_x && out_y ) {
                sh = {}; // corners never matter: faces only look at the four neighbours
            } else {
                if( lx < 0 ) {
                    sm = west;
                    sx = side - 1;
                } else if( lx >= side ) {
                    sm = east;
                    sx = 0;
                }
                if( ly < 0 ) {
                    sm = north;
                    sy = side - 1;
                } else if( ly >= side ) {
                    sm = south;
                    sy = 0;
                }
                sh = chunk_tiles.occupancy_at( sm, sx, sy );
            }
            level[static_cast<size_t>( ( ly + 1 ) * border + ( lx + 1 ) )] = sh;
        }
    }
    for( auto ly = static_cast<int>( 0 ); ly < side; ++ly ) {
        for( auto lx = static_cast<int>( 0 ); lx < side; ++lx ) {
            up[static_cast<size_t>( ly * side + lx )] = chunk_tiles.occupancy_at( above, lx, ly );
        }
    }
    auto level_at = [ &level ]( int lx, int ly ) -> const shape & {
        return level[static_cast<size_t>( ( ly + 1 ) * border + ( lx + 1 ) )];
    };

    // Chunks are only built for submaps in the reality bubble, so the map knows every tile here.
    const auto chunk_origin = tripoint_abs_ms( project_to<coords::ms>( pos ) );
    // Some mapgen corners include a one-tile inward buttress. A thin wall needs
    // just the connected outside corner, rather than a spur jutting into the room.





    auto sink = quad_sink{};
    auto mesh_uprights = std::vector<upright_sprite> {};
    // Floor for a tile that stands in a wall line (pane, see-through block): a neighbour's floor
    // on an open side (inside or out), else the ground.


    for( auto ly = static_cast<int>( 0 ); ly < side; ++ly ) {
        for( auto lx = static_cast<int>( 0 ); lx < side; ++lx ) {
            const auto p = point_sm_ms( lx, ly );
            // Connected sprites (walls, benches, counters) as the flat view orients them.
            const auto bub = tripoint_bub_ms( abs_to_bub( chunk_origin + tripoint_rel_ms( lx, ly, 0 ) ) );
            auto ter_subtile = static_cast<int>( -1 );
            auto ter_rota = static_cast<int>( 0 );
            auto furn_subtile = static_cast<int>( -1 );
            auto furn_rota = static_cast<int>( 0 );
            tiles.get_orientation( { .pos = bub, .furniture = false, .subtile = ter_subtile, .rotation = ter_rota } );
            if( self->get_furn( p ) != furn_str_id::NULL_ID() ) {
                tiles.get_orientation( { .pos = bub, .furniture = true, .subtile = furn_subtile, .rotation = furn_rota } );
            }
            const tile_info &terrain = cache.terrain( { .id = self->get_ter( p ), .tiles = tiles, .subtile = ter_subtile, .rota = ter_rota } );
            const tile_info &furn = cache.furniture( { .id = self->get_furn( p ), .tiles = tiles, .subtile = furn_subtile, .rota = furn_rota } );
            auto nb = neighbourhood{};
            nb.up = up[static_cast<size_t>( ly * side + lx )];
            nb.north = level_at( lx, ly - 1 );
            nb.south = level_at( lx, ly + 1 );
            nb.west = level_at( lx - 1, ly );
            nb.east = level_at( lx + 1, ly );

            // The gameplay outside cache includes a one-tile sheltered border. Read the
            // actual floor's INDOORS flag instead, so the outside of an exterior wall
            // remains brick even beside a sheltered pavement or doorway.

            const auto &ts = terrain.sh;
            // A treetop is drawn by the tree's own sprite on the level below.
            if( ts.kind == shape_kind::floor && below &&
                cache.terrain( { .id = below->get_ter( p ), .tiles = tiles } ).tree ) {
                continue;
            }
            // Every side is built, even one a neighbour hides: the cutaway around the player
            // removes whole tiles, and the neighbour's side must then be there to show.
            constexpr auto all_sides = uint8_t( face_north | face_south | face_west | face_east );
            const auto faces = uint8_t( visible_faces( ts, nb ) |
                                        ( ts.kind == shape_kind::block ? all_sides : 0 ) );
            switch( ts.kind ) {
                case shape_kind::floor:
                    if( ( faces & face_top ) != 0 ) {
                        auto floor_ref = light_ref( make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) );
                        floor_ref.floor = true;
                        // Tall sprites (long grass) are often transparent around what stands on
                        // the tile, so the ground goes underneath unless they bring a backdrop.
                        if( is_tall( terrain.sprite ) && !terrain.backdrop.valid ) {
                            emit_top_layers( { .sink = sink, .t = cache.ground( tiles ), .x = lx, .y = ly, .z = ts.height, .ref = floor_ref } );
                            emit_top( { .sink = sink, .s = terrain.sprite, .x = lx, .y = ly, .z = ts.height + layer_lift, .ref = floor_ref } );
                        } else {
                            emit_top_layers( { .sink = sink, .t = terrain, .x = lx, .y = ly, .z = ts.height, .ref = floor_ref } );
                        }
                    }
                    break;
                case shape_kind::block:
                    if( terrain.thin_wall ) {
                        if( chunk_tiles.trim_corner_at( bub ) ) {
                            chunk_tiles.emit_floor_under( { .sink = sink, .lx = lx, .ly = ly, .wall_along_y = false } );
                            break;
                        }
                        const auto dirs = std::array<point, 4> {{
                                point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 )
                            }
                        };
                        const auto joins = std::array<face_bit, 4> {{
                                face_north, face_south, face_west, face_east
                            }
                        };
                        auto connections = uint8_t( 0 );
                        for( auto i = size_t( 0 ); i < dirs.size(); ++i ) {
                            const tile_info *n = chunk_tiles.terrain_at( lx + dirs[i].x, ly + dirs[i].y );
                            if( n && n->wall_connection &&
                                !chunk_tiles.trim_corner_at( bub + tripoint_rel_ms( dirs[i].x, dirs[i].y, 0 ) ) ) {
                                connections |= joins[i];
                            }
                        }
                        chunk_tiles.emit_floor_under( { .sink = sink, .lx = lx, .ly = ly, .wall_along_y = ( connections & ( face_north | face_south ) ) != 0 } );
                        const std::array<const tile_info *, 4> materials = {{
                                &chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 0, .dy = -1 } ), &chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 0, .dy = 1 } ),
                                &chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = -1, .dy = 0 } ), &chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 1, .dy = 0 } )
                            }
                        };
                        emit_thin_wall( { .sink = sink, .t = terrain, .x = lx, .y = ly, .connections = connections, .side_materials = materials } );
                        break;
                    }
                    // Every wall stands on a floor: a see-through block (fence, bars, a sparse
                    // wall sprite) would otherwise show the level below through its holes, and
                    // the cutaway around the player would show a void where a wall was.
                    chunk_tiles.emit_floor_under( { .sink = sink, .lx = lx, .ly = ly, .wall_along_y = nb.north.kind == shape_kind::block ||
                                                    nb.south.kind == shape_kind::block } );
                    if( ( faces & face_top ) != 0 ) {
                        if( terrain.cap.valid ) {
                            emit_top( { .sink = sink, .s = terrain.cap, .x = lx, .y = ly, .z = ts.height,
                                        .ref = tinted( wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ), terrain.cap_tint ) } );
                        } else if( terrain.elevation_split >= 0.0f ) {
                            // A plain cap in the paint: a patch from the middle of the
                            // elevation's painted rows, scaled over the top.
                            auto patch = sprite_ref( terrain.elevation );
                            const auto dv = static_cast<float>( patch.v1 - patch.v0 );
                            const auto du = static_cast<float>( patch.u1 - patch.u0 );
                            const auto paint0 = static_cast<float>( terrain.elevation_split );
                            patch.v0 = terrain.elevation.v0 + dv * ( paint0 + ( 1.0f - paint0 ) * 0.35f );
                            patch.v1 = terrain.elevation.v0 + dv * ( paint0 + ( 1.0f - paint0 ) * 0.6f );
                            patch.u0 += du * 0.2f;
                            patch.u1 -= du * 0.2f;
                            patch.size_y = patch.size_x;
                            patch.quarter_turns = 0;
                            patch.flip = false;
                            emit_top( { .sink = sink, .s = patch, .x = lx, .y = ly, .z = ts.height,
                                        .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                        } else {
                            emit_top_layers( { .sink = sink, .t = terrain, .x = lx, .y = ly, .z = ts.height,
                                               .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                        }
                    }
                    if( ( faces & face_north ) != 0 ) {
                        emit_wall_side( { .sink = sink, .t = chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 0, .dy = -1 } ), .x = lx, .y = ly, .face = face_north, .z_base = 0.0f,
                                          .z_top = ts.height,
                                          .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = -1, .dz = 0, .shade = face_shade[1] } ) } );
                    }
                    if( ( faces & face_south ) != 0 ) {
                        emit_wall_side( { .sink = sink, .t = chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 0, .dy = 1 } ), .x = lx, .y = ly, .face = face_south, .z_base = 0.0f,
                                          .z_top = ts.height,
                                          .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 1, .dz = 0, .shade = face_shade[2] } ) } );
                    }
                    if( ( faces & face_west ) != 0 ) {
                        emit_wall_side( { .sink = sink, .t = chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = -1, .dy = 0 } ), .x = lx, .y = ly, .face = face_west, .z_base = 0.0f,
                                          .z_top = ts.height,
                                          .ref = wall_ref( { .lx = lx, .ly = ly, .dx = -1, .dy = 0, .dz = 0, .shade = face_shade[3] } ) } );
                    }
                    if( ( faces & face_east ) != 0 ) {
                        emit_wall_side( { .sink = sink, .t = chunk_tiles.face_material( { .bub = bub, .wall = terrain, .dx = 1, .dy = 0 } ), .x = lx, .y = ly, .face = face_east, .z_base = 0.0f,
                                          .z_top = ts.height,
                                          .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 1, .dy = 0, .dz = 0, .shade = face_shade[4] } ) } );
                    }
                    break;
                case shape_kind::upright:
                    // Trees and shrubs: ground here, the sprite itself is drawn per frame.
                    emit_top_layers( { .sink = sink, .t = cache.ground( tiles ), .x = lx, .y = ly, .z = floor_height,
                                       .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                    if( ts.ground_backdrop && terrain.backdrop.valid ) {
                        emit_top( { .sink = sink, .s = terrain.backdrop, .x = lx, .y = ly, .z = floor_height + layer_lift,
                                    .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                    }
                    mesh_uprights.push_back( { .lx = static_cast<uint8_t>( lx ), .ly = static_cast<uint8_t>( ly ),
                                               .z_base = floor_height, .info = &terrain } );
                    break;
                case shape_kind::pane: {
                    // The frame stands along the wall line over the floor of its open sides.
                    const auto wall_along_y = static_cast<bool>( nb.north.kind == shape_kind::block ||
                                              nb.south.kind == shape_kind::block );
                    chunk_tiles.emit_floor_under( { .sink = sink, .lx = lx, .ly = ly, .wall_along_y = wall_along_y } );
                    const auto *surround = static_cast<const tile_info *>( nullptr );
                    if( terrain.window || terrain.door ) {
                        const auto neighbours = std::array<point, 4> {{
                                point( -1, 0 ), point( 1, 0 ), point( 0, -1 ), point( 0, 1 )
                            }
                        };
                        for( const auto &d : neighbours ) {
                            const auto *candidate = chunk_tiles.terrain_at( lx + d.x, ly + d.y );
                            if( candidate && candidate->sh.kind == shape_kind::block ) {
                                surround = candidate;
                                break;
                            }
                        }
                        if( !surround ) {
                            surround = &cache.terrain( { .id = ter_str_id( "t_wall" ).id(), .tiles = tiles, .subtile = 2, .rota = 0 } );
                        }
                    }
                    if( terrain.window ) {
                        // Carry the neighbouring wall across the sill and lintel. The glass
                        // occupies an opening rather than replacing an entire storey.
                        const auto half_width = static_cast<float>( cache.wall_half_width() );
                        constexpr auto sill = static_cast<float>( 0.35f );
                        constexpr auto lintel = static_cast<float>( 0.9f );
                        constexpr auto recess_shade = 178;
                        const auto x0 = static_cast<float>( wall_along_y ? 0.5f - half_width : 0.0f );
                        const auto x1 = static_cast<float>( wall_along_y ? 0.5f + half_width : 1.0f );
                        const auto y0 = static_cast<float>( wall_along_y ? 0.0f : 0.5f - half_width );
                        const auto y1 = static_cast<float>( wall_along_y ? 1.0f : 0.5f + half_width );
                        for( const face_bit face : {
                                 face_north, face_south, face_west, face_east
                             } ) {
                            const auto broad = static_cast<bool>( wall_along_y ? ( face == face_west || face == face_east ) :
                                                                  ( face == face_north || face == face_south ) );
                            if( broad ) {
                                const auto dx = static_cast<int>( face == face_west ? -1 : face == face_east ? 1 : 0 );
                                const auto dy = static_cast<int>( face == face_north ? -1 : face == face_south ? 1 : 0 );
                                const tile_info &material = chunk_tiles.face_material( { .bub = bub, .wall = *surround, .dx = dx, .dy = dy } );
                                const auto ref = light_ref( wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = 0.0f, .top = sill, .left = 0.0f, .right = 1.0f, .ref = ref, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = lintel, .top = ts.height, .left = 0.0f, .right = 1.0f, .ref = ref, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = sill, .top = lintel, .left = 0.0f, .right = 0.1f,
                                                     .ref = light_ref( wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = recess_shade } ) ),
                                                     .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = sill, .top = lintel, .left = 0.9f, .right = 1.0f,
                                                     .ref = light_ref( wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = recess_shade } ) ),
                                                     .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                            }
                        }
                        emit_wall_cap( { .sink = sink, .t = *surround, .x = lx, .y = ly, .z = sill, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1,
                                         .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = recess_shade } ) } );
                        emit_wall_cap( { .sink = sink, .t = *surround, .x = lx, .y = ly, .z = ts.height, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1,
                                         .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) } );
                        if( terrain.backdrop.valid ) {
                            emit_pane( { .sink = sink, .s = terrain.backdrop, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = sill,
                                         .height = lintel - sill, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ), .half_thickness = half_width + 0.001f, .half_width = 0.4f } );
                        }
                        emit_pane( { .sink = sink, .s = terrain.sprite, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = sill,
                                     .height = lintel - sill, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ), .half_thickness = half_width + 0.002f, .half_width = 0.4f } );
                        break;
                    }
                    if( terrain.door ) {
                        constexpr auto lintel = static_cast<float>( 0.86f );
                        constexpr auto jamb = door_jamb_width;
                        const auto half_width = static_cast<float>( cache.wall_half_width() );
                        const auto x0 = static_cast<float>( wall_along_y ? 0.5f - half_width : 0.0f );
                        const auto x1 = static_cast<float>( wall_along_y ? 0.5f + half_width : 1.0f );
                        const auto y0 = static_cast<float>( wall_along_y ? 0.0f : 0.5f - half_width );
                        const auto y1 = static_cast<float>( wall_along_y ? 1.0f : 0.5f + half_width );

                        for( const auto face : {
                                 face_north, face_south, face_west, face_east
                             } ) {
                            const auto broad = static_cast<bool>( wall_along_y ? ( face == face_west || face == face_east ) :
                                                                  ( face == face_north || face == face_south ) );
                            if( broad ) {
                                const auto dx = static_cast<int>( face == face_west ? -1 : face == face_east ? 1 : 0 );
                                const auto dy = static_cast<int>( face == face_north ? -1 : face == face_south ? 1 : 0 );
                                const auto &material = chunk_tiles.face_material( { .bub = bub, .wall = *surround, .dx = dx, .dy = dy } );
                                const auto ref = light_ref( wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = lintel, .top = ts.height, .left = 0.0f, .right = 1.0f, .ref = ref, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = 0.0f, .top = lintel, .left = 0.0f, .right = jamb, .ref = ref, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                                emit_wall_section( { .sink = sink, .t = material, .x = lx, .y = ly, .face = face,
                                                     .bottom = 0.0f, .top = lintel, .left = 1.0f - jamb, .right = 1.0f, .ref = ref, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 } );
                            }
                        }
                        emit_wall_cap( { .sink = sink, .t = *surround, .x = lx, .y = ly, .z = ts.height, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1,
                                         .ref = wall_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );

                        if( !terrain.open_door ) {
                            if( terrain.backdrop.valid ) {
                                emit_pane( { .sink = sink, .s = terrain.backdrop, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = floor_height,
                                             .height = lintel - floor_height, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ),
                                             .half_thickness = half_width + 0.001f, .half_width = 0.5f - jamb } );
                            }
                            emit_pane( { .sink = sink, .s = terrain.sprite, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = floor_height,
                                         .height = lintel - floor_height, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ),
                                         .half_thickness = half_width + 0.002f, .half_width = 0.5f - jamb } );
                            break;
                        }

                        if( terrain.closed_door_id != t_null ) {
                            const auto &closed_info = cache.terrain( { .id = terrain.closed_door_id, .tiles = tiles, .subtile = ter_subtile, .rota = ter_rota } );
                            const auto &leaf_sprite = closed_info.main().valid ? closed_info.main() : terrain.main();
                            if( leaf_sprite.valid ) {
                                const auto z_base = floor_height;
                                const auto z_top = lintel;
                                constexpr auto leaf_half_thick = static_cast<float>( 0.015f );
                                const auto leaf_ref = light_ref( make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) );
                                const auto uv = uvs_t{{ { leaf_sprite.u0, leaf_sprite.v0 }, { leaf_sprite.u1, leaf_sprite.v0 },
                                        { leaf_sprite.u1, leaf_sprite.v1 }, { leaf_sprite.u0, leaf_sprite.v1 }
                                    }};
                                const auto v_edge = static_cast<float>( leaf_sprite.v0 + ( leaf_sprite.v1 - leaf_sprite.v0 ) *
                                                                        0.08f );
                                const auto edge_uv = uvs_t{{ { leaf_sprite.u0, leaf_sprite.v0 }, { leaf_sprite.u1, leaf_sprite.v0 },
                                        { leaf_sprite.u1, v_edge }, { leaf_sprite.u0, v_edge }
                                    }};

                                if( !wall_along_y ) {
                                    const auto north_in = static_cast<bool>( chunk_tiles.indoor_floor( bub + tripoint_rel_ms( 0, -1,
                                                          0 ) ) );
                                    const auto south_in = static_cast<bool>( chunk_tiles.indoor_floor( bub + tripoint_rel_ms( 0, 1,
                                                          0 ) ) );
                                    const auto swing_dir = static_cast<int>( ( north_in && !south_in ) ? -1 : 1 );
                                    const auto cy = static_cast<float>( ly ) + 0.5f;
                                    const auto y_start = static_cast<float>( swing_dir > 0 ? cy + half_width : cy - half_width );
                                    const auto y_end = open_door_tip( y_start, swing_dir );
                                    const auto y_min = std::min( y_start, y_end );
                                    const auto y_max = std::max( y_start, y_end );
                                    const auto hx = static_cast<float>( lx ) + jamb;

                                    for( const auto side : { -leaf_half_thick, leaf_half_thick } ) {
                                        const auto cx_side = hx + side;
                                        const auto c = corners_t{{ { cx_side, y_max, z_top }, { cx_side, y_min, z_top },
                                                { cx_side, y_min, z_base }, { cx_side, y_max, z_base }
                                            }};
                                        sink.add( { .sprite = leaf_sprite, .corners = c, .uvs = uv, .ref = leaf_ref } );
                                        sink.add( { .sprite = leaf_sprite, .corners = reversed_winding( c ), .uvs = reversed_winding( uv ), .ref = leaf_ref } );
                                    }
                                    const auto top = corners_t{{ { hx - leaf_half_thick, y_min, z_top }, { hx + leaf_half_thick, y_min, z_top },
                                            { hx + leaf_half_thick, y_max, z_top }, { hx - leaf_half_thick, y_max, z_top }
                                        }};
                                    sink.add( { .sprite = leaf_sprite, .corners = top, .uvs = edge_uv, .ref = leaf_ref } );
                                } else {
                                    const auto west_in = static_cast<bool>( chunk_tiles.indoor_floor( bub + tripoint_rel_ms( -1, 0,
                                                                            0 ) ) );
                                    const auto east_in = static_cast<bool>( chunk_tiles.indoor_floor( bub + tripoint_rel_ms( 1, 0,
                                                                            0 ) ) );
                                    const auto swing_dir = static_cast<int>( ( west_in && !east_in ) ? -1 : 1 );
                                    const auto cx = static_cast<float>( lx ) + 0.5f;
                                    const auto x_start = static_cast<float>( swing_dir > 0 ? cx + half_width : cx - half_width );
                                    const auto x_end = open_door_tip( x_start, swing_dir );
                                    const auto x_min = std::min( x_start, x_end );
                                    const auto x_max = std::max( x_start, x_end );
                                    const auto hy = static_cast<float>( ly ) + jamb;

                                    for( const auto side : { -leaf_half_thick, leaf_half_thick } ) {
                                        const auto cy_side = hy + side;
                                        const auto c = corners_t{{ { x_min, cy_side, z_top }, { x_max, cy_side, z_top },
                                                { x_max, cy_side, z_base }, { x_min, cy_side, z_base }
                                            }};
                                        sink.add( { .sprite = leaf_sprite, .corners = c, .uvs = uv, .ref = leaf_ref } );
                                        sink.add( { .sprite = leaf_sprite, .corners = reversed_winding( c ), .uvs = reversed_winding( uv ), .ref = leaf_ref } );
                                    }
                                    const auto top = corners_t{{ { x_min, hy - leaf_half_thick, z_top }, { x_max, hy - leaf_half_thick, z_top },
                                            { x_max, hy + leaf_half_thick, z_top }, { x_min, hy + leaf_half_thick, z_top }
                                        }};
                                    sink.add( { .sprite = leaf_sprite, .corners = top, .uvs = edge_uv, .ref = leaf_ref } );
                                }
                            }
                        }
                        break;
                    }
                    if( terrain.backdrop.valid ) {
                        emit_pane( { .sink = sink, .s = terrain.backdrop, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = floor_height,
                                     .height = ts.height - floor_height, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) } );
                    }
                    emit_pane( { .sink = sink, .s = terrain.sprite, .x = lx, .y = ly, .along_x = !wall_along_y, .z_base = floor_height,
                                 .height = ts.height - floor_height, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) } );
                    break;
                }
                case shape_kind::empty:
                case shape_kind::prism:
                    break;
            }

            // Furniture stands on the floor; it is skipped inside walls and under trees.
            const auto &fs = furn.sh;
            if( ts.kind != shape_kind::floor || fs.kind == shape_kind::empty ) {
                continue;
            }
            if( fs.kind == shape_kind::upright ) {
                mesh_uprights.push_back( { .lx = static_cast<uint8_t>( lx ), .ly = static_cast<uint8_t>( ly ),
                                           .z_base = ts.height, .info = &furn } );
                continue;
            }
            if( fs.kind == shape_kind::pane ) {
                const auto along_y = static_cast<bool>( ( chunk_tiles.terrain_at( lx - 1, ly ) &&
                                                        chunk_tiles.terrain_at( lx - 1, ly )->sh.kind == shape_kind::block ) ||
                                                        ( chunk_tiles.terrain_at( lx + 1, ly ) &&
                                                                chunk_tiles.terrain_at( lx + 1, ly )->sh.kind == shape_kind::block ) );
                emit_pane( { .sink = sink, .s = furn.main(), .x = lx, .y = ly, .along_x = !along_y, .z_base = ts.height, .height = fs.height,
                             .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = 230 } ) } );
                continue;
            }
            if( fs.kind == shape_kind::floor ) {
                emit_top_layers( { .sink = sink, .t = furn, .x = lx, .y = ly, .z = fs.height,
                                   .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                continue;
            }
            if( fs.kind == shape_kind::block ) {
                // Full-volume natural objects keep their complete texture on each face;
                // there is no cabinet lid split, wall-facing placement or bench underside.
                emit_top_layers( { .sink = sink, .t = furn, .x = lx, .y = ly, .z = fs.height,
                                   .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                if( fs.height > cutaway_stub_height ) {
                    // The cutaway can remove the top near the player. Close the remaining
                    // volume at its cut height, as walls do, instead of exposing a hollow box.
                    emit_top( { .sink = sink, .s = furn.backdrop.valid ? furn.backdrop : furn.main(), .x = lx, .y = ly,
                                .z = cutaway_stub_height, .ref = make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ) } );
                }
                const auto sides = std::array<face_bit, 4> {{ face_north, face_south, face_west, face_east }};
                const auto offsets = std::array<point, 4> {{
                        point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 )
                    }
                };
                for( auto i = size_t( 0 ); i < sides.size(); ++i ) {
                    emit_side_layers( { .sink = sink, .t = furn, .x = lx, .y = ly, .face = sides[i], .z_base = ts.height, .z_top = fs.height,
                                        .ref = make_ref( { .lx = lx, .ly = ly, .dx = offsets[i].x, .dy = offsets[i].y, .dz = 0, .shade = face_shade[i + 1] } ),
                                        .band = sprite_band{ .top = 0.0f, .bottom = 1.0f } } );
                }
                continue;
            }
            // A box sized to the sprite's opaque part: lid rows on top, the front rows (tall
            // pieces like lockers) or the lid's front edge (low ones like benches, whose front
            // rows are mostly legs and shadow) on the sides.
            const auto &spr = furn.main();
            constexpr auto low_box = static_cast<float>( 0.6f );
            auto layout = box_layout( furniture_box_layout( { .ox0 = spr.opaque_x0, .oy0 = spr.opaque_y0, .ox1 = spr.opaque_x1,
                                      .oy1 = spr.opaque_y1, .front = front_fraction( fs.height - ts.height ), .low = fs.height < low_box } ) );
            // The same furniture next door: a row of lockers, a long bench.
            const auto here_furn = furn_id( self->get_furn( p ) );
            const auto dirs = std::array<point, 4> {{ point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 ) }};
            const auto dir_faces = std::array<face_bit, 4> {{ face_north, face_south, face_west, face_east }};
            auto joined = uint8_t( 0 );
            for( auto i = size_t( 0 ); i < dirs.size(); ++i ) {
                if( chunk_tiles.furniture_at( lx + dirs[i].x, ly + dirs[i].y ) == here_furn ) {
                    joined |= dir_faces[i];
                }
            }
            const auto row_x = static_cast<bool>( ( joined & ( face_west | face_east ) ) != 0 );
            const auto row_y = static_cast<bool>( !row_x && ( joined & ( face_north | face_south ) ) != 0 );
            // A connected sprite's rotation says how it joins its neighbours, not where it faces.
            auto facing = static_cast<int>( furn_subtile >= 0 ? 0 : spr.facing );
            if( fs.slab > 0.0f ) {
                // A bench or table runs along its row (x before the turn): a seat about 0.6 tile
                // wide across the middle of the tile, the length of the tile when it continues
                // next door, a little shorter when it stands alone.
                constexpr auto seat = static_cast<float>( 0.65f );
                constexpr auto lone = static_cast<float>( 0.7f );
                const auto continues = static_cast<bool>( ( joined & ( face_west | face_east | face_north |
                                       face_south ) ) != 0 );
                layout.y0 = ( 1.0f - seat ) * 0.5f;
                layout.y1 = layout.y0 + seat;
                layout.x0 = continues ? 0.0f : ( 1.0f - lone ) * 0.5f;
                layout.x1 = continues ? 1.0f : layout.x0 + lone;
                if( row_x || row_y ) {
                    facing = row_y ? 1 : 0;
                }
            } else if( facing == 0 || furn.depth_scale < 1.0f ) {
                // Lockers and cabinets stand with their back to a wall, their row along it.
                facing = back_to_wall( row_x, row_y, [ &chunk_tiles, lx, ly, &bub ]( const point & d ) -> bool {
                    const tile_info *t = chunk_tiles.terrain_at( lx + d.x, ly + d.y );
                    return t && t->wall_connection && t->sh.kind == shape_kind::block &&
                    !chunk_tiles.trim_corner_at( bub + tripoint_rel_ms( d.x, d.y, 0 ) );
                } );
            }
            const auto refs = std::array<light_ref, 5> {{
                    make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 0, .dz = 0, .shade = face_shade[0] } ), make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = -1, .dz = 0, .shade = face_shade[1] } ),
                    make_ref( { .lx = lx, .ly = ly, .dx = 0, .dy = 1, .dz = 0, .shade = face_shade[2] } ), make_ref( { .lx = lx, .ly = ly, .dx = -1, .dy = 0, .dz = 0, .shade = face_shade[3] } ),
                    make_ref( { .lx = lx, .ly = ly, .dx = 1, .dy = 0, .dz = 0, .shade = face_shade[4] } )
                }
            };
            // A slab (bench, table) sits below the cutaway, so it needs no hidden sides; the
            // faces between its joined tiles would only fight with its top along the seams.
            // Thin walls leave space between their face and the old tile boundary. Extend
            // solid cabinetry into that space, keeping its front and row joins in place.
            auto wall_contacts = uint8_t( 0 );
            if( fs.slab == 0.0f ) {
                for( auto i = size_t( 0 ); i < dirs.size(); ++i ) {
                    const tile_info *t = chunk_tiles.terrain_at( lx + dirs[i].x, ly + dirs[i].y );
                    if( t && t->thin_wall && t->sh.kind == shape_kind::block &&
                        !chunk_tiles.trim_corner_at( bub + tripoint_rel_ms( dirs[i].x, dirs[i].y, 0 ) ) ) {
                        wall_contacts |= dir_faces[i];
                    }
                }
            }
            const auto box_faces = uint8_t( visible_faces( fs, nb ) | ( fs.slab > 0.0f ? 0 : all_sides ) );
            emit_box( { .sink = sink, .s = spr, .x = lx, .y = ly, .z0 = ts.height, .z1 = fs.height, .b = layout, .faces = box_faces, .refs = refs, .facing = facing, .joined = joined,
                        .wall_contacts = wall_contacts, .slab = fs.slab, .depth_scale = furn.depth_scale } );

        }
    }
    auto mesh = chunk_mesh( sink.finish() );
    mesh.uprights = std::move( mesh_uprights );
    return mesh;
}

} // namespace r3d

#endif // defined(TILES)
