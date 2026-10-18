#if defined(TILES)

#include "r3d_entities.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "units_angle.h"

#include "avatar.h"
#include "debug.h"
#include "cata_tiles.h"
#include "character.h"
#include "creature.h"
#include "game.h"
#include "item.h"
#include "map/map.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "r3d_shapes.h"
#include "r3d_scene_rules.h"
#include "r3d_decals.h"
#include "enums.h"
#include "units_utility.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"
#include "vehicle/veh_type.h"
#include "vehicle/vpart_position.h"

namespace r3d
{

namespace
{

constexpr auto creature_radius = static_cast<int>
                                 ( 45 ); // tiles around the view that are searched for creatures
constexpr auto item_radius = static_cast<int>
                             ( 20 );     // and for items (every tile costs a map lookup)
constexpr auto marker_box_height = static_cast<float>( 1.0f );
constexpr auto item_size = static_cast<float>( 0.6f );

// Light colour with a zero alpha: no depth pull (see r3d_block.vert), for everything in the
// entity mesh that is not the top of a billboard.
auto no_pull( uint32_t color ) -> uint32_t
{ return color & 0x00ffffffu; }

// Geometry and depth shared by the sprite layers of one billboard.
struct billboard_geometry {
    quad_sink &sink;
    const vec3 &right_world;
    const vec3 &up_world;
    float scale;
    float z_scale;
    float tx;
    float ty;
    float tz;
    float cx;
    float cy;
    float z_base;
    bool mirrored;
    int depth;
    const std::array<uint32_t, 4> &colors;

    auto emit_layer( const sprite_ref &s ) -> void {
        if( !s.valid ) {
            return;
        }
        const auto lw = static_cast<float>( std::clamp( s.size_x, 0.25f, 4.0f ) * scale );
        const auto lh = static_cast<float>( std::clamp( s.size_y, 0.25f, 6.0f ) * scale );
        const auto rx = static_cast<float>( right_world.x * lw * 0.5f );
        const auto ry = static_cast<float>( -right_world.y * lw * 0.5f );
        const auto ux = static_cast<float>( up_world.x * lh );
        const auto uy = static_cast<float>( -up_world.y * lh );
        const auto uz = static_cast<float>( up_world.z * lh / z_scale );
        // The flat view draws a sprite offset_y pixels below its tile's top edge, so its bottom
        // row lands (offset_y + height - tile) below the tile's bottom: the same shift here,
        // along the screen's up, keeps a 48-row body and 36-row clothes on the same feet.
        const auto drop = static_cast<float>( ( s.offset_y + s.size_y - 1.0f ) * scale );
        const auto side = static_cast<float>( s.offset_x * scale );
        const auto lift = static_cast<float>( 0.01f * static_cast<float>( depth++ ) );
        const auto ox = static_cast<float>( cx + tx * lift - up_world.x * drop + right_world.x * side );
        const auto oy = static_cast<float>( cy + ty * lift + up_world.y * drop - right_world.y * side );
        const auto oz = static_cast<float>( z_base + tz * lift - up_world.z * drop / z_scale );
        const auto ul = static_cast<float>( mirrored ? s.u1 : s.u0 );
        const auto ur = static_cast<float>( mirrored ? s.u0 : s.u1 );
        const auto uv = uvs_t{{ { ul, s.v0 }, { ur, s.v0 }, { ur, s.v1 }, { ul, s.v1 } }};
        const auto c = corners_t{{ { ox - rx + ux, oy - ry + uy, oz + uz }, { ox + rx + ux, oy + ry + uy, oz + uz },
                { ox + rx, oy + ry, oz }, { ox - rx, oy - ry, oz }
            }
        };
        sink.add_colored( { .sprite = s, .corners = c, .uvs = uv, .ref = light_ref{}, .colors = colors } );
    }
};

// Camera-facing upright quads standing on the tile centre, sized like the first layer's sprite
// is in the flat view (big trees are several tiles tall). Later layers (clothing over a body) sit
// slightly closer to the camera. Corners are in scene-frame tile space.
struct emit_billboard_options {
    quad_sink &sink;
    const std::vector<const tile_info *> &layers;
    const vec3 &right_world;
    const vec3 &up_world;
    const vec3 &toward_world;
    float z_scale;
    float cx;
    float cy;
    float z_base;
    uint32_t color;
    bool mirrored = false;
    float scale = 1.0f;
};

auto emit_billboard( const emit_billboard_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &layers = options.layers;
    const auto &right_world = options.right_world;
    const auto &up_world = options.up_world;
    const auto &toward_world = options.toward_world;
    auto z_scale = options.z_scale;
    auto cx = options.cx;
    auto cy = options.cy;
    auto z_base = options.z_base;
    auto color = options.color;
    auto mirrored = options.mirrored;
    auto scale = options.scale;

    if( layers.empty() || !layers.front() ) {
        return;
    }
    const auto &main = layers.front()->main();
    const auto height = static_cast<float>( std::clamp( main.size_y, 0.25f, 6.0f ) * scale );
    // The camera vectors are in the right-handed world frame; tile space has y flipped.
    const auto tx = static_cast<float>( toward_world.x );
    const auto ty = static_cast<float>( -toward_world.y );
    // The camera vectors are in stretched space, where sprites keep their flat-view proportions;
    // their vertical parts shrink back into world space, where a z-level is 1 high.
    const auto tz = static_cast<float>( toward_world.z / z_scale );
    // Facing the camera squarely leans the top away from it, into whatever stands behind the
    // tile (a wall). Sliding the quad towards the camera along the line of sight keeps it where
    // it is on screen but brings the top back inside the tile's own footprint.
    const auto lean = static_cast<float>( height * std::sqrt( up_world.x * up_world.x + up_world.y *
                                          up_world.y ) );
    const auto toward_ground = static_cast<float>( std::sqrt( toward_world.x * toward_world.x +
                               toward_world.y * toward_world.y ) );
    constexpr auto half_tile = static_cast<float>( 0.45f );
    const auto pull = static_cast<float>( std::clamp( ( lean - half_tile ) / std::max( toward_ground,
                                          0.05f ), 0.0f,
                                          max_billboard_pull ) );
    // The pull only moves depth (see r3d_block.vert): the top's light alpha carries it, the base
    // keeps 0 so feet stay where they stand.
    const auto top_alpha = static_cast<uint32_t>( static_cast<uint32_t>( pull / max_billboard_pull *
                           255.0f + 0.5f ) << 24 );
    const auto colors = std::array<uint32_t, 4> {{
            ( color & 0x00ffffffu ) | top_alpha, ( color & 0x00ffffffu ) | top_alpha,
            color & 0x00ffffffu, color & 0x00ffffffu
        }
    };
    // The quad faces the camera squarely (leaning back with its up vector), so sprites keep the
    // full size and detail they have in the flat view instead of being squashed by the pitch.
    // Each layer keeps its own sprite's size, standing on the same feet: a 48-row body under
    // 36-row clothing, as the flat view draws them.
    auto geometry = billboard_geometry{ .sink = sink, .right_world = right_world, .up_world = up_world, .scale = scale, .z_scale = z_scale, .tx = tx, .ty = ty, .tz = tz,
                                        .cx = cx, .cy = cy, .z_base = z_base, .mirrored = mirrored, .depth = 0, .colors = colors };
    for( const tile_info *t : layers ) {
        if( t ) {
            if( !t->sh.ground_backdrop ) {
                geometry.emit_layer( t->backdrop );
            }
            geometry.emit_layer( t->sprite );
        }
    }
}

// Sprite lying flat at height z, size tiles wide.
struct emit_flat_options {
    quad_sink &sink;
    const tile_info &sprite;
    float cx;
    float cy;
    float z;
    uint32_t color;
    float size = item_size;
};

auto emit_flat( const emit_flat_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &sprite = options.sprite;
    auto cx = options.cx;
    auto cy = options.cy;
    auto z = options.z;
    auto color = options.color;
    auto size = options.size;

    const auto h = static_cast<float>( size * 0.5f );
    const auto c = corners_t{{ { cx - h, cy - h, z }, { cx + h, cy - h, z }, { cx + h, cy + h, z },
            { cx - h, cy + h, z }
        }
    };
    for( const sprite_ref *s : {
             &sprite.backdrop, &sprite.sprite
         } ) {
        if( !s->valid ) {
            continue;
        }
        const auto uv = uvs_t{{ { s->u0, s->v0 }, { s->u1, s->v0 }, { s->u1, s->v1 }, { s->u0, s->v1 } }};
        sink.add_colored( { .sprite = *s, .corners = c, .uvs = uv, .ref = light_ref{}, .color = no_pull( color ) } );
    }
}

// Box standing on a tile with the sprite on its four sides, so a cursor reads as a 3D outline.
struct emit_marker_box_options {
    quad_sink &sink;
    const sprite_ref &s;
    float x0;
    float y0;
    float z0;
    float z1;
    uint32_t color;
    float width = 1.0f;
};

auto emit_marker_box( const emit_marker_box_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &s = options.s;
    auto x0 = options.x0;
    auto y0 = options.y0;
    auto z0 = options.z0;
    auto z1 = options.z1;
    auto color = options.color;
    auto width = options.width;

    const auto x1 = static_cast<float>( x0 + width );
    const auto y1 = static_cast<float>( y0 + width );
    const auto uv = uvs_t{{ { s.u0, s.v0 }, { s.u1, s.v0 }, { s.u1, s.v1 }, { s.u0, s.v1 } }};
    const auto sides = std::array<std::array<float, 4>, 4> {{
            { x0, y1, x1, y1 }, { x1, y0, x0, y0 }, { x1, y1, x1, y0 }, { x0, y0, x0, y1 }
        }
    };
    for( const std::array<float, 4> &e : sides ) {
        const auto c = corners_t{{ { e[0], e[1], z1 }, { e[2], e[3], z1 }, { e[2], e[3], z0 },
                { e[0], e[1], z0 }
            }
        };
        sink.add_colored( { .sprite = s, .corners = c, .uvs = uv, .ref = light_ref{}, .color = no_pull( color ) } );
    }
}


struct point_f {
    float x = 0.0f;
    float y = 0.0f;
};

struct to_scene_transform {
    float pivot_sx = 0.0f;
    float pivot_sy = 0.0f;
    float cos_th = 1.0f;
    float sin_th = 0.0f;

