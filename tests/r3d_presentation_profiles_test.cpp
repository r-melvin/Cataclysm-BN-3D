#include "catch/catch.hpp"
#include "r3d_presentation_profiles.h"

#include <sstream>
#include <string>

TEST_CASE(
    "r3d_presentation_profiles_load_dimensions_and_anchors_alongside_gallery_metadata",
    "[r3d][profiles]") {
    auto input = std::istringstream(R"({
        "version": 1,
        "profiles": [{
            "category": "item",
            "id": "carbondioxide_tank",
            "size": [0.48, 0.34, 0.86],
            "support_anchor": [0.5, 0.5, 0.0],
            "depth_fraction": 0.38,
            "material_regions": [
                { "name": "tank_shell", "role": "painted metal", "palette_indices": [1, 2] }
            ]
        }]
    })");
    const auto profiles = r3d::load_presentation_profiles(input);
    const auto& profile = profiles.at("item:carbondioxide_tank");
    CHECK(profile.size[0] == Approx(0.48f));
    CHECK(profile.size[1] == Approx(0.34f));
    CHECK(profile.size[2] == Approx(0.86f));
    CHECK((profile.support_anchor == std::array<float, 3>{0.5f, 0.5f, 0.0f}));
    CHECK(profile.depth_fraction == Approx(0.38f));
}

TEST_CASE("r3d_presentation_profiles_reject_invalid_anchors", "[r3d][profiles]") {
    const auto invalid_anchor = std::string(R"({ "version": 1, "profiles": [{
        "category": "item", "id": "bad", "support_anchor": [1.1, 0.5, 0]
    }] })");
    auto anchor_input = std::istringstream(invalid_anchor);
    CHECK_THROWS(r3d::load_presentation_profiles(anchor_input));
}
