#include "r3d_item_profiles.h"

#include <array>
#include <ranges>

namespace r3d
{

auto item_profile( std::string_view id ) -> item_render_profile
{
    constexpr auto upright_cylinders = std::array<std::string_view, 11> {
        "carbondioxide_tank", "hydrogen_tank", "nitrogen_tank", "oxygen_cylinder", "oxygen_tank",
        "smoxygen_tank", "scuba_tank", "small_scuba_tank", "tinyweldtank", "weldtank",
        "gradcylinder"
    };
    if( std::ranges::find( upright_cylinders, id ) != upright_cylinders.end() ) {
        // One sprite is repeated around a shallow four-sided volume, as in single-sprite voxel tools.
        const auto depth = id == "small_scuba_tank" || id == "tinyweldtank" ? 0.28f : 0.38f;
        return { .geometry = item_geometry::repeated_sprite_volume, .depth_fraction = depth };
    }
    return {};
}

} // namespace r3d