    auto apply( float local_forward, float local_right ) const -> point_f {
        return {
            .x = pivot_sx + local_forward * cos_th - local_right * sin_th,
            .y = pivot_sy + local_forward * sin_th + local_right * cos_th
        };
    }
};

struct emit_vehicle_tile_quads_options {
    quad_sink &sink;
    const tile_info &info;
    float rel_forward = 0.0f;
    float rel_right = 0.0f;
    float z = 0.0f;
    uint32_t color = 0;
    to_scene_transform transform;
    float f_min = -0.5f;
    float f_max = 0.5f;
    float r_min = -0.5f;
    float r_max = 0.5f;
};

auto compute_side_shade( float local_f, float local_r, float cos_th, float sin_th ) -> float
{
    const auto nx = local_f * cos_th - local_r * sin_th;
    const auto ny = local_f * sin_th + local_r * cos_th;
    const auto x_shade = ( nx >= 0.0f ) ? 0.725f : 0.588f; // East 0.73, West 0.59
    const auto y_shade = ( ny >= 0.0f ) ? 0.831f : 0.647f; // South 0.83, North 0.65
    return std::clamp( nx * nx * x_shade + ny * ny * y_shade, 0.4f, 1.0f );
}

auto modulate_light( uint32_t light, float shade ) -> uint32_t
{
    const auto r = static_cast<uint8_t>( std::clamp( static_cast<int>( ( light & 0xffu ) * shade ), 0, 255 ) );
    const auto g = static_cast<uint8_t>( std::clamp( static_cast<int>( ( ( light >> 8u ) & 0xffu ) * shade ), 0, 255 ) );
    const auto b = static_cast<uint8_t>( std::clamp( static_cast<int>( ( ( light >> 16u ) & 0xffu ) * shade ), 0, 255 ) );
    return pack_light( r, g, b );
}

auto make_sprite_uvs( const sprite_ref &s ) -> uvs_t
{
    const auto left_u = static_cast<float>( s.flip ? s.u1 : s.u0 );
    const auto right_u = static_cast<float>( s.flip ? s.u0 : s.u1 );
    const auto base_uv = uvs_t{{ { left_u, s.v0 }, { right_u, s.v0 }, { right_u, s.v1 }, { left_u, s.v1 } }};
    auto uv = uvs_t{};
    for( auto i = size_t( 0 ); i < 4; ++i ) {
        uv[i] = base_uv[( i + 4 - static_cast<size_t>( s.quarter_turns & 3 ) ) % 4];
    }
    return uv;
}

auto make_solid_uvs( const sprite_ref &s ) -> uvs_t
{
    const auto fx = static_cast<float>( std::clamp( ( s.opaque_x0 + s.opaque_x1 ) * 0.5f, 0.05f, 0.95f ) );
    const auto fy = static_cast<float>( std::clamp( s.opaque_y0 * 0.4f + s.opaque_y1 * 0.6f, 0.05f, 0.95f ) );
    const auto u_mid = static_cast<float>( s.u0 + ( s.u1 - s.u0 ) * fx );
    const auto v_mid = static_cast<float>( s.v0 + ( s.v1 - s.v0 ) * fy );
    return uvs_t{{ { u_mid, v_mid }, { u_mid, v_mid }, { u_mid, v_mid }, { u_mid, v_mid } }};
}

auto map_sprite_uv( const sprite_ref &s, float gx, float gy ) -> std::array<float, 2>
{
    const auto ul = static_cast<float>( s.flip ? s.u1 : s.u0 );
    const auto ur = static_cast<float>( s.flip ? s.u0 : s.u1 );
    auto sx = 0.0f;
    auto sy = 0.0f;
    switch( s.quarter_turns & 3 ) {
        case 0:
            sx = gx;
            sy = gy;
            break;
        case 1:
            sx = gy;
            sy = 1.0f - gx;
            break;
        case 2:
            sx = 1.0f - gx;
            sy = 1.0f - gy;
            break;
        case 3:
            sx = 1.0f - gy;
            sy = gx;
            break;
    }
    return std::array<float, 2>{ ul + ( ur - ul ) * sx, s.v0 + ( s.v1 - s.v0 ) * sy };
}

auto emit_vehicle_tile_quads( const emit_vehicle_tile_quads_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &info = options.info;
    const auto rel_forward = options.rel_forward;
    const auto rel_right = options.rel_right;
    const auto z = options.z;
    const auto color = options.color;
    const auto &transform = options.transform;

    const auto cell_f_min = static_cast<float>( rel_forward + options.f_min );
    const auto cell_f_max = static_cast<float>( rel_forward + options.f_max );
    const auto cell_r_min = static_cast<float>( rel_right + options.r_min );
    const auto cell_r_max = static_cast<float>( rel_right + options.r_max );

    auto lift = static_cast<float>( 0.0f );
    for( const sprite_ref *sprite : { &info.backdrop, &info.sprite } ) {
        if( !sprite->valid ) {
            continue;
        }
        const auto &s = *sprite;
        const auto r_left = static_cast<float>( rel_right - 0.5f + s.offset_x );
        const auto r_right = static_cast<float>( r_left + s.size_x );
        const auto f_top = static_cast<float>( rel_forward + 0.5f - s.offset_y );
        const auto f_bot = static_cast<float>( f_top - s.size_y );

        const auto c_r_left = std::clamp( r_left, cell_r_min, cell_r_max );
        const auto c_r_right = std::clamp( r_right, cell_r_min, cell_r_max );
        const auto c_f_top = std::clamp( f_top, cell_f_min, cell_f_max );
        const auto c_f_bot = std::clamp( f_bot, cell_f_min, cell_f_max );

        if( c_r_right <= c_r_left || c_f_top <= c_f_bot ) {
            continue;
        }

        const auto gx_l = ( c_r_left - r_left ) / std::max( s.size_x, 0.001f );
        const auto gx_r = ( c_r_right - r_left ) / std::max( s.size_x, 0.001f );
        const auto gy_top = ( f_top - c_f_top ) / std::max( s.size_y, 0.001f );
        const auto gy_bot = ( f_top - c_f_bot ) / std::max( s.size_y, 0.001f );

        const auto p0 = transform.apply( c_f_top, c_r_left );
        const auto p1 = transform.apply( c_f_top, c_r_right );
        const auto p2 = transform.apply( c_f_bot, c_r_right );
        const auto p3 = transform.apply( c_f_bot, c_r_left );

        const auto uv0 = map_sprite_uv( s, gx_l, gy_top );
        const auto uv1 = map_sprite_uv( s, gx_r, gy_top );
        const auto uv2 = map_sprite_uv( s, gx_r, gy_bot );
        const auto uv3 = map_sprite_uv( s, gx_l, gy_bot );

        const auto cur_z = static_cast<float>( z + lift );
        const auto corners = corners_t{{
            { p0.x, p0.y, cur_z },
            { p1.x, p1.y, cur_z },
            { p2.x, p2.y, cur_z },
            { p3.x, p3.y, cur_z }
        }};
        const auto uvs = uvs_t{{ uv0, uv1, uv2, uv3 }};
        sink.add_colored( { .sprite = s, .corners = corners, .uvs = uvs, .ref = light_ref{}, .color = no_pull( color ) } );
        lift += 0.004f;
    }
}

struct vehicle_cell_data {
    tripoint_mnt_veh mount;
    int main_part_idx = -1;
    int roof_part_idx = -1;
    bool has_roof = false;
    bool show_roof = false;
    bool has_wheel = false;
    bool is_windshield = false;
    bool is_opaque_wall = false;
    bool is_openable = false;
    bool is_door = false;
    bool is_tall_cabin = false;
    bool is_interior = false;
    float height = 0.0f;
    uint32_t light = 0;
};

struct add_vehicles_options {
    quad_sink &sink;
    const entity_view &view;
    map &here;
    const avatar &you;
};

auto add_vehicles( const add_vehicles_options &options ) -> void
{
    auto &sink = options.sink;
    const auto &view = options.view;
    auto &here = options.here;
    static_cast<void>( options.you );

    for( const wrapped_vehicle &wrapped : here.get_vehicles() ) {
        auto &veh = *wrapped.v;
        const auto veh_bub = veh.bub_ms_location();
        const auto veh_z = veh_bub.z();
        if( veh_z < view.z_lo || veh_z > view.z_hi ) {
            continue;
        }

        const auto pivot_mnt = veh.pivot_anchor[0];
        const auto pivot_sx = static_cast<float>( veh_bub.x() - view.center.x() ) + 0.5f;
        const auto pivot_sy = static_cast<float>( veh_bub.y() - view.center.y() ) + 0.5f;
        const auto theta = static_cast<float>( units::to_radians( veh.face.dir() ) );
        const auto cos_th = static_cast<float>( std::cos( theta ) );
        const auto sin_th = static_cast<float>( std::sin( theta ) );

        const auto transform = to_scene_transform{
            .pivot_sx = pivot_sx,
            .pivot_sy = pivot_sy,
            .cos_th = cos_th,
            .sin_th = sin_th
        };

        const auto z_base = static_cast<float>( static_cast<float>( veh_z - view.center.z() ) + floor_height + 0.02f );

        auto occupied_mounts = std::vector<tripoint_mnt_veh>{};
        auto unique_mounts = std::set<tripoint_mnt_veh>{};
        for( auto i = 0; i < veh.part_count(); ++i ) {
            const auto &vp = veh.cpart( i );
            if( !vp.removed && unique_mounts.insert( vp.mount ).second ) {
                occupied_mounts.push_back( vp.mount );
            }
        }

        auto cells = std::map<point, vehicle_cell_data>{};
        constexpr auto ON_ROOF_Z = 9;

        for( const tripoint_mnt_veh &mount : occupied_mounts ) {
            const auto p = veh.mount_to_bubble( mount );
            if( !here.inbounds( p ) || p.z() < view.z_lo || p.z() > view.z_hi ) {
                continue;
            }
            const auto light = static_cast<uint32_t>( view.light( p ) );
            if( light == pack_light( 0, 0, 0 ) ) {
                continue;
            }
            const auto parts_in_cell = veh.parts_at_relative( mount, true );
            if( parts_in_cell.empty() ) {
                continue;
            }

            auto data = vehicle_cell_data{
                .mount = mount,
                .main_part_idx = -1,
                .roof_part_idx = -1,
                .has_roof = false,
                .show_roof = false,
                .has_wheel = false,
                .is_windshield = false,
                .is_opaque_wall = false,
                .is_openable = false,
                .is_door = false,
                .is_tall_cabin = false,
                .is_interior = false,
                .height = vehicle_body_h,
                .light = light
            };

            for( const auto p_idx : parts_in_cell ) {
                const auto &vp = veh.cpart( p_idx );
                const auto &vpi = veh.part_info( p_idx );
                if( !vp.is_broken() && ( veh.part_flag( p_idx, "ROOF" ) || veh.part_flag( p_idx, "SOLAR_PANEL" ) ||
                                         veh.part_flag( p_idx, "TURRET" ) || vpi.location == "on_roof" ) ) {
                    data.has_roof = true;
                    if( data.roof_part_idx == -1 ) {
                        data.roof_part_idx = p_idx;
                    }
                }
                if( veh.part_flag( p_idx, "WINDSHIELD" ) ) {
                    data.is_windshield = true;
                    data.is_tall_cabin = true;
                }
                if( veh.part_flag( p_idx, "OPAQUE" ) ) {
                    data.is_tall_cabin = true;
                    if( !veh.part_flag( p_idx, "WINDSHIELD" ) && !veh.part_flag( p_idx, "OPENABLE" ) ) {
                        data.is_opaque_wall = true;
                    }
                }
                if( veh.part_flag( p_idx, "OBSTACLE" ) && !vp.open ) {
                    data.is_tall_cabin = true;
                }
                if( veh.part_flag( p_idx, "WHEEL" ) ) {
                    data.has_wheel = true;
                }
                if( veh.part_flag( p_idx, "OPENABLE" ) ) {
                    data.is_openable = true;
                }
                if( veh.part_flag( p_idx, "DOOR" ) || vpi.location == "door" ) {
                    data.is_door = true;
                }
                if( veh.part_flag( p_idx, "BOARDABLE" ) || veh.part_flag( p_idx, "AISLE" ) ||
                    veh.part_flag( p_idx, "SEAT" ) || veh.part_flag( p_idx, "BED" ) ||
                    ( veh.part_flag( p_idx, "OPENABLE" ) && vp.open ) ) {
                    data.is_interior = true;
                }
                if( vpi.z_order < ON_ROOF_Z ) {
                    if( data.main_part_idx == -1 || vpi.z_order > veh.part_info( data.main_part_idx ).z_order ) {
                        data.main_part_idx = p_idx;
                    }
                }
            }
            if( data.main_part_idx == -1 ) {
                data.main_part_idx = parts_in_cell.front();
            }

            if( !data.has_roof && vpart_position( const_cast<vehicle&>( veh ), data.main_part_idx ).is_inside() ) {
                data.has_roof = true;
            }

            // Always emit roof; driver cabin visibility is handled by shader cutaway uniforms.
            data.show_roof = data.has_roof;

            if( data.has_roof || data.is_tall_cabin ) {
                data.height = vehicle_cabin_h;
            } else if( data.is_interior ) {
                data.height = chassis_floor_h;
            } else {
                data.height = vehicle_body_h;
            }

            cells[point( mount.x(), mount.y() )] = data;
        }

        auto min_mount_x = std::numeric_limits<int>::max();
        auto max_mount_x = std::numeric_limits<int>::min();
        for( const auto &[pt, cell] : cells ) {
            min_mount_x = std::min( min_mount_x, cell.mount.x() );
            max_mount_x = std::max( max_mount_x, cell.mount.x() );
        }

        auto min_y_front = std::numeric_limits<int>::max();
        auto max_y_front = std::numeric_limits<int>::min();
        auto min_y_rear = std::numeric_limits<int>::max();
        auto max_y_rear = std::numeric_limits<int>::min();
        for( const auto &[pt, cell] : cells ) {
            if( cell.mount.x() == max_mount_x ) {
                min_y_front = std::min( min_y_front, cell.mount.y() );
                max_y_front = std::max( max_y_front, cell.mount.y() );
            }
            if( cell.mount.x() == min_mount_x ) {
                min_y_rear = std::min( min_y_rear, cell.mount.y() );
                max_y_rear = std::max( max_y_rear, cell.mount.y() );
            }
        }

        const sprite_ref *best_body_spr = nullptr;
        auto best_body_color = uint32_t{ 0 };
        auto best_score = -1;

        for( auto i = 0; i < veh.part_count(); ++i ) {
            if( veh.cpart( i ).removed || veh.cpart( i ).is_broken() ) {
                continue;
            }
            auto modifier = char{ 0 };
            const auto &id = veh.part_id_string( i, false, modifier );
            if( id.is_null() ) {
                continue;
            }
            const auto id_str = id.str();
            auto score = 0;
            if( id_str.find( "hood" ) != std::string::npos || id_str.find( "trunk" ) != std::string::npos ) {
                score = 40;
            } else if( id_str.find( "quarterpanel" ) != std::string::npos || id_str.find( "panel" ) != std::string::npos ||
                       id_str.find( "board" ) != std::string::npos || id_str.find( "halfboard" ) != std::string::npos ) {
                score = 30;
            } else if( id_str.find( "door" ) != std::string::npos ) {
                score = 20;
            } else if( veh.part_flag( i, "OBSTACLE" ) || veh.part_flag( i, "OPAQUE" ) ) {
                score = 10;
            } else {
                score = 1;
            }

            if( score <= best_score ) {
                continue;
            }

            const auto subtile = modifier == 1 ? open_ : modifier == 2 ? broken : 0;
            auto rota = 0;
            if( veh.cpart( i ).direction != 0_degrees ) {
                const auto dir_deg = static_cast<int>( std::lround( units::to_degrees( veh.cpart( i ).direction ) ) );
                rota = ( ( dir_deg / 90 ) % 4 + 4 ) % 4;
            }

            const auto &info = view.cache.named( {
                .category = C_VEHICLE_PART,
                .id = "vp_" + id_str,
                .tiles = view.tiles,
                .subtile = subtile,
                .rota = rota,
                .allow_unknown = false
            } );

            const auto *spr = info.sprite.valid ? &info.sprite : ( info.backdrop.valid ? &info.backdrop : nullptr );
            if( spr && spr->valid ) {
                best_score = score;
                best_body_spr = spr;
                best_body_color = spr->body_color != 0 ? spr->body_color : spr->top_color;
            }
        }
        static_cast<void>( best_body_color );

        for( const auto &[pt, cell] : cells ) {
            const auto rel_forward = static_cast<float>( cell.mount.x() - pivot_mnt.x() );
            const auto rel_right = static_cast<float>( cell.mount.y() - pivot_mnt.y() );

            auto main_sprite_opt = std::optional<std::reference_wrapper<const tile_info>>{};
            if( cell.main_part_idx >= 0 ) {
                auto modifier = char{ 0 };
                const auto &id = veh.part_id_string( cell.main_part_idx, false, modifier );
                if( !id.is_null() ) {
                    const auto subtile = modifier == 1 ? open_ : modifier == 2 ? broken : 0;
                    auto rota = 0;
                    if( veh.cpart( cell.main_part_idx ).direction != 0_degrees ) {
                        const auto dir_deg = static_cast<int>( std::lround( units::to_degrees( veh.cpart( cell.main_part_idx ).direction ) ) );
                        rota = ( ( dir_deg / 90 ) % 4 + 4 ) % 4;
                    }
                    const auto &info = view.cache.named( {
                        .category = C_VEHICLE_PART,
                        .id = "vp_" + id.str(),
                        .tiles = view.tiles,
                        .subtile = subtile,
                        .rota = rota,
                        .allow_unknown = false
                    } );
                    if( info.sprite.valid || info.backdrop.valid ) {
                        main_sprite_opt = info;
                    }
                }
            }

            auto roof_sprite_opt = std::optional<std::reference_wrapper<const tile_info>>{};
            if( cell.show_roof && cell.has_roof && cell.roof_part_idx >= 0 ) {
                auto modifier = char{ 0 };
                const auto &id = veh.part_id_string( cell.roof_part_idx, true, modifier );
                if( !id.is_null() ) {
                    const auto subtile = modifier == 1 ? open_ : modifier == 2 ? broken : 0;
                    auto rota = 0;
                    if( veh.cpart( cell.roof_part_idx ).direction != 0_degrees ) {
                        const auto dir_deg = static_cast<int>( std::lround( units::to_degrees( veh.cpart( cell.roof_part_idx ).direction ) ) );
                        rota = ( ( dir_deg / 90 ) % 4 + 4 ) % 4;
                    }
                    const auto &info = view.cache.named( {
                        .category = C_VEHICLE_PART,
                        .id = "vp_" + id.str(),
                        .tiles = view.tiles,
                        .subtile = subtile,
                        .rota = rota,
                        .allow_unknown = false
                    } );
                    if( info.sprite.valid || info.backdrop.valid ) {
                        roof_sprite_opt = info;
                    }
                }
            }

            const auto *cell_spr = ( main_sprite_opt.has_value() && main_sprite_opt->get().main().valid )
                                   ? &main_sprite_opt->get().main()
                                   : nullptr;
            const auto *side_spr = ( cell_spr && !cell.is_interior )
                                   ? cell_spr
                                   : ( best_body_spr ? best_body_spr : cell_spr );

            // Neighbor heights in local mount space
            auto h_fwd = 0.0f;
            auto h_rear = 0.0f;
            auto h_right = 0.0f;
            auto h_left = 0.0f;
            const auto nit_fwd = cells.find( point( cell.mount.x() + 1, cell.mount.y() ) );
            if( nit_fwd != cells.end() ) {
                h_fwd = nit_fwd->second.height;
            }
            const auto nit_rear = cells.find( point( cell.mount.x() - 1, cell.mount.y() ) );
            if( nit_rear != cells.end() ) {
                h_rear = nit_rear->second.height;
            }
            const auto nit_right = cells.find( point( cell.mount.x(), cell.mount.y() + 1 ) );
            if( nit_right != cells.end() ) {
                h_right = nit_right->second.height;
            }
            const auto nit_left = cells.find( point( cell.mount.x(), cell.mount.y() - 1 ) );
            if( nit_left != cells.end() ) {
                h_left = nit_left->second.height;
            }

            const auto has_fwd = nit_fwd != cells.end();
            const auto has_rear = nit_rear != cells.end();
            const auto has_right = nit_right != cells.end();
            const auto has_left = nit_left != cells.end();

            // Compute trimmed cell footprint bounds
            auto f_bot_rel = -0.5f;
            auto f_top_rel = 0.5f;
            auto r_l_rel = -0.5f;
            auto r_r_rel = 0.5f;

            const auto my_bounds = cell_spr ? get_sprite_opaque_bounds( *cell_spr ) : sprite_opaque_bounds{};

            if( !has_left ) {
                if( my_bounds.has_trim && my_bounds.local_r_min > -0.48f ) {
                    r_l_rel = my_bounds.local_r_min;
                } else {
                    for( const auto dx : { 1, -1, 2, -2 } ) {
                        const auto it = cells.find( point( cell.mount.x() + dx, cell.mount.y() ) );
                        if( it != cells.end() && cells.find( point( cell.mount.x() + dx, cell.mount.y() - 1 ) ) == cells.end() ) {
                            if( it->second.main_part_idx >= 0 ) {
                                auto mod = char{ 0 };
                                const auto &nid = veh.part_id_string( it->second.main_part_idx, false, mod );
                                if( !nid.is_null() ) {
                                    const auto &ninfo = view.cache.named( { .category = C_VEHICLE_PART, .id = "vp_" + nid.str(),
                                                                            .tiles = view.tiles, .allow_unknown = false } );
                                    if( ninfo.main().valid ) {
                                        const auto nb = get_sprite_opaque_bounds( ninfo.main() );
                                        if( nb.has_trim && nb.local_r_min > -0.48f ) {
                                            r_l_rel = nb.local_r_min;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if( !has_right ) {
                if( my_bounds.has_trim && my_bounds.local_r_max < 0.48f ) {
                    r_r_rel = my_bounds.local_r_max;
                } else {
                    for( const auto dx : { 1, -1, 2, -2 } ) {
                        const auto it = cells.find( point( cell.mount.x() + dx, cell.mount.y() ) );
                        if( it != cells.end() && cells.find( point( cell.mount.x() + dx, cell.mount.y() + 1 ) ) == cells.end() ) {
                            if( it->second.main_part_idx >= 0 ) {
                                auto mod = char{ 0 };
                                const auto &nid = veh.part_id_string( it->second.main_part_idx, false, mod );
                                if( !nid.is_null() ) {
                                    const auto &ninfo = view.cache.named( { .category = C_VEHICLE_PART, .id = "vp_" + nid.str(),
                                                                            .tiles = view.tiles, .allow_unknown = false } );
                                    if( ninfo.main().valid ) {
                                        const auto nb = get_sprite_opaque_bounds( ninfo.main() );
                                        if( nb.has_trim && nb.local_r_max < 0.48f ) {
                                            r_r_rel = nb.local_r_max;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if( !has_fwd && my_bounds.has_trim && my_bounds.local_f_max < 0.48f ) {
                f_top_rel = my_bounds.local_f_max;
            }
            if( !has_rear && my_bounds.has_trim && my_bounds.local_f_min > -0.48f ) {
                f_bot_rel = my_bounds.local_f_min;
            }

            const auto f_top = static_cast<float>( rel_forward + f_top_rel );
            const auto f_bot = static_cast<float>( rel_forward + f_bot_rel );
            const auto r_l = static_cast<float>( rel_right + r_l_rel );
            const auto r_r = static_cast<float>( rel_right + r_r_rel );

            // Greenhouse tumblehome (sloping inward up to roof)
            const auto max_tumble_r = ( r_r - r_l ) * 0.25f;
            const auto tumble_l = !has_left ? std::min( 0.12f, max_tumble_r ) : 0.0f;
            const auto tumble_r = !has_right ? std::min( 0.12f, max_tumble_r ) : 0.0f;

            const auto max_tumble_f = ( f_top - f_bot ) * 0.25f;
            const auto tumble_f = ( h_fwd <= vehicle_body_h ) ? std::min( 0.12f, max_tumble_f ) : 0.0f;
            const auto tumble_b = ( h_rear <= vehicle_body_h ) ? std::min( 0.12f, max_tumble_f ) : 0.0f;

            const auto r_l_roof = r_l + tumble_l;
            const auto r_r_roof = r_r - tumble_r;
            const auto f_top_roof = f_top - tumble_f;
            const auto f_bot_roof = f_bot + tumble_b;

            const auto is_fwd_windshield = cell.is_windshield && !cell.is_opaque_wall && h_rear >= 0.70f && h_fwd <= 0.55f;
            const auto is_rear_windshield = cell.is_windshield && !cell.is_opaque_wall && h_fwd >= 0.70f && h_rear <= 0.55f;
            const auto is_front_bumper = ( cell.height <= vehicle_body_h && cell.mount.x() == max_mount_x );
            const auto is_rear_bumper = ( cell.height <= vehicle_body_h && cell.mount.x() == min_mount_x );

            // 1. Top Surface Geometry
            if( is_fwd_windshield && main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                // Sloped front windshield: vehicle_cabin_h at rear down to vehicle_body_h at front
                const auto &ms = main_sprite_opt->get().main();
                const auto z_front = static_cast<float>( z_base + vehicle_body_h );
                const auto z_rear = static_cast<float>( z_base + vehicle_cabin_h );
                const auto p0 = transform.apply( f_top, r_l );
                const auto p1 = transform.apply( f_top, r_r );
                const auto p2 = transform.apply( f_bot, r_r_roof );
                const auto p3 = transform.apply( f_bot, r_l_roof );
                const auto c = corners_t{{
                    { p0.x, p0.y, z_front }, { p1.x, p1.y, z_front },
                    { p2.x, p2.y, z_rear }, { p3.x, p3.y, z_rear }
                }};
                sink.add_colored( { .sprite = ms, .corners = c, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );

                // Triangular side cheeks connecting sloped glass to 0.50f waistline
                if( side_spr && side_spr->valid ) {
                    const auto &s = *side_spr;
                    const auto solid_uv = make_solid_uvs( s );
                    if( h_left <= vehicle_body_h ) {
                        const auto shade = compute_side_shade( 0.0f, -1.0f, cos_th, sin_th );
                        const auto cheek_col = no_pull( modulate_light( cell.light, shade ) );
                        const auto p3_waist = transform.apply( f_bot, r_l );
                        const auto cheek_c = corners_t{{
                            { p3.x, p3.y, z_rear }, { p0.x, p0.y, z_front },
                            { p0.x, p0.y, z_front }, { p3_waist.x, p3_waist.y, z_front }
                        }};
                        sink.add_colored( { .sprite = s, .corners = cheek_c, .uvs = solid_uv, .ref = light_ref{}, .color = cheek_col } );
                    }
                    if( h_right <= vehicle_body_h ) {
                        const auto shade = compute_side_shade( 0.0f, 1.0f, cos_th, sin_th );
                        const auto cheek_col = no_pull( modulate_light( cell.light, shade ) );
                        const auto p2_waist = transform.apply( f_bot, r_r );
                        const auto cheek_c = corners_t{{
                            { p1.x, p1.y, z_front }, { p2.x, p2.y, z_rear },
                            { p2_waist.x, p2_waist.y, z_front }, { p1.x, p1.y, z_front }
                        }};
                        sink.add_colored( { .sprite = s, .corners = cheek_c, .uvs = solid_uv, .ref = light_ref{}, .color = cheek_col } );
                    }
                }
            } else if( is_rear_windshield && main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                // Sloped rear windshield: vehicle_cabin_h at front down to vehicle_body_h at rear
                const auto &ms = main_sprite_opt->get().main();
                const auto z_front = static_cast<float>( z_base + vehicle_cabin_h );
                const auto z_rear = static_cast<float>( z_base + vehicle_body_h );
                const auto p0 = transform.apply( f_top, r_l_roof );
                const auto p1 = transform.apply( f_top, r_r_roof );
                const auto p2 = transform.apply( f_bot, r_r );
                const auto p3 = transform.apply( f_bot, r_l );
                const auto c = corners_t{{
                    { p0.x, p0.y, z_front }, { p1.x, p1.y, z_front },
                    { p2.x, p2.y, z_rear }, { p3.x, p3.y, z_rear }
                }};
                sink.add_colored( { .sprite = ms, .corners = c, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );

                // Triangular side cheeks
                if( side_spr && side_spr->valid ) {
                    const auto &s = *side_spr;
                    const auto solid_uv = make_solid_uvs( s );
                    if( h_left <= vehicle_body_h ) {
                        const auto shade = compute_side_shade( 0.0f, -1.0f, cos_th, sin_th );
                        const auto cheek_col = no_pull( modulate_light( cell.light, shade ) );
                        const auto p0_waist = transform.apply( f_top, r_l );
                        const auto cheek_c = corners_t{{
                            { p0.x, p0.y, z_front }, { p3.x, p3.y, z_rear },
                            { p3.x, p3.y, z_rear }, { p0_waist.x, p0_waist.y, z_rear }
                        }};
                        sink.add_colored( { .sprite = s, .corners = cheek_c, .uvs = solid_uv, .ref = light_ref{}, .color = cheek_col } );
                    }
                    if( h_right <= vehicle_body_h ) {
                        const auto shade = compute_side_shade( 0.0f, 1.0f, cos_th, sin_th );
                        const auto cheek_col = no_pull( modulate_light( cell.light, shade ) );
                        const auto p1_waist = transform.apply( f_top, r_r );
                        const auto cheek_c = corners_t{{
                            { p2.x, p2.y, z_rear }, { p1.x, p1.y, z_front },
                            { p1_waist.x, p1_waist.y, z_rear }, { p2.x, p2.y, z_rear }
                        }};
                        sink.add_colored( { .sprite = s, .corners = cheek_c, .uvs = solid_uv, .ref = light_ref{}, .color = cheek_col } );
                    }
                }
            } else if( is_front_bumper && main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                // Front bumper tile: flat section + beveled nose to vehicle_bumper_h with outer corner chamfer
                const auto &ms = main_sprite_opt->get().main();
                const auto f_rear_bumper = f_bot;
                const auto f_front_bumper = f_top;
                const auto f_bevel = f_top - ( f_top - f_bot ) * 0.30f;
                const auto is_chamfer_r = ( cell.mount.y() == max_y_front );
                const auto is_chamfer_l = ( cell.mount.y() == min_y_front );
                const auto front_r_x = is_chamfer_r ? ( f_front_bumper - 0.10f ) : f_front_bumper;
                const auto front_r_y = is_chamfer_r ? ( r_r - 0.10f ) : r_r;
                const auto front_l_x = is_chamfer_l ? ( f_front_bumper - 0.10f ) : f_front_bumper;
                const auto front_l_y = is_chamfer_l ? ( r_l + 0.10f ) : r_l;

                // Flat rear section
                const auto p0 = transform.apply( f_bevel, r_l );
                const auto p1 = transform.apply( f_bevel, r_r );
                const auto p2 = transform.apply( f_rear_bumper, r_r );
                const auto p3 = transform.apply( f_rear_bumper, r_l );
                const auto c_flat = corners_t{{
                    { p0.x, p0.y, z_base + vehicle_body_h }, { p1.x, p1.y, z_base + vehicle_body_h },
                    { p2.x, p2.y, z_base + vehicle_body_h }, { p3.x, p3.y, z_base + vehicle_body_h }
                }};
                sink.add_colored( { .sprite = ms, .corners = c_flat, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );

                // Beveled front nose down to vehicle_bumper_h
                const auto p_front_l = transform.apply( front_l_x, front_l_y );
                const auto p_front_r = transform.apply( front_r_x, front_r_y );
                const auto c_bevel = corners_t{{
                    { p_front_l.x, p_front_l.y, z_base + vehicle_bumper_h }, { p_front_r.x, p_front_r.y, z_base + vehicle_bumper_h },
                    { p1.x, p1.y, z_base + vehicle_body_h }, { p0.x, p0.y, z_base + vehicle_body_h }
                }};
                sink.add_colored( { .sprite = ms, .corners = c_bevel, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );
            } else if( is_rear_bumper && main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                // Rear bumper tile: flat section + beveled tail to vehicle_bumper_h with outer corner chamfer
                const auto &ms = main_sprite_opt->get().main();
                const auto f_front_bumper = f_top;
                const auto f_rear_bumper = f_bot;
                const auto f_bevel = f_bot + ( f_top - f_bot ) * 0.30f;
                const auto is_chamfer_r = ( cell.mount.y() == max_y_rear );
                const auto is_chamfer_l = ( cell.mount.y() == min_y_rear );
                const auto rear_r_x = is_chamfer_r ? ( f_rear_bumper + 0.10f ) : f_rear_bumper;
                const auto rear_r_y = is_chamfer_r ? ( r_r - 0.10f ) : r_r;
                const auto rear_l_x = is_chamfer_l ? ( f_rear_bumper + 0.10f ) : f_rear_bumper;
                const auto rear_l_y = is_chamfer_l ? ( r_l + 0.10f ) : r_l;

                // Flat front section
                const auto p0 = transform.apply( f_front_bumper, r_l );
                const auto p1 = transform.apply( f_front_bumper, r_r );
                const auto p2 = transform.apply( f_bevel, r_r );
                const auto p3 = transform.apply( f_bevel, r_l );
                const auto c_flat = corners_t{{
                    { p0.x, p0.y, z_base + vehicle_body_h }, { p1.x, p1.y, z_base + vehicle_body_h },
                    { p2.x, p2.y, z_base + vehicle_body_h }, { p3.x, p3.y, z_base + vehicle_body_h }
                }};
                sink.add_colored( { .sprite = ms, .corners = c_flat, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );

                // Beveled rear tail down to vehicle_bumper_h
                const auto p_rear_l = transform.apply( rear_l_x, rear_l_y );
                const auto p_rear_r = transform.apply( rear_r_x, rear_r_y );
                const auto c_bevel = corners_t{{
                    { p3.x, p3.y, z_base + vehicle_body_h }, { p2.x, p2.y, z_base + vehicle_body_h },
                    { p_rear_r.x, p_rear_r.y, z_base + vehicle_bumper_h }, { p_rear_l.x, p_rear_l.y, z_base + vehicle_bumper_h }
                }};
                sink.add_colored( { .sprite = ms, .corners = c_bevel, .uvs = make_sprite_uvs( ms ), .ref = light_ref{}, .color = no_pull( cell.light ) } );
            } else if( cell.show_roof && cell.has_roof ) {
                // 1a. Emit interior floor and parts under the roof
                if( main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                    if( side_spr && side_spr->valid ) {
                        const auto p0 = transform.apply( f_top, r_l );
                        const auto p1 = transform.apply( f_top, r_r );
                        const auto p2 = transform.apply( f_bot, r_r );
                        const auto p3 = transform.apply( f_bot, r_l );
                        const auto z_floor = static_cast<float>( z_base + chassis_floor_h );
                        const auto c_floor = corners_t{{
                            { p0.x, p0.y, z_floor }, { p1.x, p1.y, z_floor },
                            { p2.x, p2.y, z_floor }, { p3.x, p3.y, z_floor }
                        }};
                        sink.add_colored( { .sprite = *side_spr, .corners = c_floor, .uvs = make_solid_uvs( *side_spr ),
                                            .ref = light_ref{}, .color = no_pull( modulate_light( cell.light, 0.70f ) ) } );
                    }
                    emit_vehicle_tile_quads( { .sink = sink, .info = main_sprite_opt->get(), .rel_forward = rel_forward,
                                               .rel_right = rel_right, .z = z_base + chassis_floor_h + 0.002f, .color = cell.light,
                                               .transform = transform, .f_min = f_bot_rel, .f_max = f_top_rel,
                                               .r_min = r_l_rel, .r_max = r_r_rel } );
                }

                // 1b. Emit solid watertight roof cap at vehicle_cabin_h (0.82f) with tumblehome
                const auto p0 = transform.apply( f_top_roof, r_l_roof );
                const auto p1 = transform.apply( f_top_roof, r_r_roof );
                const auto p2 = transform.apply( f_bot_roof, r_r_roof );
                const auto p3 = transform.apply( f_bot_roof, r_l_roof );
                const auto z_roof = static_cast<float>( z_base + vehicle_cabin_h );
                const auto c_roof = corners_t{{
                    { p0.x, p0.y, z_roof }, { p1.x, p1.y, z_roof },
                    { p2.x, p2.y, z_roof }, { p3.x, p3.y, z_roof }
                }};

                if( side_spr && side_spr->valid ) {
                    sink.add_colored( { .sprite = *side_spr, .corners = c_roof, .uvs = make_solid_uvs( *side_spr ),
                                        .ref = light_ref{}, .color = no_pull( modulate_light( cell.light, 0.95f ) ) } );
                }

                // On-roof accessories (solar panels, turrets, etc.)
                if( cell.roof_part_idx >= 0 && roof_sprite_opt.has_value() && roof_sprite_opt->get().main().valid ) {
                    const auto &rs = roof_sprite_opt->get().main();
                    const auto z_acc = static_cast<float>( z_roof + 0.003f );
                    const auto c_acc = corners_t{{
                        { p0.x, p0.y, z_acc }, { p1.x, p1.y, z_acc },
                        { p2.x, p2.y, z_acc }, { p3.x, p3.y, z_acc }
                    }};
                    sink.add_colored( { .sprite = rs, .corners = c_acc, .uvs = make_sprite_uvs( rs ), .ref = light_ref{}, .color = no_pull( cell.light ) } );
                }
            } else if( main_sprite_opt.has_value() && main_sprite_opt->get().main().valid ) {
                if( cell.is_interior && side_spr && side_spr->valid ) {
                    const auto p0 = transform.apply( f_top, r_l );
                    const auto p1 = transform.apply( f_top, r_r );
                    const auto p2 = transform.apply( f_bot, r_r );
                    const auto p3 = transform.apply( f_bot, r_l );
                    const auto z_floor = static_cast<float>( z_base + cell.height );
                    const auto c_floor = corners_t{{
                        { p0.x, p0.y, z_floor }, { p1.x, p1.y, z_floor },
                        { p2.x, p2.y, z_floor }, { p3.x, p3.y, z_floor }
                    }};
                    sink.add_colored( { .sprite = *side_spr, .corners = c_floor, .uvs = make_solid_uvs( *side_spr ),
                                        .ref = light_ref{}, .color = no_pull( modulate_light( cell.light, 0.70f ) ) } );
                }
                emit_vehicle_tile_quads( { .sink = sink, .info = main_sprite_opt->get(), .rel_forward = rel_forward,
                                           .rel_right = rel_right, .z = z_base + cell.height + 0.002f, .color = cell.light,
                                           .transform = transform, .f_min = f_bot_rel, .f_max = f_top_rel,
                                           .r_min = r_l_rel, .r_max = r_r_rel } );
            }

            // 2. Vertical Side Walls with Directional Shading & Two-Band Cabin Sides
            if( side_spr && side_spr->valid ) {
                const auto &s = *side_spr;
                const auto solid_uv = make_solid_uvs( s );

                auto emit_side_wall = [ & ]( float local_f, float local_r, float h_neighbor,
                                             float f_center, float r_center, bool is_lateral,
                                             const point_f &p_top_l, const point_f &p_top_r,
                                             float tumble_val ) {
                    if( cell.height <= h_neighbor ) {
                        return;
                    }
                    const auto side_shade = compute_side_shade( local_f, local_r, cos_th, sin_th );
                    const auto side_color = no_pull( modulate_light( cell.light, side_shade ) );
                    const auto is_exterior = ( h_neighbor == 0.0f );

                    // Recessed dark undercarriage skirt on exterior edges
                    if( is_exterior ) {
                        const auto skirt_shade_col = no_pull( modulate_light( pack_light( 35, 38, 42 ), side_shade ) );
                        const auto z_skirt_bot = static_cast<float>( z_base + 0.06f );
                        const auto z_skirt_top = static_cast<float>( z_base + vehicle_clearance_h );
                        point_f sp0;
                        point_f sp1;
                        if( is_lateral ) {
                            const auto r_in = r_center - local_r * 0.08f;
                            sp0 = transform.apply( f_bot, r_in );
                            sp1 = transform.apply( f_top, r_in );
                            if( local_r < 0.0f ) {
                                std::swap( sp0, sp1 );
                            }
                        } else {
                            const auto f_in = f_center - local_f * 0.08f;
                            sp0 = transform.apply( f_in, r_r );
                            sp1 = transform.apply( f_in, r_l );
                            if( local_f < 0.0f ) {
                                std::swap( sp0, sp1 );
                            }
                        }
                        const auto skirt_c = corners_t{{
                            { sp0.x, sp0.y, z_skirt_top }, { sp1.x, sp1.y, z_skirt_top },
                            { sp1.x, sp1.y, z_skirt_bot }, { sp0.x, sp0.y, z_skirt_bot }
                        }};
                        sink.add_colored( { .sprite = s, .corners = skirt_c, .uvs = solid_uv, .ref = light_ref{}, .color = skirt_shade_col } );
                    }

                    auto z_bot = static_cast<float>( z_base + ( is_exterior ? vehicle_clearance_h : h_neighbor ) );
                    auto z_top = static_cast<float>( z_base + cell.height );
                    if( is_exterior && local_f > 0.0f && is_front_bumper ) {
                        z_top = static_cast<float>( z_base + vehicle_bumper_h );
                    } else if( is_exterior && local_f < 0.0f && is_rear_bumper ) {
                        z_top = static_cast<float>( z_base + vehicle_bumper_h );
                    }

                    // Check if upper cabin side should split into pillars + recessed glass
                    if( cell.height > vehicle_body_h && !cell.is_opaque_wall && z_top > z_base + vehicle_body_h ) {
                        const auto z_mid = static_cast<float>( z_base + vehicle_body_h );

                        // Lower Band (body waistline)
                        if( z_mid > z_bot ) {
                            const auto c_lower = corners_t{{
                                { p_top_l.x, p_top_l.y, z_mid }, { p_top_r.x, p_top_r.y, z_mid },
                                { p_top_r.x, p_top_r.y, z_bot }, { p_top_l.x, p_top_l.y, z_bot }
                            }};
                            sink.add_colored( { .sprite = s, .corners = c_lower, .uvs = solid_uv, .ref = light_ref{}, .color = side_color } );
                        }

                        // Upper Band: two pillars + window pane recessed 0.03f with tumblehome
                        const auto glass_col = no_pull( modulate_light( pack_light( 45, 55, 65 ), side_shade ) );
                        if( is_lateral ) {
                            const auto r_outer = r_center;
                            const auto r_roof_edge = r_center - local_r * tumble_val;
                            const auto r_pane_bot = r_outer - local_r * 0.03f;
                            const auto r_pane_top = r_roof_edge - local_r * 0.03f;

                            const auto f_pil_front_bot = f_top - 0.12f;
                            const auto f_pil_front_top = f_top_roof - 0.12f;
                            const auto f_pil_rear_bot = f_bot + 0.12f;
                            const auto f_pil_rear_top = f_bot_roof + 0.12f;

                            // Pillar 1 (near f_top)
                            point_f pil1_p0_top, pil1_p1_top, pil1_p1_mid, pil1_p0_mid;
                            if( local_r > 0.0f ) {
                                pil1_p0_top = transform.apply( f_pil_front_top, r_roof_edge );
                                pil1_p1_top = transform.apply( f_top_roof, r_roof_edge );
                                pil1_p1_mid = transform.apply( f_top, r_outer );
                                pil1_p0_mid = transform.apply( f_pil_front_bot, r_outer );
                            } else {
                                pil1_p0_top = transform.apply( f_top_roof, r_roof_edge );
                                pil1_p1_top = transform.apply( f_pil_front_top, r_roof_edge );
                                pil1_p1_mid = transform.apply( f_pil_front_bot, r_outer );
                                pil1_p0_mid = transform.apply( f_top, r_outer );
                            }
                            const auto c_pil1 = corners_t{{
                                { pil1_p0_top.x, pil1_p0_top.y, z_top }, { pil1_p1_top.x, pil1_p1_top.y, z_top },
                                { pil1_p1_mid.x, pil1_p1_mid.y, z_mid }, { pil1_p0_mid.x, pil1_p0_mid.y, z_mid }
                            }};
                            sink.add_colored( { .sprite = s, .corners = c_pil1, .uvs = solid_uv, .ref = light_ref{}, .color = side_color } );

                            // Pillar 2 (near f_bot)
                            point_f pil2_p0_top, pil2_p1_top, pil2_p1_mid, pil2_p0_mid;
                            if( local_r > 0.0f ) {
                                pil2_p0_top = transform.apply( f_bot_roof, r_roof_edge );
                                pil2_p1_top = transform.apply( f_pil_rear_top, r_roof_edge );
                                pil2_p1_mid = transform.apply( f_pil_rear_bot, r_outer );
                                pil2_p0_mid = transform.apply( f_bot, r_outer );
                            } else {
                                pil2_p0_top = transform.apply( f_pil_rear_top, r_roof_edge );
                                pil2_p1_top = transform.apply( f_bot_roof, r_roof_edge );
                                pil2_p1_mid = transform.apply( f_bot, r_outer );
                                pil2_p0_mid = transform.apply( f_pil_rear_bot, r_outer );
                            }
                            const auto c_pil2 = corners_t{{
                                { pil2_p0_top.x, pil2_p0_top.y, z_top }, { pil2_p1_top.x, pil2_p1_top.y, z_top },
                                { pil2_p1_mid.x, pil2_p1_mid.y, z_mid }, { pil2_p0_mid.x, pil2_p0_mid.y, z_mid }
                            }};
                            sink.add_colored( { .sprite = s, .corners = c_pil2, .uvs = solid_uv, .ref = light_ref{}, .color = side_color } );

                            // Window Pane (recessed)
                            point_f pane_p0_top, pane_p1_top, pane_p1_mid, pane_p0_mid;
                            if( local_r > 0.0f ) {
                                pane_p0_top = transform.apply( f_pil_rear_top, r_pane_top );
                                pane_p1_top = transform.apply( f_pil_front_top, r_pane_top );
                                pane_p1_mid = transform.apply( f_pil_front_bot, r_pane_bot );
                                pane_p0_mid = transform.apply( f_pil_rear_bot, r_pane_bot );
                            } else {
                                pane_p0_top = transform.apply( f_pil_front_top, r_pane_top );
                                pane_p1_top = transform.apply( f_pil_rear_top, r_pane_top );
                                pane_p1_mid = transform.apply( f_pil_rear_bot, r_pane_bot );
                                pane_p0_mid = transform.apply( f_pil_front_bot, r_pane_bot );
                            }
                            const auto c_pane = corners_t{{
                                { pane_p0_top.x, pane_p0_top.y, z_top }, { pane_p1_top.x, pane_p1_top.y, z_top },
                                { pane_p1_mid.x, pane_p1_mid.y, z_mid }, { pane_p0_mid.x, pane_p0_mid.y, z_mid }
                            }};
                            sink.add_colored( { .sprite = s, .corners = c_pane, .uvs = solid_uv, .ref = light_ref{}, .color = glass_col } );
                        } else {
                            // Front/rear upper wall
                            const auto f_outer = f_center;
                            const auto f_roof_edge = f_center - local_f * tumble_val;
                            const auto p0_top = ( local_f > 0.0f ) ? transform.apply( f_roof_edge, r_l_roof ) : transform.apply( f_roof_edge, r_r_roof );
                            const auto p1_top = ( local_f > 0.0f ) ? transform.apply( f_roof_edge, r_r_roof ) : transform.apply( f_roof_edge, r_l_roof );
                            const auto p0_mid = ( local_f > 0.0f ) ? transform.apply( f_outer, r_l ) : transform.apply( f_outer, r_r );
                            const auto p1_mid = ( local_f > 0.0f ) ? transform.apply( f_outer, r_r ) : transform.apply( f_outer, r_l );
                            const auto c_upper = corners_t{{
                                { p0_top.x, p0_top.y, z_top }, { p1_top.x, p1_top.y, z_top },
                                { p1_mid.x, p1_mid.y, z_mid }, { p0_mid.x, p0_mid.y, z_mid }
                            }};
                            sink.add_colored( { .sprite = s, .corners = c_upper, .uvs = solid_uv, .ref = light_ref{}, .color = side_color } );
                        }
                    } else {
                        // Standard solid wall
                        const auto c = corners_t{{
                            { p_top_l.x, p_top_l.y, z_top }, { p_top_r.x, p_top_r.y, z_top },
                            { p_top_r.x, p_top_r.y, z_bot }, { p_top_l.x, p_top_l.y, z_bot }
                        }};
                        sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{}, .color = side_color } );
                    }
                };

                // 1. Front: (+1, 0)
                {
                    const auto p0 = transform.apply( f_top, r_r );
                    const auto p1 = transform.apply( f_top, r_l );
                    emit_side_wall( 1.0f, 0.0f, h_fwd, f_top, rel_right, false, p0, p1, tumble_f );
                }
                // 2. Back: (-1, 0)
                {
                    const auto p0 = transform.apply( f_bot, r_l );
                    const auto p1 = transform.apply( f_bot, r_r );
                    emit_side_wall( -1.0f, 0.0f, h_rear, f_bot, rel_right, false, p0, p1, tumble_b );
                }
                // 3. Right: (0, +1)
                {
                    const auto p0 = transform.apply( f_bot, r_r );
                    const auto p1 = transform.apply( f_top, r_r );
                    emit_side_wall( 0.0f, 1.0f, h_right, rel_forward, r_r, true, p0, p1, tumble_r );
                }
                // 4. Left: (0, -1)
                {
                    const auto p0 = transform.apply( f_top, r_l );
                    const auto p1 = transform.apply( f_bot, r_l );
                    emit_side_wall( 0.0f, -1.0f, h_left, rel_forward, r_l, true, p0, p1, tumble_l );
                }
            }

            // 3. Distinct 3D Wheels
            if( cell.has_wheel && side_spr && side_spr->valid ) {
                const auto &s = *side_spr;
                const auto solid_uv = make_solid_uvs( s );
                const auto f_center = ( f_top + f_bot ) * 0.5f;
                const auto f_min = static_cast<float>( f_center - 0.375f );
                const auto f_max = static_cast<float>( f_center + 0.375f );
                const auto z_wheel_bot = static_cast<float>( z_base + 0.0f );
                const auto z_wheel_top = static_cast<float>( z_base + vehicle_wheel_h );
                constexpr auto wheel_w = 0.26f;

                auto r_min = 0.0f;
                auto r_max = 0.0f;
                auto outer_side = 0; // +1 for right, -1 for left, 0 for centerline
                if( cell.mount.y() > 0 ) {
                    r_max = r_r;
                    r_min = r_max - wheel_w;
                    outer_side = 1;
                } else if( cell.mount.y() < 0 ) {
                    r_min = r_l;
                    r_max = r_min + wheel_w;
                    outer_side = -1;
                } else {
                    const auto has_r = cells.find( point( cell.mount.x(), cell.mount.y() + 1 ) ) != cells.end();
                    const auto has_l = cells.find( point( cell.mount.x(), cell.mount.y() - 1 ) ) != cells.end();
                    if( has_r && !has_l ) {
                        r_min = r_l;
                        r_max = r_min + wheel_w;
                        outer_side = -1;
                    } else if( has_l && !has_r ) {
                        r_max = r_r;
                        r_min = r_max - wheel_w;
                        outer_side = 1;
                    } else {
                        r_min = rel_right - 0.13f;
                        r_max = rel_right + 0.13f;
                        outer_side = 0;
                    }
                }
                const auto rubber_color = pack_light( 30, 30, 34 );

                // Top face
                {
                    const auto p0 = transform.apply( f_max, r_min );
                    const auto p1 = transform.apply( f_max, r_max );
                    const auto p2 = transform.apply( f_min, r_max );
                    const auto p3 = transform.apply( f_min, r_min );
                    const auto c = corners_t{{
                        { p0.x, p0.y, z_wheel_top }, { p1.x, p1.y, z_wheel_top },
                        { p2.x, p2.y, z_wheel_top }, { p3.x, p3.y, z_wheel_top }
                    }};
                    sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{},
                                        .color = no_pull( modulate_light( rubber_color, 1.0f ) ) } );
                }
                // Front face
                {
                    const auto shade = compute_side_shade( 1.0f, 0.0f, cos_th, sin_th );
                    const auto col = no_pull( modulate_light( rubber_color, shade ) );
                    const auto p0 = transform.apply( f_max, r_max );
                    const auto p1 = transform.apply( f_max, r_min );
                    const auto c = corners_t{{
                        { p0.x, p0.y, z_wheel_top }, { p1.x, p1.y, z_wheel_top },
                        { p1.x, p1.y, z_wheel_bot }, { p0.x, p0.y, z_wheel_bot }
                    }};
                    sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{}, .color = col } );
                }
                // Back face
                {
                    const auto shade = compute_side_shade( -1.0f, 0.0f, cos_th, sin_th );
                    const auto col = no_pull( modulate_light( rubber_color, shade ) );
                    const auto p0 = transform.apply( f_min, r_min );
                    const auto p1 = transform.apply( f_min, r_max );
                    const auto c = corners_t{{
                        { p0.x, p0.y, z_wheel_top }, { p1.x, p1.y, z_wheel_top },
                        { p1.x, p1.y, z_wheel_bot }, { p0.x, p0.y, z_wheel_bot }
                    }};
                    sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{}, .color = col } );
                }
                // Outer face & Hubcap
                if( outer_side != 0 ) {
                    const auto r_outer = outer_side > 0 ? r_max : r_min;
                    const auto local_r = outer_side > 0 ? 1.0f : -1.0f;
                    const auto shade = compute_side_shade( 0.0f, local_r, cos_th, sin_th );
                    const auto col = no_pull( modulate_light( rubber_color, shade ) );
                    const auto p0 = outer_side > 0 ? transform.apply( f_min, r_outer ) : transform.apply( f_max, r_outer );
                    const auto p1 = outer_side > 0 ? transform.apply( f_max, r_outer ) : transform.apply( f_min, r_outer );
                    const auto c = corners_t{{
                        { p0.x, p0.y, z_wheel_top }, { p1.x, p1.y, z_wheel_top },
                        { p1.x, p1.y, z_wheel_bot }, { p0.x, p0.y, z_wheel_bot }
                    }};
                    sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{}, .color = col } );

                    // Hubcap center strip
                    const auto f_hub_min = static_cast<float>( f_center - 0.18f );
                    const auto f_hub_max = static_cast<float>( f_center + 0.18f );
                    const auto z_hub_bot = static_cast<float>( z_base + 0.08f );
                    const auto z_hub_top = static_cast<float>( z_base + 0.28f );
                    const auto r_hub = static_cast<float>( r_outer + ( outer_side > 0 ? 0.005f : -0.005f ) );
                    const auto hub_col = no_pull( modulate_light( pack_light( 140, 145, 155 ), shade ) );
                    const auto hp0 = outer_side > 0 ? transform.apply( f_hub_min, r_hub ) : transform.apply( f_hub_max, r_hub );
                    const auto hp1 = outer_side > 0 ? transform.apply( f_hub_max, r_hub ) : transform.apply( f_hub_min, r_hub );
                    const auto hc = corners_t{{
                        { hp0.x, hp0.y, z_hub_top }, { hp1.x, hp1.y, z_hub_top },
                        { hp1.x, hp1.y, z_hub_bot }, { hp0.x, hp0.y, z_hub_bot }
                    }};
                    sink.add_colored( { .sprite = s, .corners = hc, .uvs = solid_uv, .ref = light_ref{}, .color = hub_col } );
                } else {
                    for( const auto sign : { 1.0f, -1.0f } ) {
                        const auto r_outer = sign > 0.0f ? r_max : r_min;
                        const auto shade = compute_side_shade( 0.0f, sign, cos_th, sin_th );
                        const auto col = no_pull( modulate_light( rubber_color, shade ) );
                        const auto p0 = sign > 0.0f ? transform.apply( f_min, r_outer ) : transform.apply( f_max, r_outer );
                        const auto p1 = sign > 0.0f ? transform.apply( f_max, r_outer ) : transform.apply( f_min, r_outer );
                        const auto c = corners_t{{
                            { p0.x, p0.y, z_wheel_top }, { p1.x, p1.y, z_wheel_top },
                            { p1.x, p1.y, z_wheel_bot }, { p0.x, p0.y, z_wheel_bot }
                        }};
                        sink.add_colored( { .sprite = s, .corners = c, .uvs = solid_uv, .ref = light_ref{}, .color = col } );
                    }
                }
            }

            // 4. Cargo Highlight
            if( cell.main_part_idx >= 0 ) {
                const auto cargo = vpart_position( veh, cell.main_part_idx ).part_with_feature( "CARGO", true );
                if( cargo && !veh.get_items( cargo->part_index() ).empty() ) {
                    const auto &highlight = view.cache.named( { .category = C_NONE, .id = "highlight", .tiles = view.tiles } );
                    if( highlight.main().valid ) {
                        const auto p0 = transform.apply( f_top, r_l );
                        const auto p1 = transform.apply( f_top, r_r );
                        const auto p2 = transform.apply( f_bot, r_r );
                        const auto p3 = transform.apply( f_bot, r_l );
                        const auto cur_z = static_cast<float>( z_base + cell.height + 0.02f );
                        const auto c = corners_t{{
                            { p0.x, p0.y, cur_z }, { p1.x, p1.y, cur_z }, { p2.x, p2.y, cur_z }, { p3.x, p3.y, cur_z }
                        }};
                        const auto &hl = highlight.main();
                        const auto uv = uvs_t{{ { hl.u0, hl.v0 }, { hl.u1, hl.v0 }, { hl.u1, hl.v1 }, { hl.u0, hl.v1 } }};
                        sink.add_colored( { .sprite = hl, .corners = c, .uvs = uv, .ref = light_ref{}, .color = pack_light( 255, 255, 255 ) } );
                    }
                }
            }
        }
    }
}

} // namespace

auto color_to_packed_light( uint32_t color_argb, uint32_t ambient_light ) -> uint32_t
{
    if( color_argb == 0 ) {
        return ambient_light;
    }
    const auto col_r = static_cast<uint8_t>( ( color_argb >> 16u ) & 0xffu );
    const auto col_g = static_cast<uint8_t>( ( color_argb >> 8u ) & 0xffu );
    const auto col_b = static_cast<uint8_t>( color_argb & 0xffu );

    const auto amb_r = static_cast<uint8_t>( ambient_light & 0xffu );
    const auto amb_g = static_cast<uint8_t>( ( ambient_light >> 8u ) & 0xffu );
    const auto amb_b = static_cast<uint8_t>( ( ambient_light >> 16u ) & 0xffu );

    const auto out_r = static_cast<uint8_t>( ( static_cast<uint32_t>( col_r ) * amb_r ) / 255u );
    const auto out_g = static_cast<uint8_t>( ( static_cast<uint32_t>( col_g ) * amb_g ) / 255u );
    const auto out_b = static_cast<uint8_t>( ( static_cast<uint32_t>( col_b ) * amb_b ) / 255u );

    return pack_light( out_r, out_g, out_b );
}

auto get_sprite_opaque_bounds( const sprite_ref &s ) -> sprite_opaque_bounds
{
    if( !s.valid || ( s.opaque_x0 <= 0.01f && s.opaque_x1 >= 0.99f &&
                      s.opaque_y0 <= 0.01f && s.opaque_y1 >= 0.99f ) ) {
        return sprite_opaque_bounds{};
    }

    auto x0 = s.opaque_x0;
    auto x1 = s.opaque_x1;
    if( s.flip ) {
        const auto temp = 1.0f - x1;
        x1 = 1.0f - x0;
        x0 = temp;
    }
    const auto y0 = s.opaque_y0;
    const auto y1 = s.opaque_y1;

    auto gx_min = 0.0f;
    auto gx_max = 1.0f;
    auto gy_min = 0.0f;
    auto gy_max = 1.0f;

    switch( s.quarter_turns & 3 ) {
        case 0:
            gx_min = x0;
            gx_max = x1;
            gy_min = y0;
            gy_max = y1;
            break;
        case 1:
            gx_min = 1.0f - y1;
            gx_max = 1.0f - y0;
            gy_min = x0;
            gy_max = x1;
            break;
        case 2:
            gx_min = 1.0f - x1;
            gx_max = 1.0f - x0;
            gy_min = 1.0f - y1;
            gy_max = 1.0f - y0;
            break;
        case 3:
            gx_min = y0;
            gx_max = y1;
            gy_min = 1.0f - x1;
            gy_max = 1.0f - x0;
            break;
    }

    return sprite_opaque_bounds{
        .local_r_min = std::clamp( -0.5f + gx_min, -0.5f, -0.10f ),
        .local_r_max = std::clamp( -0.5f + gx_max, 0.10f, 0.5f ),
        .local_f_min = std::clamp( 0.5f - gy_max, -0.5f, -0.10f ),
        .local_f_max = std::clamp( 0.5f - gy_min, 0.10f, 0.5f ),
        .has_trim = true
    };
}

auto build_vehicle_mesh( const entity_view &view ) -> chunk_mesh
{
    auto &here = get_map();
    const auto &you = get_avatar();
    auto sink = quad_sink{};
    add_vehicles( { .sink = sink, .view = view, .here = here, .you = you } );
    return sink.finish();
}

auto build_entity_mesh( const entity_view &view ) -> chunk_mesh
{
    auto &here = get_map();
    const auto &you = get_avatar();
    auto sink = quad_sink{};
    const auto right = vec3( view.cam.ground_right() );
    // Camera vectors in the stretched space the picture is in (see camera::z_scale).
    const auto toward = vec3( normalize( view.cam.eye() - view.cam.stretched_target() ) );
    const auto up = vec3( cross( right, vec3{ .x = 0.0f, .y = 0.0f, .z = 0.0f } - toward ) );
    const auto white = static_cast<uint32_t>( pack_light( 255, 255, 255 ) );

    auto in_range = [ &view ]( const tripoint_bub_ms & p, int radius ) -> bool {
        return p.z() >= view.z_lo && p.z() <= view.z_hi &&
        std::abs( p.x() - view.center.x() ) <= radius &&
        std::abs( p.y() - view.center.y() ) <= radius;
    };
    auto frame_x = [ &view ]( const tripoint_bub_ms & p ) -> float {
        return static_cast<float>( p.x() - view.center.x() ) + 0.5f;
    };
    auto frame_y = [ &view ]( const tripoint_bub_ms & p ) -> float {
        return static_cast<float>( p.y() - view.center.y() ) + 0.5f;
    };
    auto frame_z = [ &view ]( const tripoint_bub_ms & p ) -> float {
        return static_cast<float>( p.z() - view.center.z() );
    };

    // Fields and items change independently of the terrain mesh, including between turns.
    for( auto z = static_cast<int>( view.z_lo ); z <= view.z_hi; ++z ) {
        for( auto dy = -creature_radius; dy <= creature_radius; ++dy ) {
            for( auto dx = -creature_radius; dx <= creature_radius; ++dx ) {
                const auto p = tripoint_bub_ms( view.center.x() + dx, view.center.y() + dy, z );
                if( !here.inbounds( p ) ) {
                    continue;
                }
                const auto field = visible_field_at( here, you, p );
                if( field && field->display_field ) {
                    auto subtile = 0;
                    auto rotation = 0;
                    view.tiles.get_orientation( { .pos = p, .furniture = false, .subtile = subtile,
                                                  .rotation = rotation, .field = true } );
                    const auto &sprite = view.cache.named( { .category = C_FIELD, .id = field->id.id().str(),
                                                           .tiles = view.tiles, .subtile = subtile, .rota = rotation } );
                    add_ground_decal( { .sink = sink, .tile = sprite, .x = frame_x( p ) - 0.5f,
                                        .y = frame_y( p ) - 0.5f, .z = frame_z( p ) + floor_height + 0.015f,
                                        .color = view.light( p ) } );
                }
                // Nothing on tiles the player cannot see into, and nothing inside closed
                // furniture (lockers, cupboards): the flat view shows those once the player
                // stands next to them, but here the furniture is drawn closed, and its
                // contents would sit on its top (or float there when the cutaway removes it).
                if( std::abs( dx ) > item_radius || std::abs( dy ) > item_radius ||
                    ( field && !field->display_items ) || !here.sees_some_items( p, you ) ||
                    here.has_flag( "CONTAINER", p ) ) {
                    continue;
                }
                auto stack = map_stack( here.i_at( p ) );
                if( stack.empty() ) {
                    continue;
                }
                const auto light = static_cast<uint32_t>( view.light( p ) );
                if( light == pack_light( 0, 0, 0 ) ) {
                    continue; // not visible
                }
                const auto id = std::string( ( *stack.begin() )->typeId().str() );
                const tile_info &sprite = view.cache.named( { .category = C_ITEM, .id = id, .tiles = view.tiles } );
                // Keep the item's illustrated profile visible at every camera angle, like
                // creature sprites, rather than projecting it onto the ground.
                emit_billboard( { .sink = sink, .layers = { &sprite }, .right_world = right, .up_world = up, .toward_world = toward, .z_scale = view.cam.z_scale,
                                  .cx = frame_x( p ), .cy = frame_y( p ), .z_base = frame_z( p ) + view.surface( p ) + 0.02f,
                                  .color = light, .mirrored = false, .scale = item_size } );
            }
        }
    }

    // Trees and tall furniture of the visible chunks; unseen ones are drawn like their tile.
    for( const entity_view::standing &u : view.uprights ) {
        if( !u.info ) {
            continue;
        }
        emit_billboard( { .sink = sink, .layers = { u.info }, .right_world = right, .up_world = up, .toward_world = toward, .z_scale = view.cam.z_scale, .cx = frame_x( u.pos ),
                          .cy = frame_y( u.pos ),
                          .z_base = frame_z( u.pos ) + u.z_base, .color = view.light( u.pos ) } );
    }

    // Creatures: the player, NPCs and monsters that the player can see.
    for( Creature &critter : g->all_creatures() ) {
        const auto p = tripoint_bub_ms( critter.bub_pos() );
        if( !in_range( p, creature_radius ) ) {
            continue;
        }
        if( !critter.is_avatar() && !you.sees( critter ) ) {
            continue;
        }
        // Characters are their body plus worn and wielded overlays, like the flat view.
        std::vector<const tile_info *> layers;
        auto mirrored = static_cast<bool>( false );
        if( const monster *m = critter.as_monster() ) {
            layers.push_back( &view.cache.named( { .category = C_MONSTER, .id = m->type->id.str(), .tiles = view.tiles } ) );
        } else if( const Character *ch = critter.as_character() ) {
            const auto looks = cata_tiles::character_layers( view.tiles.get_character_layers( *ch ) );
            mirrored = looks.mirrored;
            for( const std::pair<std::string, TILE_CATEGORY> &id : looks.ids ) {
                layers.push_back( &view.cache.named( { .category = id.second, .id = id.first, .tiles = view.tiles } ) );
            }
        } else {
            continue;
        }
        const auto color = static_cast<uint32_t>( critter.is_avatar() ? white : view.light( p ) );
        // Debug aid: CATA_R3D_DEBUG_LAYERS=1 logs the sprite layers of the player once per run.
        static const auto debug_layers = static_cast<bool>( std::getenv( "CATA_R3D_DEBUG_LAYERS" ) !=
                                         nullptr );
        static auto logged = static_cast<bool>( false );
        if( debug_layers && critter.is_avatar() && !logged ) {
            logged = true;
            const auto looks = cata_tiles::character_layers( view.tiles.get_character_layers(
                                   *critter.as_character() ) );
            for( auto i = size_t( 0 ); i < looks.ids.size() && i < layers.size(); ++i ) {
                const auto &m = layers[i]->main();
                DebugLog( DL::Info, DC::Main ) << "r3d layer " << i << " " << looks.ids[i].first << " valid=" <<
                                               m.valid
                                               << " size=" << m.size_x << "x" << m.size_y << " uv=" << m.u0 << "," << m.v0
                                               << " backdrop=" << layers[i]->backdrop.valid;
            }
        }
        emit_billboard( { .sink = sink, .layers = layers, .right_world = right, .up_world = up, .toward_world = toward, .z_scale = view.cam.z_scale, .cx = frame_x( p ), .cy = frame_y( p ),
                          .z_base = frame_z( p ) + view.surface( p ), .color = color == pack_light( 0, 0, 0 ) ? white : color,
                          .mirrored = mirrored } );
    }
    // Markers last; they sit just above the surface so the depth test keeps them in front of it.
    for( const entity_view::marker &m : view.markers ) {
        if( !m.sprite || m.pos.z() < view.z_lo || m.pos.z() > view.z_hi ) {
            continue;
        }
        const auto x0 = static_cast<float>( m.pos.x() - view.center.x() );
        const auto y0 = static_cast<float>( m.pos.y() - view.center.y() );
        const auto z = static_cast<float>( frame_z( m.pos ) + m.top + 0.03f );
        emit_flat( { .sink = sink, .sprite = *m.sprite, .cx = x0 + 0.5f, .cy = y0 + 0.5f, .z = z, .color = white, .size = 1.0f } );
        if( m.box ) {
            const auto &s = m.sprite->sprite.valid ? m.sprite->sprite : m.sprite->backdrop;
            if( s.valid ) {
                emit_marker_box( { .sink = sink, .s = s, .x0 = x0, .y0 = y0, .z0 = z, .z1 = z + marker_box_height, .color = white } );
            }
        }
    }
    return sink.finish();
}

} // namespace r3d

#endif // defined(TILES)
