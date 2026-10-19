#include "r3d_item_profiles.h"

#include <array>
#include <ranges>

namespace r3d
{

auto item_profile( std::string_view id ) -> item_render_profile
{
    // A shallow silhouette extrusion gives every supported item sprite a small amount of volume.
    // Cylinders need extra depth to read as canisters when viewed from the side.
    constexpr auto default_depth = 0.22f;
    constexpr auto upright_cylinders = std::array<std::string_view, 11> {
        "carbondioxide_tank", "hydrogen_tank", "nitrogen_tank", "oxygen_cylinder", "oxygen_tank",
        "smoxygen_tank", "scuba_tank", "small_scuba_tank", "tinyweldtank", "weldtank",
        "gradcylinder"
    };
    if( std::ranges::find( upright_cylinders, id ) != upright_cylinders.end() ) {
        // One sprite is repeated around a shallow four-sided volume, as in single-sprite voxel tools.
        const auto depth = id == "small_scuba_tank" || id == "tinyweldtank" ? 0.28f : 0.38f;
        return { .geometry = item_geometry::sprite_volume, .depth_fraction = depth };
    }
    return { .geometry = item_geometry::sprite_volume, .depth_fraction = default_depth };
}

auto resolve_item_depth_fraction( const item_depth_options &options ) -> float
{
    if( options.authored_fraction > 0.0f ) {
        return options.authored_fraction;
    }
    if( options.size[0] > 0.0f && options.size[1] > 0.0f ) {
        return options.size[1] / options.size[0];
    }
    return options.fallback;
}

} // namespace r3d
