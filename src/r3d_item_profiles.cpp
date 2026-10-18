#include "r3d_item_profiles.h"

#include <array>
#include <ranges>

namespace r3d
{

auto item_profile( std::string_view id ) -> item_render_profile
{
    constexpr auto pressurized_tanks = std::array<std::string_view, 6> {
        "carbondioxide_tank", "hydrogen_tank", "nitrogen_tank", "oxygen_cylinder", "oxygen_tank",
        "smoxygen_tank"
    };
    if( std::ranges::find( pressurized_tanks, id ) != pressurized_tanks.end() ) {
        // One sprite is repeated around a shallow four-sided volume, as in single-sprite voxel tools.
        return { .geometry = item_geometry::repeated_sprite_volume, .depth_fraction = 0.38f };
    }
    return {};
}

} // namespace r3d
