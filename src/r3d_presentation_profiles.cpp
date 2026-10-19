#include "r3d_presentation_profiles.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "json.h"

namespace r3d
{

namespace
{

auto read_vec3( const JsonObject &profile, const std::string &name,
                const std::array<float, 3> &fallback ) -> std::array<float, 3>
{
    if( !profile.has_member( name ) ) {
        return fallback;
    }
    const auto values = profile.get_array( name );
    if( values.size() != 3 ) {
        profile.throw_error( "expected three numeric values", name );
    }
    return { static_cast<float>( values.get_float( 0 ) ),
             static_cast<float>( values.get_float( 1 ) ),
             static_cast<float>( values.get_float( 2 ) ) };
}

auto valid_positive_size( const std::array<float, 3> &size ) -> bool
{
    return std::ranges::all_of( size, []( const float value ) {
        return value == 0.0f || ( value > 0.0f && value <= 8.0f );
    } );
}

} // namespace

auto load_presentation_profiles( std::istream &input ) -> presentation_profile_map
{
    auto json = JsonIn( input );
    const auto root = json.get_object();
    if( root.get_int( "version" ) != 1 ) {
        throw std::runtime_error( "unsupported presentation profile version" );
    }
    auto profiles = presentation_profile_map {};
    for( const JsonObject &entry : root.get_array( "profiles" ) ) {
        const auto category = entry.get_string( "category" );
        const auto id = entry.get_string( "id" );
        if( category.empty() || id.empty() ) {
            entry.throw_error( "category and id must not be empty" );
        }
        auto profile = presentation_profile {
            .size = read_vec3( entry, "size", {} ),
            .support_anchor = read_vec3( entry, "support_anchor", { 0.5f, 0.5f, 0.0f } ),
            .depth_fraction = static_cast<float>( entry.get_float( "depth_fraction", 0.0 ) )
        };
        if( !valid_positive_size( profile.size ) || profile.depth_fraction < 0.0f ||
            profile.depth_fraction > 4.0f ||
        !std::ranges::all_of( profile.support_anchor, []( const float coordinate ) {
        return coordinate >= 0.0f && coordinate <= 1.0f;
    } ) ) {
            entry.throw_error( "size and support anchor values are outside their valid range" );
        }
        // Gallery annotations stay in the shared JSON, but have no runtime material consumer.
        if( entry.has_member( "material_regions" ) ) {
            static_cast<void>( entry.get_member( "material_regions" ) );
        }
        profiles.insert_or_assign( presentation_profile_key( category, id ), std::move( profile ) );
    }
    return profiles;
}

auto presentation_profile_key( const std::string &category, const std::string &id ) -> std::string
{
    return category + ":" + id;
}

} // namespace r3d
