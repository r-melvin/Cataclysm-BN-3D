#include "r3d_dda.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace r3d
{

namespace
{

struct box_hit {
    bool hit = false;
    float t = 0.0f;
    hit_face face = hit_face::none;
};

// Ray vs. axis-aligned box using the slab method. A ray that starts inside the box hits at t = 0
// with face none. Only hits with t <= max_t count.
struct intersect_box_options {
    const vec3 &o;
    const vec3 &d;
    const vec3 &lo;
    const vec3 &hi;
    float max_t;
};

auto intersect_box( const intersect_box_options &options ) -> box_hit
{
    const auto &o = options.o;
    const auto &d = options.d;
    const auto &lo = options.lo;
    const auto &hi = options.hi;
    auto max_t = options.max_t;

    auto t_near = static_cast<float>( -std::numeric_limits<float>::infinity() );
    auto t_far = static_cast<float>( std::numeric_limits<float>::infinity() );
    auto face = hit_face( hit_face::none );

    const float origin[3] = { o.x, o.y, o.z };
    const float dir[3] = { d.x, d.y, d.z };
    const float lo_a[3] = { lo.x, lo.y, lo.z };
    const float hi_a[3] = { hi.x, hi.y, hi.z };
    // Entry face when moving in the positive / negative direction along each axis.
    const hit_face enter_pos[3] = { hit_face::west, hit_face::north, hit_face::bottom };
    const hit_face enter_neg[3] = { hit_face::east, hit_face::south, hit_face::top };

    for( auto axis = static_cast<int>( 0 ); axis < 3; ++axis ) {
        if( dir[axis] == 0.0f ) {
            if( origin[axis] < lo_a[axis] || origin[axis] > hi_a[axis] ) {
                return {};
            }
            continue;
        }
        const auto inv = static_cast<float>( 1.0f / dir[axis] );
        auto t0 = static_cast<float>( ( lo_a[axis] - origin[axis] ) * inv );
        auto t1 = static_cast<float>( ( hi_a[axis] - origin[axis] ) * inv );
        auto entered = hit_face( enter_pos[axis] );
        if( t0 > t1 ) {
            std::swap( t0, t1 );
            entered = enter_neg[axis];
        }
        if( t0 > t_near ) {
            t_near = t0;
            face = entered;
        }
        t_far = std::min( t_far, t1 );
        if( t_near > t_far ) {
            return {};
        }
    }
    if( t_far < 0.0f ) {
        return {};
    }
    auto out = box_hit{};
    if( t_near < 0.0f ) {
        out.t = 0.0f;
        out.face = hit_face::none;
    } else {
        out.t = t_near;
        out.face = face;
    }
    out.hit = out.t <= max_t;
    return out;
}

} // namespace

auto raycast_columns( const raycast_columns_options &options ) -> voxel_hit
{
    const auto &origin = options.origin;
    const auto &dir = options.dir;
    auto max_t = options.max_t;
    const auto &height = options.height;

    int cell[3] = { static_cast<int>( std::floor( origin.x ) ),
                    static_cast<int>( std::floor( origin.y ) ),
                    static_cast<int>( std::floor( origin.z ) )
                  };
    const float o[3] = { origin.x, origin.y, origin.z };
    const float d[3] = { dir.x, dir.y, dir.z };
    int step[3];
    float t_max[3];
    float t_delta[3];
    for( auto axis = static_cast<int>( 0 ); axis < 3; ++axis ) {
        if( d[axis] > 0.0f ) {
            step[axis] = 1;
            t_delta[axis] = 1.0f / d[axis];
            t_max[axis] = ( static_cast<float>( cell[axis] + 1 ) - o[axis] ) * t_delta[axis];
        } else if( d[axis] < 0.0f ) {
            step[axis] = -1;
            t_delta[axis] = -1.0f / d[axis];
            t_max[axis] = ( o[axis] - static_cast<float>( cell[axis] ) ) * t_delta[axis];
        } else {
            step[axis] = 0;
            t_delta[axis] = std::numeric_limits<float>::infinity();
            t_max[axis] = std::numeric_limits<float>::infinity();
        }
    }

    // The cell the ray starts in is visited first; afterwards we step across the nearest boundary.
    auto t_enter = static_cast<float>( 0.0f );
    while( t_enter <= max_t ) {
        const auto h = static_cast<float>( height( cell[0], cell[1], cell[2] ) );
        if( h > 0.0f ) {
            const auto lo = vec3{ .x = static_cast<float>( cell[0] ), .y = static_cast<float>( cell[1] ),
                                  .z = static_cast<float>( cell[2] ) };
            const auto hi = vec3{ .x = lo.x + 1.0f, .y = lo.y + 1.0f, .z = lo.z + std::min( h, 1.0f ) };
            const auto bh = box_hit( intersect_box( { .o = origin, .d = dir, .lo = lo, .hi = hi, .max_t = max_t } ) );
            if( bh.hit ) {
                return { .hit = true, .x = cell[0], .y = cell[1], .z = cell[2], .t = bh.t, .face = bh.face };
            }
        }
        auto axis = static_cast<int>( 0 );
        if( t_max[1] < t_max[axis] ) {
            axis = 1;
        }
        if( t_max[2] < t_max[axis] ) {
            axis = 2;
        }
        if( step[axis] == 0 || std::isinf( t_max[axis] ) ) {
            break;
        }
        t_enter = t_max[axis];
        cell[axis] += step[axis];
        t_max[axis] += t_delta[axis];
    }
    return {};
}

auto raycast_columns_reference( const raycast_columns_reference_options &options ) -> voxel_hit
{
    const auto &origin = options.origin;
    const auto &dir = options.dir;
    auto max_t = options.max_t;
    const auto &height = options.height;
    auto lo_x = options.lo_x;
    auto lo_y = options.lo_y;
    auto lo_z = options.lo_z;
    auto hi_x = options.hi_x;
    auto hi_y = options.hi_y;
    auto hi_z = options.hi_z;

    auto best = voxel_hit{};
    for( auto x = static_cast<int>( lo_x ); x <= hi_x; ++x ) {
        for( auto y = static_cast<int>( lo_y ); y <= hi_y; ++y ) {
            for( auto z = static_cast<int>( lo_z ); z <= hi_z; ++z ) {
                const auto h = static_cast<float>( height( x, y, z ) );
                if( h <= 0.0f ) {
                    continue;
                }
                const auto lo = vec3{ .x = static_cast<float>( x ), .y = static_cast<float>( y ), .z = static_cast<float>( z ) };
                const auto hi = vec3{ .x = lo.x + 1.0f, .y = lo.y + 1.0f, .z = lo.z + std::min( h, 1.0f ) };
                const auto bh = box_hit( intersect_box( { .o = origin, .d = dir, .lo = lo, .hi = hi, .max_t = max_t } ) );
                if( bh.hit && ( !best.hit || bh.t < best.t ) ) {
                    best = { .hit = true, .x = x, .y = y, .z = z, .t = bh.t, .face = bh.face };
                }
            }
        }
    }
    return best;
}

auto clear_line( const vec3 &from, const vec3 &to, const column_height_fn &height ) -> bool
{
    const auto d = vec3( to - from );
    const auto dist = static_cast<float>( length( d ) );
    if( dist < 1e-4f ) {
        return true;
    }
    const auto hit = voxel_hit( raycast_columns( { .origin = from, .dir = d * ( 1.0f / dist ), .max_t = dist, .height = height } ) );
    if( !hit.hit ) {
        return true;
    }
    // Reaching the target's own column (the furniture it stands on) is not being blocked.
    return hit.x == static_cast<int>( std::floor( to.x ) ) &&
           hit.y == static_cast<int>( std::floor( to.y ) ) &&
           hit.z == static_cast<int>( std::floor( to.z ) );
}

auto pick_cell( const pick_cell_options &options ) -> voxel_hit
{
    const auto &cam = options.cam;
    auto ndc_x = options.ndc_x;
    auto ndc_y = options.ndc_y;
    auto aspect = options.aspect;
    auto max_t = options.max_t;
    const auto &height = options.height;
    auto floor_z = options.floor_z;

    const auto world = ray( cam.screen_ray( ndc_x, ndc_y, aspect ) );
    const auto origin = vec3( world_to_tile( world.origin ) );
    const auto dir = vec3( world_to_tile( world.dir ) );
    const auto hit = voxel_hit( raycast_columns( { .origin = origin, .dir = dir, .max_t = max_t, .height = height } ) );
    if( hit.hit || dir.z >= 0.0f ) {
        return hit;
    }
    const auto t = static_cast<float>( ( floor_z - origin.z ) / dir.z );
    if( t < 0.0f ) {
        return {};
    }
    const auto p = vec3( origin + dir * t );
    auto plane = voxel_hit{};
    plane.hit = true;
    plane.x = static_cast<int>( std::floor( p.x ) );
    plane.y = static_cast<int>( std::floor( p.y ) );
    plane.z = static_cast<int>( std::floor( floor_z ) );
    plane.t = t;
    plane.face = hit_face::top;
    return plane;
}

} // namespace r3d
