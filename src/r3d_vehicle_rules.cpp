#include "r3d_vehicle_rules.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include "r3d_shapes.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"
#include "vehicle/veh_type.h"

namespace r3d
{
auto vehicle_profile( const vehicle &veh, const tripoint_mnt_veh &mount ) -> vehicle_render_profile
{
    auto data = vehicle_render_profile{ .mount = mount };
    const auto parts = veh.parts_at_relative( mount, true );
    auto half_height_panel = false;
    for( const auto index : parts ) {
        const auto &part = veh.cpart( index );
        const auto &info = veh.part_info( index );
        if( part.removed ) { continue; }
        if( info.z_order < 9 && ( data.main_part_idx < 0 ||
                                  info.z_order > veh.part_info( data.main_part_idx ).z_order ) ) {
            data.main_part_idx = index;
        }
        if( part.is_broken() ) { continue; }
        const auto roof = info.has_flag( "ROOF" );
        const auto accessory = info.location == "on_roof" || info.has_flag( "SOLAR_PANEL" ) ||
                               info.has_flag( "TURRET" );
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
        data.is_windshield |= info.has_flag( "WINDSHIELD" );
        data.is_openable |= info.has_flag( "OPENABLE" );
        data.is_door |= info.has_flag( "DOOR" ) || info.location == "door";
        data.has_wheel |= info.has_flag( "WHEEL" );
        data.is_opaque_wall |= info.has_flag( "OPAQUE" ) && !info.has_flag( "WINDSHIELD" ) &&
                               !info.has_flag( "OPENABLE" );
        data.is_tall_cabin |= info.has_flag( "WINDSHIELD" ) || info.has_flag( "OPAQUE" ) ||
                              ( info.has_flag( "OBSTACLE" ) && !half_panel && !part.open );
        data.is_interior |= info.has_flag( "BOARDABLE" ) || info.has_flag( "AISLE" ) ||
                            info.has_flag( "SEAT" ) || info.has_flag( "BED" ) ||
                            ( info.has_flag( "OPENABLE" ) && part.open );
    }
    data.show_roof = data.has_roof;
    data.height = data.is_tall_cabin ? vehicle_cabin_h : half_height_panel ? vehicle_body_h :
                  data.is_interior ? chassis_floor_h : vehicle_body_h;
    return data;
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

auto wheel_outline( const wheel_outline_options &options ) -> std::vector<std::array<float, 2>>
{
    const auto segments = std::max( options.segments, 3 );
    auto result = std::vector<std::array<float, 2>>{};
    result.reserve( static_cast<size_t>( segments ) );
    for( auto index = 0; index < segments; ++index ) {
        const auto angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>( index ) /
                           static_cast<float>( segments );
        result.push_back( { options.center_forward + std::cos( angle ) * options.radius_forward,
                            options.center_z + std::sin( angle ) * options.radius_z } );
    }
    return result;
}
}
