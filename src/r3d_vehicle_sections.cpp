#include "r3d_vehicle_sections.h"

#include <algorithm>
#include <cmath>
#include <set>

#include "r3d_vehicle_silhouette.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"
#include "vehicle/veh_type.h"

namespace r3d
{
namespace
{

auto neighbors( const tripoint_mnt_veh &mount ) -> std::array<tripoint_mnt_veh, 4>
{
    return { tripoint_mnt_veh( mount.x() - 1, mount.y(), mount.z() ),
             tripoint_mnt_veh( mount.x() + 1, mount.y(), mount.z() ),
             tripoint_mnt_veh( mount.x(), mount.y() - 1, mount.z() ),
             tripoint_mnt_veh( mount.x(), mount.y() + 1, mount.z() ) };
}

auto include_mount( vehicle_section_bounds &bounds, const tripoint_mnt_veh &mount ) -> void
{
    bounds.front = std::max( bounds.front, mount.x() );
    bounds.rear = std::min( bounds.rear, mount.x() );
    bounds.left = std::min( bounds.left, mount.y() );
    bounds.right = std::max( bounds.right, mount.y() );
}

auto initial_bounds( const tripoint_mnt_veh &mount ) -> vehicle_section_bounds
{
    return { .front = mount.x(), .rear = mount.x(), .left = mount.y(), .right = mount.y() };
}

struct connect_sections_options {
    vehicle_sections &sections;
    bool canopy;
};

auto connect_sections( const connect_sections_options &options ) -> void
{
    auto &cells = options.sections.cells;
    auto &groups = options.canopy ? options.sections.canopies : options.sections.bodies;
    const auto eligible = [&]( const vehicle_section_cell & cell ) {
        return options.canopy ? ( cell.profile.has_roof || cell.glazing || cell.profile.is_door ) &&
               !cell.full_panel : cell.skin;
    };
    for( auto &[mount, cell] : cells ) {
        if( !eligible( cell ) || ( options.canopy ? cell.canopy : cell.body ) >= 0 ) { continue; }
        const auto id = static_cast<int>( groups.size() );
        auto group = vehicle_section{ .bounds = initial_bounds( mount ), .mounts = { mount } };
        ( options.canopy ? cell.canopy : cell.body ) = id;
        for( auto i = size_t{0}; i < group.mounts.size(); ++i ) {
            const auto current = group.mounts[i];
            const auto &member = cells.at( current );
            include_mount( group.bounds, current );
            group.glazed |= member.glazing;
            for( const auto next : neighbors( current ) ) {
                const auto found = cells.find( next );
                if( found == cells.end() ) { continue; }
                auto &candidate = found->second;
                group.box |= candidate.full_panel;
                auto &assigned = options.canopy ? candidate.canopy : candidate.body;
                if( assigned < 0 && eligible( candidate ) ) {
                    assigned = id;
                    group.mounts.push_back( next );
                }
            }
        }
        groups.push_back( std::move( group ) );
    }
}

} // namespace

auto describe_vehicle_sections( const vehicle &veh ) -> vehicle_sections
{
    auto result = vehicle_sections{};
    for( auto i = 0; i < veh.part_count(); ++i ) {
        const auto &part = veh.cpart( i );
        if( part.removed ) { continue; }
        if( result.cells.contains( part.mount ) ) { continue; }
        result.cells.try_emplace( part.mount, vehicle_section_cell{
            .profile = vehicle_profile( veh, part.mount ), .row = initial_bounds( part.mount )
        } );
    }
    for( auto &[mount, cell] : result.cells ) {
        for( const auto &layer : cell.profile.layers ) {
            const auto &info = veh.part_info( layer.part_idx );
            cell.full_panel |= !layer.broken && info.has_flag( "FULL_BOARD" );
            cell.glazing |= info.has_flag( "WINDSHIELD" );
            cell.skin |= info.has_flag( "HALF_BOARD" ) || info.has_flag( "FULL_BOARD" ) ||
                         info.has_flag( "ROOF" ) || info.has_flag( "WINDSHIELD" ) ||
                         info.location == "door" || info.has_flag( "CARGO" ) || info.has_flag( "SEAT" ) ||
                         info.has_flag( "AISLE" );
        }
    }
    connect_sections( { .sections = result, .canopy = false } );
    connect_sections( { .sections = result, .canopy = true } );
    const auto aircraft = describe_vehicle( veh ).rotorcraft;
    for( auto &canopy : result.canopies ) {
        auto roof_rows = std::set<int> {};
        auto wheels = std::vector<int> {};
        const auto body = result.cells.at( canopy.mounts.front() ).body;
        for( const auto mount : canopy.mounts ) {
            if( result.cells.at( mount ).profile.has_roof ) { roof_rows.insert( mount.x() ); }
        }
        // A distant trailer axle or a decorative spare does not resize the cabin.
        // Use the typical installed wheel around this section, not a vehicle-wide maximum.
        for( const auto &[mount, candidate] : result.cells ) {
            if( candidate.body != body || mount.x() < canopy.bounds.rear - 2 ||
                mount.x() > canopy.bounds.front + 2 ) { continue; }
            for( const auto &layer : candidate.profile.layers ) {
                if( veh.part_flag( layer.part_idx, "WHEEL" ) && !layer.broken ) {
                    wheels.push_back( veh.cpart( layer.part_idx ).wheel_diameter() );
                }
            }
        }
        std::ranges::sort( wheels );
        const auto typical_wheel = wheels.empty() ? 0 : wheels[wheels.size() / 2];
        auto native_height = 0.0f;
        for( const auto mount : canopy.mounts ) {
            native_height = std::max( native_height, result.cells.at( mount ).profile.height );
        }
        canopy.height = aircraft ? 1.04f : !canopy.glazed ? ( canopy.box ? 1.18f : native_height ) :
                        typical_wheel >= 650 ? 1.02f : roof_rows.size() <= 2 ? 0.74f : vehicle_cabin_h;
    }
    for( auto &[mount, cell] : result.cells ) {
        if( cell.body >= 0 ) {
            // Follow the contiguous skin run at this station. An outrigger or a
            // second body across an empty column cannot widen this cross-section.
            for( const auto side : { -1, 1 } ) {
                auto y = mount.y() + side;
                while( true ) {
                    const auto found = result.cells.find( tripoint_mnt_veh( mount.x(), y, mount.z() ) );
                    if( found == result.cells.end() || found->second.body != cell.body ) { break; }
                    include_mount( cell.row, found->first );
                    y += side;
                }
            }
        }
        cell.height = cell.profile.height;
        if( cell.canopy >= 0 ) { cell.height = result.canopies[cell.canopy].height; }
        if( cell.full_panel ) {
            for( const auto next : neighbors( mount ) ) {
                const auto found = result.cells.find( next );
                if( found != result.cells.end() && !found->second.full_panel && found->second.canopy >= 0 ) {
                    const auto candidate = found->second.canopy;
                    if( result.canopies[candidate].height >= cell.height ) {
                        cell.height = result.canopies[candidate].height;
                        cell.canopy = candidate;
                    }
                }
            }
        }
    }
    return result;
}

auto vehicle_wheel_opening( const vehicle_wheel_opening_options &options ) ->
std::vector<vehicle_wall_strip>
{
    if( options.to <= options.from ) { return {}; }
    auto cuts = std::vector<float> { options.from, options.to };
    if( options.radius_forward > 0.0f && options.radius_up > 0.0f ) {
        for( auto step = 0; step <= 12; ++step ) {
            const auto x = options.center + options.radius_forward * ( static_cast<float>
                           ( step ) / 6.0f - 1.0f );
            if( x > options.from && x < options.to ) { cuts.push_back( x ); }
        }
    }
    std::ranges::sort( cuts );
    const auto bottom = [&]( const float forward ) {
        if( options.radius_forward <= 0.0f || options.radius_up <= 0.0f ) { return options.bottom; }
        const auto x = ( forward - options.center ) / options.radius_forward;
        if( std::abs( x ) > 1.0f ) { return options.bottom; }
        return std::max( options.bottom, options.center_z + options.radius_up * std::sqrt( std::max( 0.0f,
                         1.0f - x * x ) ) );
    };
    auto result = std::vector<vehicle_wall_strip> {};
    for( auto i = size_t{1}; i < cuts.size(); ++i ) {
        const auto within = std::abs( ( cuts[i - 1] + cuts[i] ) * 0.5f - options.center ) <
                            options.radius_forward;
        result.push_back( { .from = cuts[i - 1], .to = cuts[i],
                            .bottom_from = within ? bottom( cuts[i - 1] ) : options.bottom,
                            .bottom_to = within ? bottom( cuts[i] ) : options.bottom } );
    }
    return result;
}

auto vehicle_section_inset( const vehicle_section_inset_options &options ) -> float
{
    const auto span = options.right - options.left;
    if( span <= 0.0f ) { return options.position; }
    const auto center = ( options.left + options.right ) * 0.5f;
    const auto fit = std::max( 0.1f, 1.0f - options.amount / ( span * 0.5f ) );
    return center + ( options.position - center ) * fit;
}

} // namespace r3d
