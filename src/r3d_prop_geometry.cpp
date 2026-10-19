#include "r3d_prop_geometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#if defined(TILES)
#include "r3d_mesh.h"
#endif

namespace r3d
{
auto prop_cuboids( const solid_prop kind ) -> std::vector<prop_cuboid>
{
    if( kind == solid_prop::overturned_bench ) {
        auto boxes = std::vector<prop_cuboid> {
            { .center = { 0.5f, 0.61f, 0.325f }, .size = { 0.85f, 0.06f, 0.65f } }
        };
        for( const auto x : { 0.18f, 0.82f } ) {
            for( const auto z : { 0.08f, 0.57f } ) {
                boxes.push_back( { .center = { x, 0.45f, z }, .size = { 0.09f, 0.32f, 0.08f } } );
            }
        }
        return boxes;
    }
    if( kind == solid_prop::metal_wreckage ) {
        return {
            { .center = { 0.50f, 0.50f, 0.08f }, .size = { 0.70f, 0.50f, 0.12f }, .yaw = 0.15f },
            { .center = { 0.48f, 0.40f, 0.20f }, .size = { 0.80f, 0.09f, 0.08f }, .yaw = 0.45f, .pitch = 0.24f },
            { .center = { 0.50f, 0.55f, 0.24f }, .size = { 0.74f, 0.10f, 0.08f }, .yaw = -0.65f, .pitch = -0.28f },
            { .center = { 0.42f, 0.54f, 0.18f }, .size = { 0.58f, 0.16f, 0.08f }, .yaw = 1.3f, .pitch = 0.20f },
            { .center = { 0.60f, 0.50f, 0.28f }, .size = { 0.43f, 0.12f, 0.07f }, .yaw = 0.8f, .pitch = -0.32f }
        };
    }
    return {};
}

auto stair_cuboids( const stair_flight_options &options ) -> std::vector<prop_cuboid>
{
    auto boxes = std::vector<prop_cuboid> {};
    constexpr auto treads = 4;
    const auto count = std::max( options.segments, 1 ) * treads;
    const auto segment = std::clamp( options.segment, 0, std::max( options.segments - 1, 0 ) );
    for( auto step = 0; step < treads; ++step ) {
        const auto progress = static_cast<float>( segment * treads + step ) / count;
        const auto top = options.down ? -progress : progress + 1.0f / count;
        const auto bottom = options.down ? -1.0f : 0.0f;
        const auto along = ( static_cast<float>( step ) + 0.5f ) / treads;
        boxes.push_back( {
            .center = { options.along_y ? 0.5f : along, options.along_y ? along : 0.5f, ( top + bottom ) * 0.5f },
            .size = { options.along_y ? 1.0f : 1.0f / treads, options.along_y ? 1.0f / treads : 1.0f, top - bottom }
        } );
    }
    return boxes;
}

#if defined(TILES)
auto emit_prop_cuboids( const emit_prop_options &options ) -> void
{
    if( !options.material.valid ) { return; }
    const auto &material = options.material;
    const auto sample = material.solid_uv.value_or( std::array<float, 2> {
        ( material.u0 + material.u1 ) * 0.5f, ( material.v0 + material.v1 ) * 0.5f
    } );
    const auto uv = uvs_t{ { sample, sample, sample, sample } };
    constexpr auto faces = std::array<std::array<int, 4>, 6> { {
            { 4, 5, 7, 6 }, { 6, 7, 3, 2 }, { 5, 4, 0, 1 },
            { 4, 6, 2, 0 }, { 7, 5, 1, 3 }, { 2, 3, 1, 0 }
        }
    };
    constexpr auto shades = std::array<uint8_t, 6> { 255, 225, 190, 180, 215, 120 };
    for( const auto &box : options.boxes ) {
        auto vertices = std::array<std::array<float, 3>, 8> {};
        const auto yaw = box.yaw + static_cast<float>( options.facing & 3 ) * std::numbers::pi_v<float> *
                         0.5f;
        const auto cy = std::cos( yaw );
        const auto sy = std::sin( yaw );
        const auto cp = std::cos( box.pitch );
        const auto sp = std::sin( box.pitch );
        auto center_x = box.center[0];
        auto center_y = box.center[1];
        for( auto turn = 0; turn < ( options.facing & 3 ); ++turn ) {
            const auto previous = center_x;
            center_x = 1.0f - center_y;
            center_y = previous;
        }
        for( auto i = size_t{ 0 }; i < vertices.size(); ++i ) {
            const auto x = ( ( i & 1 ) != 0 ? 0.5f : -0.5f ) * box.size[0];
            const auto y = ( ( i & 2 ) != 0 ? 0.5f : -0.5f ) * box.size[1];
            const auto z = ( ( i & 4 ) != 0 ? 0.5f : -0.5f ) * box.size[2];
            const auto tilted_x = x * cp - z * sp;
            vertices[i] = { options.x + center_x + tilted_x *cy - y * sy,
                            options.y + center_y + tilted_x *sy + y * cy,
                            options.base + box.center[2] + x *sp + z *cp
                          };
        }
        for( auto side = size_t{ 0 }; side < faces.size(); ++side ) {
            auto corners = corners_t{};
            for( auto i = size_t{ 0 }; i < corners.size(); ++i ) { corners[i] = vertices[faces[side][i]]; }
            options.sink.add( { .sprite = material, .corners = corners, .uvs = uv,
                                .ref = { .lx = static_cast<uint8_t>( options.x ), .ly = static_cast<uint8_t>( options.y ), .shade = shades[side] } } );
        }
    }
}
#endif
} // namespace r3d
