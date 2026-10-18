#include "catch/catch.hpp"
#include "r3d_shapes.h"
#include "r3d_vehicle_rules.h"
#include "vehicle/veh_type.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <span>
#include <string_view>

TEST_CASE("r3d_native_quarterpanels_stay_half_height_under_roofs", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("halfboard_nw"), true) >= 0);
    auto profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.height == r3d::vehicle_body_h);
    CHECK_FALSE(profile.is_tall_cabin);
    CHECK(profile.corner == r3d::panel_corner::front_left);
    REQUIRE(veh.install_part(mount, vpart_id("roof"), true) >= 0);
    profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.has_roof);
    CHECK(profile.height == r3d::vehicle_body_h);
    CHECK(profile.accessory_part_idx == -1);
    REQUIRE(veh.install_part(mount, vpart_id("solar_panel"), true) >= 0);
    profile = r3d::vehicle_profile(veh, mount);
    REQUIRE(profile.accessory_part_idx >= 0);
    CHECK(veh.part_info(profile.accessory_part_idx).has_flag("SOLAR_PANEL"));
}

TEST_CASE("r3d_full_panels_and_windshields_keep_cabin_height", "[r3d][vehicle]") {
    for (const auto id : {"board_horizontal", "windshield_horizontal_front"}) {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh.install_part(mount, vpart_id(id), true) >= 0);
        CHECK(r3d::vehicle_profile(veh, mount).height == r3d::vehicle_cabin_h);
    }
}

TEST_CASE("r3d_native_panel_corner_shapes_clip_the_matching_corner", "[r3d][vehicle]") {
    for (const auto id : {"halfboard_nw", "halfboard_ne", "halfboard_sw", "halfboard_se"}) {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh.install_part(mount, vpart_id(id), true) >= 0);
        const auto profile = r3d::vehicle_profile(veh, mount);
        const auto outline = r3d::panel_outline(
            {.corner = profile.corner, .front = 0.5f, .rear = -0.5f, .left = -0.5f, .right = 0.5f});
        REQUIRE(outline.size() == 5);
        const auto front =
            std::string_view(id).ends_with("_nw") || std::string_view(id).ends_with("_ne");
        const auto left =
            std::string_view(id).ends_with("_nw") || std::string_view(id).ends_with("_sw");
        const auto removed = std::array<float, 2>{front ? 0.5f : -0.5f, left ? -0.5f : 0.5f};
        CHECK(std::ranges::find(outline, removed) == outline.end());
        const auto area = [&] {
            auto sum = 0.0f;
            for (auto i = size_t{0}; i < outline.size(); ++i) {
                const auto& a = outline[i];
                const auto& b = outline[(i + 1) % outline.size()];
                sum += a[0] * b[1] - b[0] * a[1];
            }
            return std::abs(sum) * 0.5f;
        }();
        CHECK(area == Approx(0.955f));
    }
}

#if defined(TILES)
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"

TEST_CASE("r3d_visible_vehicle_retains_parts_with_unlit_mounts", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    for (auto x = 0; x < 3; ++x) {
        REQUIRE(
            veh->install_part(tripoint_mnt_veh(x, 0, 0), vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh->install_part(tripoint_mnt_veh(x, 0, 0), vpart_id("halfboard_horizontal"), true)
                >= 0);
    }
    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); }};
    const auto full = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(full.empty());
    view.light = [&origin](const tripoint_bub_ms& p) {
        return p == origin ? r3d::pack_light(255, 255, 255) : r3d::pack_light(0, 0, 0);
    };
    const auto partial = r3d::build_vehicle_mesh(view);
    CHECK(partial.vertices.size() == full.vertices.size());
    CHECK(partial.indices == full.indices);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("roof"), true) >= 0);
    const auto roofed = r3d::build_vehicle_mesh(view);
    const auto& roof_art =
        cache.named({.category = C_VEHICLE_PART, .id = "vp_roof", .tiles = *tilecontext}).main();
    REQUIRE(roof_art.valid);
    auto placeholder_sheet_vertices = std::set<uint32_t>{};
    for (const auto& batch : roofed.batches) {
        if (batch.sheet == roof_art.sheet) {
            for (const auto index :
                 std::span(roofed.indices).subspan(batch.first_index, batch.index_count)) {
                placeholder_sheet_vertices.insert(index);
            }
        }
    }
    auto roof_quads = 0;
    for (auto i = size_t{0}; i + 3 < roofed.vertices.size(); i += 4) {
        const auto& v = roofed.vertices[i];
        if (std::abs(v.z - (r3d::floor_height + 0.02f + r3d::vehicle_cabin_h)) < 0.001f) {
            ++roof_quads;
        }
        // No quad may sample the missing-art roof placeholder over the body cap.
        auto placeholder = placeholder_sheet_vertices.contains(i);
        for (auto j = size_t{0}; j < 4; ++j) {
            const auto& corner = roofed.vertices[i + j];
            placeholder &=
                corner.u >= roof_art.u0 && corner.u <= roof_art.u1 && corner.v >= roof_art.v0
                && corner.v <= roof_art.v1 && corner.z > r3d::vehicle_cabin_h;
        }
        CHECK_FALSE(placeholder);
    }
    CHECK(roof_quads > 0);
    view.light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(0, 0, 0); };
    CHECK(r3d::build_vehicle_mesh(view).empty());
}
#endif
