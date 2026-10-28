#include "r3d_vehicle_silhouette.h"

#include <algorithm>
#include <limits>
#include <set>
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"
#include "vehicle/veh_type.h"

namespace r3d
{
auto describe_vehicle( const vehicle &veh ) -> vehicle_silhouette
{
    auto result = vehicle_silhouette{};
    auto roofs = std::set<int> {};
    auto glass = 0;
    auto cabin_left = std::numeric_limits<int>::max();
    auto cabin_right = std::numeric_limits<int>::lowest();
    auto wheel_diameter = 0;
    auto rear = std::numeric_limits<int>::max();
    auto left = std::numeric_limits<int>::max();
    auto right = std::numeric_limits<int>::lowest();
    auto cabin_rear = std::numeric_limits<int>::max();
    for( auto i = 0; i < veh.part_count(); ++i ) {
        const auto &part = veh.cpart( i );
        if( part.removed ) { continue; }
        const auto &info = veh.part_info( i );
        // Underbody VTOL thrusters are not helicopter main rotors.
        result.main_rotors += info.has_flag( "ROTOR" ) && info.location == "on_roof" ? 1 : 0;
        if( info.location == "structure" ) {
            rear = std::min( rear, part.mount.x() );
            left = std::min( left, part.mount.y() );
            right = std::max( right, part.mount.y() );
        }
        if( info.has_flag( "WHEEL" ) ) { wheel_diameter = std::max( wheel_diameter, part.wheel_diameter() ); }
        if( info.has_flag( "ROOF" ) ) { roofs.insert( part.mount.x() ); }
        glass += info.has_flag( "WINDSHIELD" ) ? 1 : 0;
        if( info.has_flag( "ROOF" ) || info.has_flag( "WINDSHIELD" ) || info.location == "door" ) {
            cabin_rear = std::min( cabin_rear, part.mount.x() );
            cabin_left = std::min( cabin_left, part.mount.y() );
            cabin_right = std::max( cabin_right, part.mount.y() );
        }
    }
    result.rotorcraft = result.main_rotors > 0;
    result.enclosed = !roofs.empty() && glass >= 2;
    if( !result.enclosed ) { return result; }
    result.cabin_rear = cabin_rear;
    result.roof_front = std::ranges::max( roofs );
    result.roof_rear = std::ranges::min( roofs );
    result.rotor_forward = ( result.roof_front + result.roof_rear ) * 0.5f;
    result.rotor_right = ( static_cast<float>( cabin_left ) + cabin_right ) * 0.5f;
    result.tail_rear = rear;
    result.tail_center = ( static_cast<float>( left ) + right ) * 0.5f;
    result.cabin_height = result.rotorcraft ? 1.04f :
                          roofs.size() >= 5 ? 1.18f :
                          wheel_diameter >= 650 || right - left >= 4 ? 1.02f :
                          roofs.size() <= 2 ? 0.74f :
                          roofs.size() >= 4 ? 0.94f : vehicle_cabin_h;
    return result;
}

auto describe_airframe_section( const airframe_section_options &options ) -> airframe_section
{
    const auto &shape = options.silhouette;
    if( !shape.rotorcraft || !shape.enclosed || options.mount.x() >= shape.cabin_rear ) {
        return airframe_section::cabin;
    }
    auto half_panel = false;
    auto cover = false;
    for( const auto i : options.veh.parts_at_relative( options.mount, true ) ) {
        const auto &part = options.veh.cpart( i );
        if( part.removed ) { continue; }
        const auto &info = options.veh.part_info( i );
        if( info.has_flag( "CARGO" ) || info.has_flag( "BOARDABLE" ) || info.has_flag( "FULL_BOARD" ) ||
            info.has_flag( "ROOF" ) || info.has_flag( "WINDSHIELD" ) ) { return airframe_section::cabin; }
        half_panel |= info.has_flag( "HALF_BOARD" );
        cover |= info.has_flag( "HALF_BOARD" ) && info.id.str().ends_with( "_cover" );
    }
    if( !half_panel ) { return airframe_section::cabin; }
    if( cover ) { return airframe_section::stabilizer; }
    return options.mount.x() == shape.tail_rear ? airframe_section::fin : airframe_section::boom;
}

auto native_wheel_dimensions( const vehicle_part &part ) -> wheel_dimensions
{
    const auto radius = std::clamp( part.wheel_diameter() * 0.001f * 0.5f, 0.10f, 0.65f );
    return { .radius_forward = radius, .radius_up = radius / 1.8f,
             .width = std::clamp( part.wheel_width() * 0.001f, 0.06f, 0.70f ) };
}
}
