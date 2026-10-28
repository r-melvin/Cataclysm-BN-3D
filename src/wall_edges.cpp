#include "wall_edges.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <string_view>

#include "map/map.h"
#include "map/mapbuffer.h"
#include "map/mapbuffer_registry.h"
#include "map/mapdata.h"
#include "options.h"
#include "r3d_shapes.h"
#include "r3d_wall_layout.h"

namespace wall_edges
{
namespace
{

auto rendering_available = std::atomic_bool{ true };

auto frame_art( std::string_view name ) -> bool
{
    return name == "t_window_frame" || name.starts_with( "t_window_frame_" ) ||
           name == "t_window_empty" || name.starts_with( "t_window_empty_" );
}

struct rectangle {
    double x0;
    double y0;
    double x1;
    double y1;
};

struct intersection_options {
    rectangle box;
    double x;
    double y;
    double dx;
    double dy;
};

auto intersection( const intersection_options &options ) -> std::optional<double>
{
    auto enter = 0.0;
    auto leave = 1.0;
    const auto slab = [&]( double start, double delta, double low, double high ) {
        if( std::abs( delta ) < 1e-12 ) { return start >= low && start <= high; }
        const auto a = ( low - start ) / delta;
        const auto b = ( high - start ) / delta;
        enter = std::max( enter, std::min( a, b ) );
        leave = std::min( leave, std::max( a, b ) );
        return enter <= leave;
    };
    if( !slab( options.x, options.dx, options.box.x0, options.box.x1 ) ||
        !slab( options.y, options.dy, options.box.y0, options.box.y1 ) ) { return std::nullopt; }
    return enter >= 0.0 && enter <= 1.0 ? std::optional<double>( enter ) : std::nullopt;
}

} // namespace

auto enabled() -> bool
{
#if defined(TILES)
    return rendering_available.load( std::memory_order_relaxed ) &&
           get_option<bool>( "RENDERER_3D" ) && get_option<bool>( "R3D_THIN_WALLS" );
#else
    return false;
#endif
}

auto set_rendering_available( const bool available ) -> void
{
    rendering_available.store( available, std::memory_order_relaxed );
}

auto is_door_terrain( const ter_t &terrain ) -> bool
{
    const auto matches = [&]( std::string_view token ) {
        return terrain.id.str().find( token ) != std::string::npos ||
               terrain.looks_like.find( token ) != std::string::npos;
    };
    if( matches( "hatch" ) || matches( "manhole" ) || matches( "grate" ) ||
        matches( "ramp" ) || matches( "_control" ) ) { return false; }
    return terrain.has_flag( "DOOR" ) || terrain.has_flag( "BARRICADABLE_DOOR" ) ||
           terrain.has_flag( "BARRICADABLE_DOOR_DAMAGED" ) ||
           terrain.has_flag( "BARRICADABLE_DOOR_REINFORCED" ) || matches( "door" ) || matches( "gate" );
}

auto is_edge_terrain( const ter_t &terrain ) -> bool
{
    return terrain.has_flag( TFLAG_WALL ) || terrain.has_flag( "WINDOW" ) ||
           ( terrain.movecost > 0 && terrain.has_flag( "CONNECT_TO_WALL" ) &&
             ( frame_art( terrain.id.str() ) || frame_art( terrain.looks_like ) ) ) ||
           is_door_terrain( terrain );
}

auto terrain_move_cost( const ter_t &terrain ) -> int
{
    return enabled() && is_edge_terrain( terrain ) ? 2 : terrain.movecost;
}

auto trace( const trace_options &options ) -> std::optional<barrier>
{
    if( !enabled() || options.from.z() != options.to.z() || ( options.from == options.to &&
            options.padding <= 0.0 ) ) {
        return std::nullopt;
    }
    // Touching an adjacent strip tangentially is not overlap for a swept body.
    const auto padding = std::max( options.padding - 1e-7, 0.0 );
    const auto radius = static_cast<int>( std::ceil( padding ) );
    const auto reader = options.buffer.make_abs_tile_reader( options.lookup );
    const auto z = options.from.z();
    const auto x0 = static_cast<double>( options.from.x() ) + 0.5;
    const auto y0 = static_cast<double>( options.from.y() ) + 0.5;
    const auto dx = static_cast<double>( options.to.x() - options.from.x() );
    const auto dy = static_cast<double>( options.to.y() - options.from.y() );
    auto result = std::optional<barrier> {};
    constexpr auto directions = std::array<point, 4> {{ point( 0, -1 ), point( 0, 1 ), point( -1, 0 ), point( 1, 0 ) }};
    constexpr auto faces = std::array<uint8_t, 4> {{ r3d::face_north, r3d::face_south, r3d::face_west, r3d::face_east }};
    const auto connections_at = [&]( const tripoint_abs_ms & p ) {
        auto mask = uint8_t{ 0 };
        for( auto i = size_t{ 0 }; i < directions.size(); ++i ) {
            const auto d = directions[i];
            const auto next = reader.get_tile( p + tripoint_rel_ms( d.x, d.y, 0 ) );
            if( next && is_edge_terrain( next->get_ter_t() ) ) { mask |= faces[i]; }
        }
        return mask;
    };
    const auto test_cell = [&]( const int x, const int y ) {
        const auto p = tripoint_abs_ms( x, y, z );
        const auto tile = reader.get_tile( p );
        if( !tile || !is_edge_terrain( tile->get_ter_t() ) ) { return; }
        const auto &terrain = tile->get_ter_t();
        if( options.kind == trace_kind::vision && terrain.transparent ) { return; }
        auto indoors = std::array<bool, 9> {};
        auto connections = uint8_t{ 0 };
        for( auto oy = -1; oy <= 1; ++oy ) {
            for( auto ox = -1; ox <= 1; ++ox ) {
                const auto neighbour = reader.get_tile( p + tripoint_rel_ms( ox, oy, 0 ) );
                if( !neighbour ) { continue; }
                const auto &next = neighbour->get_ter_t();
                indoors[( oy + 1 ) * 3 + ox + 1] = next.movecost > 0 &&
                                                   ( next.has_flag( "INDOORS" ) || !next.roof.is_null() ) && !is_edge_terrain( next );
                if( is_edge_terrain( next ) ) {
                    if( ox == 0 && oy == -1 ) { connections |= r3d::face_north; }
                    if( ox == 0 && oy == 1 ) { connections |= r3d::face_south; }
                    if( ox == -1 && oy == 0 ) { connections |= r3d::face_west; }
                    if( ox == 1 && oy == 0 ) { connections |= r3d::face_east; }
                }
            }
        }
        const auto layout = r3d::exterior_wall_layout( { .indoors = indoors,
                            .half_width = thickness * 0.5f } );
        const auto test_box = [&]( const rectangle & box ) {
            const auto expanded = rectangle{ .x0 = x + box.x0 - padding, .y0 = y + box.y0 - padding,
                                             .x1 = x + box.x1 + padding, .y1 = y + box.y1 + padding };
            if( padding > 0.0 && x0 > expanded.x0 && x0 < expanded.x1 &&
                y0 > expanded.y0 && y0 < expanded.y1 ) {
                // Recover an existing overlap by moving out through its nearest face;
                // crossing the strip to its far side remains a collision.
                const auto clearance = std::min( { x0 - expanded.x0, expanded.x1 - x0,
                                                   y0 - expanded.y0, expanded.y1 - y0 } );
                if( ( x0 - expanded.x0 == clearance && x0 + dx < expanded.x0 ) ||
                    ( expanded.x1 - x0 == clearance && x0 + dx > expanded.x1 ) ||
                    ( y0 - expanded.y0 == clearance && y0 + dy < expanded.y0 ) ||
                    ( expanded.y1 - y0 == clearance && y0 + dy > expanded.y1 ) ) { return; }
            }
            const auto hit = intersection( { .box = expanded, .x = x0, .y = y0, .dx = dx, .dy = dy } );
            if( hit && ( !result || *hit < result->fraction ) ) {
                result = barrier{ .tile = p, .fraction = *hit };
            }
        };
        if( terrain.has_flag( TFLAG_WALL ) ) {
            // Preserve the renderer's existing removal of a one-cell inward corner spur.
            if( connections != 0 && ( connections & ( connections - 1 ) ) == 0 ) {
                auto indoor_sides = uint8_t{ 0 };
                auto neighbour_connections = uint8_t{ 0 };
                for( auto i = size_t{ 0 }; i < directions.size(); ++i ) {
                    const auto d = directions[i];
                    const auto next_pos = p + tripoint_rel_ms( d.x, d.y, 0 );
                    const auto next = reader.get_tile( next_pos );
                    if( next && next->get_ter_t().movecost > 0 &&
                        !is_edge_terrain( next->get_ter_t() ) && next->get_ter_t().has_flag( "INDOORS" ) ) {
                        indoor_sides |= faces[i];
                    }
                    if( ( connections & faces[i] ) != 0 ) { neighbour_connections = connections_at( next_pos ); }
                }
                if( r3d::trim_wall_spur( connections, indoor_sides, neighbour_connections ) ) { return; }
            }
            const auto occupied = r3d::wall_footprint( connections );
            for( auto row = 0; row < 3; ++row ) {
                for( auto col = 0; col < 3; ++col ) {
                    if( !occupied[row * 3 + col] ) { continue; }
                    const auto box = rectangle{ .x0 = layout.x_edges[col], .y0 = layout.y_edges[row],
                                                .x1 = layout.x_edges[col + 1], .y1 = layout.y_edges[row + 1] };
                    if( box.x1 > box.x0 && box.y1 > box.y0 ) { test_box( box ); }
                }
            }
            return;
        }
        const auto along_y = ( connections & ( r3d::face_north | r3d::face_south ) ) != 0;
        const auto half = thickness * 0.5f;
        const auto low = along_y ? layout.center_x - half : layout.center_y - half;
        const auto high = low + thickness;
        const auto opening = terrain.movecost > 0 || terrain.has_flag( TFLAG_FLAT );
        const auto jamb = is_door_terrain( terrain ) ? 0.08 : 0.1;
        if( !opening ) {
            test_box( along_y ? rectangle{ .x0 = low, .y0 = 0.0, .x1 = high, .y1 = 1.0 } :
                      rectangle{ .x0 = 0.0, .y0 = low, .x1 = 1.0, .y1 = high } );
        } else {
            test_box( along_y ? rectangle{ .x0 = low, .y0 = 0.0, .x1 = high, .y1 = jamb } :
                      rectangle{ .x0 = 0.0, .y0 = low, .x1 = jamb, .y1 = high } );
            test_box( along_y ? rectangle{ .x0 = low, .y0 = 1.0 - jamb, .x1 = high, .y1 = 1.0 } :
                      rectangle{ .x0 = 1.0 - jamb, .y0 = low, .x1 = 1.0, .y1 = high } );
        }
    };
    const auto test_neighbourhood = [&]( const int cx, const int cy ) {
        for( auto oy = -radius; oy <= radius; ++oy ) {
            for( auto ox = -radius; ox <= radius; ++ox ) { test_cell( cx + ox, cy + oy ); }
        }
    };
    auto x = options.from.x();
    auto y = options.from.y();
    const auto step_x = dx > 0.0 ? 1 : dx < 0.0 ? -1 : 0;
    const auto step_y = dy > 0.0 ? 1 : dy < 0.0 ? -1 : 0;
    const auto delta_x = step_x == 0 ? std::numeric_limits<double>::infinity() : 1.0 / std::abs( dx );
    const auto delta_y = step_y == 0 ? std::numeric_limits<double>::infinity() : 1.0 / std::abs( dy );
    auto next_x = delta_x * 0.5;
    auto next_y = delta_y * 0.5;
    for( ;; ) {
        test_neighbourhood( x, y );
        const auto next = std::min( next_x, next_y );
        if( next > 1.0 || ( result && result->fraction < next ) ) { break; }
        if( std::abs( next_x - next_y ) < 1e-12 ) {
            test_neighbourhood( x + step_x, y );
            test_neighbourhood( x, y + step_y );
            x += step_x;
            y += step_y;
            next_x += delta_x;
            next_y += delta_y;
        } else if( next_x < next_y ) {
            x += step_x;
            next_x += delta_x;
        } else {
            y += step_y;
            next_y += delta_y;
        }
    }
    return result;
}

auto trace( const local_trace_options &options ) -> std::optional<tripoint_bub_ms>
{
    auto &buffer = MAPBUFFER_REGISTRY.get( options.here.get_bound_dimension() );
    const auto hit = trace( { .buffer = buffer, .from = map_local_to_abs( options.here, options.from ),
                              .to = map_local_to_abs( options.here, options.to ), .kind = options.kind,
                              .padding = options.padding } );
    return hit ? std::optional<tripoint_bub_ms>( abs_to_map_local( options.here,
            hit->tile ) ) : std::nullopt;
}

} // namespace wall_edges
