#if defined(TILES)
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "catch/catch.hpp"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"
#    include "vehicle/vehicle.h"

#    include <cmath>
#    include <ranges>

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


TEST_CASE("r3d_oversized_and_tall_vehicle_elevations_cover_one_tile", "[r3d][vehicle]") {
    for (const auto rotation : {0, 1, 2, 3}) {
        for (const auto flip : {false, true}) {
            auto hatch = r3d::sprite_ref{};
            hatch.valid = true;
            hatch.size_x = 2.0f;
            hatch.size_y = 2.0f;
            hatch.offset_x = -0.5f;
            hatch.offset_y = -1.0f;
            hatch.opaque_x0 = 0.25f;
            hatch.opaque_x1 = 0.75f;
            hatch.opaque_y0 = 0.5f;
            hatch.opaque_y1 = 1.0f;
            hatch.quarter_turns = rotation;
            hatch.flip = flip;
            const auto bounds = r3d::get_sprite_opaque_bounds(hatch);
            CHECK_FALSE(bounds.has_trim);
            CHECK(bounds.local_r_min == Approx(-0.5f));
            CHECK(bounds.local_r_max == Approx(0.5f));
            CHECK(bounds.local_f_min == Approx(-0.5f));
            CHECK(bounds.local_f_max == Approx(0.5f));
            hatch.size_x = 1.0f;
            hatch.size_y = 1.5f;
            hatch.offset_x = 0.0f;
            hatch.offset_y = -0.5f;
            hatch.opaque_x0 = 0.0f;
            hatch.opaque_x1 = 1.0f;
            hatch.opaque_y0 = 1.0f / 3.0f;
            CHECK_FALSE(r3d::get_sprite_opaque_bounds(hatch).has_trim);
        }
    }
}

TEST_CASE("r3d_oversized_vehicle_art_keeps_native_mount_dimensions", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    for (auto x = 0; x < 2; ++x) {
        REQUIRE(
            veh->install_part(tripoint_mnt_veh(x, 0, 0), vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh->install_part(tripoint_mnt_veh(x, 0, 0), vpart_id("hatch"), true) >= 0);
    }
    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = {}};
    const auto mesh = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(mesh.empty());
    // The two native mounts occupy [0,2] x [0,1] around the half-tile pivot.
    // Transparent pixels in an elevation must not narrow their shared body envelope.
    for (const auto y : {0.0f, -1.0f}) {
        CAPTURE(y);
        CHECK(std::ranges::any_of(mesh.vertices, [&](const auto& v) { return v.y == Approx(y); }));
    }
    for (const auto x : {0.0f, 1.0f, 2.0f}) {
        CAPTURE(x);
        CHECK(std::ranges::any_of(mesh.vertices, [&](const auto& v) { return v.x == Approx(x); }));
    }
}

#endif
