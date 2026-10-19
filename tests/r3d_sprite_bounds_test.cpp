#if defined(TILES)
#    include "catch/catch.hpp"
#    include "r3d_entities.h"

#    include <cmath>

namespace {
auto approx(float a, float b, float eps = 1e-3f) -> bool { return std::fabs(a - b) <= eps; }
} // namespace

TEST_CASE("r3d_vehicle_opaque_bounds_trimming", "[r3d]") {
    // Full/unknown sprite bounds: has_trim must be false
    const auto s_full = r3d::sprite_ref{
        .opaque_x0 = 0.0f, .opaque_y0 = 0.0f, .opaque_x1 = 1.0f, .opaque_y1 = 1.0f, .valid = true};
    const auto b_full = r3d::get_sprite_opaque_bounds(s_full);
    CHECK_FALSE(b_full.has_trim);

    // Door sprite with transparent outer margin on right:
    // occupies [0.0, 0.75] horizontally and [0.0, 1.0] vertically
    const auto s_door = r3d::sprite_ref{
        .quarter_turns = 0,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 0.75f,
        .opaque_y1 = 1.0f,
        .valid = true};
    const auto b_door = r3d::get_sprite_opaque_bounds(s_door);
    CHECK(b_door.has_trim);
    CHECK(approx(b_door.local_r_min, -0.5f));
    CHECK(approx(b_door.local_r_max, 0.25f)); // -0.5f + 0.75f = 0.25f
    CHECK(approx(b_door.local_f_min, -0.5f));
    CHECK(approx(b_door.local_f_max, 0.5f));

    // 90-degree rotated door (quarter_turns = 1):
    // In rotated space, sx = gy, sy = 1 - gx
    const auto s_rot1 = r3d::sprite_ref{
        .quarter_turns = 1,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 1.0f,
        .opaque_y1 = 0.75f,
        .valid = true};
    const auto b_rot1 = r3d::get_sprite_opaque_bounds(s_rot1);
    CHECK(b_rot1.has_trim);
    CHECK(approx(b_rot1.local_r_min, -0.25f));
    CHECK(approx(b_rot1.local_r_max, 0.5f));

    // Flipped door (flip = true):
    const auto s_flip = r3d::sprite_ref{
        .flip = true,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 0.75f,
        .opaque_y1 = 1.0f,
        .valid = true};
    const auto b_flip = r3d::get_sprite_opaque_bounds(s_flip);
    CHECK(b_flip.has_trim);
    CHECK(approx(b_flip.local_r_min, -0.25f));
    CHECK(approx(b_flip.local_r_max, 0.5f));

    // Snapping rule: interior shared edges snap to -0.5f and +0.5f, exterior trims
    const auto has_left_neighbor = true;
    const auto has_right_neighbor = false;
    const auto r_l_rel = has_left_neighbor ? -0.5f : b_door.local_r_min;
    const auto r_r_rel = has_right_neighbor ? 0.5f : b_door.local_r_max;
    CHECK(approx(r_l_rel, -0.5f));
    CHECK(approx(r_r_rel, 0.25f));
}


#endif
