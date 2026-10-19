#include "r3d_vehicle_rules.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <utility>
#include "r3d_shapes.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"
#include "vehicle/veh_type.h"

namespace r3d
{
auto vehicle_profile( const vehicle &veh, const tripoint_mnt_veh &mount ) -> vehicle_render_profile
{
    auto data = vehicle_render_profile{ .mount = mount, .layers = {} };
    const auto parts = veh.parts_at_relative( mount, true );
    data.layers.reserve( parts.size() );
    auto half_height_panel = false;
    for( const auto index : parts ) {
        const auto &part = veh.cpart( index );
        const auto &info = veh.part_info( index );
        if( part.removed ) { continue; }
        auto layer = vehicle_render_layer{ .part_idx = index, .roles = {}, .open = part.open,
                                           .broken = part.is_broken() };
        if( info.location == "structure" ) {
            layer.roles.push_back( vehicle_render_role::frame );
        }
        if( info.has_flag( "HALF_BOARD" ) || info.has_flag( "FULL_BOARD" ) ) {
            layer.roles.push_back( vehicle_render_role::body_panel );
        }
        if( info.has_flag( "WINDSHIELD" ) ) {
            layer.roles.push_back( vehicle_render_role::glass );
        }
        if( info.has_flag( "OPAQUE" ) && !info.has_flag( "WINDSHIELD" ) &&
            !info.has_flag( "OPENABLE" ) ) {
            layer.roles.push_back( vehicle_render_role::opaque_structure );
        }
        if( info.has_flag( "DOOR" ) || info.location == "door" ) {
            layer.roles.push_back( vehicle_render_role::door );
        }
        if( info.has_flag( "OPENABLE" ) ) {
            layer.roles.push_back( vehicle_render_role::openable );
        }
        if( info.has_flag( "WHEEL" ) ) {
            layer.roles.push_back( vehicle_render_role::wheel );
        }
        if( info.has_flag( "ROOF" ) ) {
            layer.roles.push_back( vehicle_render_role::roof );
        }
        const auto accessory = info.location == "on_roof" || info.has_flag( "SOLAR_PANEL" ) ||
                               info.has_flag( "TURRET" );
        if( accessory ) {
            layer.roles.push_back( vehicle_render_role::roof_accessory );
        }
        if( info.has_flag( "BOARDABLE" ) || info.has_flag( "AISLE" ) || info.has_flag( "SEAT" ) ||
            info.has_flag( "BED" ) || ( info.has_flag( "OPENABLE" ) && part.open ) ) {
            layer.roles.push_back( vehicle_render_role::interior );
        }
        if( layer.roles.empty() ) {
            layer.roles.push_back( vehicle_render_role::detail );
        }
        data.layers.push_back( std::move( layer ) );
        const auto has_role = [&data]( const vehicle_render_role role ) {
            return std::ranges::any_of( data.layers.back().roles, [role]( const auto layer_role ) {
                return layer_role == role;
            } );
        };
        if( info.z_order < 9 && ( data.main_part_idx < 0 ||
                                  info.z_order > veh.part_info( data.main_part_idx ).z_order ) ) {
            data.main_part_idx = index;
        }
        // A broken wheel is still a wheel: it is drawn with its broken sprite.
        data.has_wheel |= has_role( vehicle_render_role::wheel );
        if( part.is_broken() ) { continue; }
        const auto roof = has_role( vehicle_render_role::roof );
        if( roof ) {
            data.has_roof = true;
            data.roof_part_idx = index;
        }
        if( accessory && ( data.accessory_part_idx < 0 ||
                           info.z_order > veh.part_info( data.accessory_part_idx ).z_order ) ) {
            data.accessory_part_idx = index;
        }
        const auto half_panel = info.has_flag( "HALF_BOARD" );
        const auto full_panel = info.has_flag( "FULL_BOARD" );
        half_height_panel |= half_panel;
        if( half_panel || full_panel ) {
            // The loader appends the JSON shapes[].direction token to each generated id.
            const auto &id = info.id.str();
            if( id.ends_with( "_nw" ) ) { data.corner = panel_corner::front_left; }
            else if( id.ends_with( "_ne" ) ) { data.corner = panel_corner::front_right; }
            else if( id.ends_with( "_sw" ) ) { data.corner = panel_corner::rear_left; }
            else if( id.ends_with( "_se" ) ) { data.corner = panel_corner::rear_right; }
        }
        data.is_windshield |= has_role( vehicle_render_role::glass );
        data.is_openable |= has_role( vehicle_render_role::openable );
        data.is_door |= has_role( vehicle_render_role::door );
        data.is_opaque_wall |= has_role( vehicle_render_role::opaque_structure );
        data.is_tall_cabin |= info.has_flag( "WINDSHIELD" ) || info.has_flag( "OPAQUE" ) ||
                              ( info.has_flag( "OBSTACLE" ) && !half_panel && !part.open );
        data.is_interior |= has_role( vehicle_render_role::interior );
    }
    data.show_roof = data.has_roof;
    data.height = data.is_tall_cabin ? vehicle_cabin_h : half_height_panel ? vehicle_body_h :
                  data.is_interior ? chassis_floor_h : vehicle_body_h;
    return data;
}

auto vehicle_surface_height( const vehicle &veh, const tripoint_mnt_veh &mount ) -> float
{
    return vehicle_profile( veh, mount ).is_interior ? chassis_floor_h : 0.0f;
}

auto preferred_wheel_layer( std::span<const vehicle_render_layer> layers ) -> const vehicle_render_layer * // *NOPAD*
{
    const auto is_wheel = []( const auto & layer ) {
        return std::ranges::find( layer.roles, vehicle_render_role::wheel ) != layer.roles.end();
    };
    const auto intact = std::ranges::find_if( layers, [&is_wheel]( const auto & layer ) {
        return is_wheel( layer ) && !layer.broken;
    } );
    if( intact != layers.end() ) {
        return &*intact;
    }
    const auto any = std::ranges::find_if( layers, is_wheel );
    return any != layers.end() ? &*any : nullptr;
}

auto roof_cap_height( float wall_height ) -> float
{
    return wall_height >= vehicle_body_h ? wall_height : vehicle_cabin_h;
}

auto panel_outline( const panel_outline_options &options ) -> std::vector<std::array<float, 2>>
{
    const auto &o = options;
    const auto cut = std::min( o.front - o.rear, o.right - o.left ) * 0.3f;
    auto result = std::vector<std::array<float, 2>> {};
    const auto corners = std::array<std::array<float, 2>, 4> {{
            {o.front, o.left}, {o.front, o.right}, {o.rear, o.right}, {o.rear, o.left}
        }
    };
    const auto selected = o.corner == panel_corner::front_left ? 0 :
                          o.corner == panel_corner::front_right ? 1 :
                          o.corner == panel_corner::rear_right ? 2 :
                          o.corner == panel_corner::rear_left ? 3 : -1;
    for( auto index = 0; index < 4; ++index ) {
        const auto &current = corners[index];
        if( index != selected ) { result.push_back( current ); continue; }
        for( const auto adjacent : { ( index + 3 ) % 4, ( index + 1 ) % 4 } ) {
            const auto &next = corners[adjacent];
            result.push_back( { current[0] + ( next[0] > current[0] ? cut : next[0] < current[0] ? -cut : 0.0f ),
                                current[1] + ( next[1] > current[1] ? cut : next[1] < current[1] ? -cut : 0.0f ) } );
        }
    }
    return result;
}

auto classify_outline_edge( const outline_edge_options &options ) -> outline_edge
{
    const auto &o = options;
    const auto on = []( const float a, const float b, const float side ) {
        return std::abs( a - side ) < 1e-4f && std::abs( b - side ) < 1e-4f;
    };
    return on( o.a[0], o.b[0], o.front ) ? outline_edge::front :
           on( o.a[0], o.b[0], o.rear ) ? outline_edge::rear :
           on( o.a[1], o.b[1], o.left ) ? outline_edge::left :
           on( o.a[1], o.b[1], o.right ) ? outline_edge::right : outline_edge::chamfer;
}

auto wheel_outline( const wheel_outline_options &options ) -> std::vector<std::array<float, 2>>
{
    const auto segments = std::max( options.segments, 3 );
    auto result = std::vector<std::array<float, 2>> {};
    result.reserve( static_cast<size_t>( segments ) );
    for( auto index = 0; index < segments; ++index ) {
        const auto angle = 2.0f * std::numbers::pi_v<float> *static_cast<float>( index ) /
                           static_cast<float>( segments );
        result.push_back( { options.center_forward + std::cos( angle ) * options.radius_forward,
                            options.center_z + std::sin( angle ) * options.radius_z } );
    }
    return result;
}
}
