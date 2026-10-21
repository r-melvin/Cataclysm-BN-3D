#include "catch/catch.hpp"
#include "r3d_shapes.h"
#include "r3d_vehicle_rules.h"
#include "vehicle/veh_type.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <ranges>
#include <set>
#include <span>
#include <string_view>
#include <vector>

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

TEST_CASE("r3d_vehicle_wheel_outline_is_round_and_faceted", "[r3d][vehicle]") {
    const auto outline = r3d::wheel_outline(
        {.segments = 12,
         .center_forward = 0.25f,
         .center_z = 0.4f,
         .radius_forward = 0.29f,
         .radius_z = 0.17f});
    REQUIRE(outline.size() == 12);
    CHECK(outline.front()[0] == Approx(0.54f));
    CHECK(outline.front()[1] == Approx(0.4f));
    CHECK(outline[3][0] == Approx(0.25f));
    CHECK(outline[3][1] == Approx(0.57f));
    for (const auto& point : outline) {
        const auto f = (point[0] - 0.25f) / 0.29f;
        const auto z = (point[1] - 0.4f) / 0.17f;
        CHECK(f * f + z * z == Approx(1.0f));
    }
    CHECK(
        r3d::wheel_outline(
            {.segments = 2,
             .center_forward = 0.0f,
             .center_z = 0.0f,
             .radius_forward = 1.0f,
             .radius_z = 1.0f})
            .size()
        == 3);
}

TEST_CASE("r3d_vehicle_profile_preserves_semantic_part_layers", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("halfboard_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("wheel_mount_medium"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("wheel"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("roof"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("solar_panel"), true) >= 0);

    const auto profile = r3d::vehicle_profile(veh, mount);
    const auto has_role = [&profile](const r3d::vehicle_render_role role) {
        return std::ranges::any_of(profile.layers, [role](const auto& layer) {
            return std::ranges::find(layer.roles, role) != layer.roles.end();
        });
    };

    CHECK(has_role(r3d::vehicle_render_role::frame));
    CHECK(has_role(r3d::vehicle_render_role::body_panel));
    CHECK(has_role(r3d::vehicle_render_role::wheel));
    CHECK(has_role(r3d::vehicle_render_role::roof));
    CHECK(has_role(r3d::vehicle_render_role::roof_accessory));
    CHECK(std::ranges::all_of(profile.layers, [](const auto& layer) {
        return !layer.open && !layer.broken;
    }));
}

TEST_CASE("r3d_broken_wheels_still_produce_a_wheel", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(mount, vpart_id("wheel_mount_medium"), true) >= 0);
    const auto wheel = veh.install_part(mount, vpart_id("wheel"), true);
    REQUIRE(wheel >= 0);
    veh.set_hp(veh.part(wheel), 0);
    REQUIRE(veh.cpart(wheel).is_broken());
    const auto profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.has_wheel);
    CHECK(std::ranges::any_of(profile.layers, [wheel](const auto& layer) {
        return layer.part_idx == wheel && layer.broken;
    }));
}

TEST_CASE("r3d_wheel_layer_prefers_intact_over_broken", "[r3d][vehicle]") {
    const auto wheel_layer = [](const int idx, const bool broken) {
        return r3d::vehicle_render_layer{
            .part_idx = idx, .roles = {r3d::vehicle_render_role::wheel}, .broken = broken};
    };
    const auto detail =
        r3d::vehicle_render_layer{.part_idx = 9, .roles = {r3d::vehicle_render_role::detail}};
    CHECK(r3d::preferred_wheel_layer(std::vector<r3d::vehicle_render_layer>{}) == nullptr);
    CHECK(r3d::preferred_wheel_layer(std::vector{detail}) == nullptr);
    const auto broken_first = std::vector{detail, wheel_layer(1, true), wheel_layer(2, false)};
    REQUIRE(r3d::preferred_wheel_layer(broken_first) != nullptr);
    CHECK(r3d::preferred_wheel_layer(broken_first)->part_idx == 2);
    const auto only_broken = std::vector{wheel_layer(3, true)};
    REQUIRE(r3d::preferred_wheel_layer(only_broken) != nullptr);
    CHECK(r3d::preferred_wheel_layer(only_broken)->part_idx == 3);
}

TEST_CASE("r3d_roof_cap_sits_on_the_wall_top", "[r3d][vehicle]") {
    // Half-height panels and frames keep their wall top; the roof rests on it.
    CHECK(r3d::roof_cap_height(r3d::vehicle_body_h) == Approx(r3d::vehicle_body_h));
    CHECK(r3d::roof_cap_height(r3d::vehicle_cabin_h) == Approx(r3d::vehicle_cabin_h));
    // Wall-less floor cells have nothing to rest on and keep the cabin roof line.
    CHECK(r3d::roof_cap_height(r3d::chassis_floor_h) == Approx(r3d::vehicle_cabin_h));
}

TEST_CASE("r3d_shaped_panel_outline_edges_are_classified_by_cell_side", "[r3d][vehicle]") {
    for (const auto corner :
         {r3d::panel_corner::front_left, r3d::panel_corner::front_right,
          r3d::panel_corner::rear_left, r3d::panel_corner::rear_right}) {
        const auto outline = r3d::panel_outline(
            {.corner = corner, .front = 0.5f, .rear = -0.5f, .left = -0.5f, .right = 0.5f});
        auto counts = std::map<r3d::outline_edge, int>{};
        for (auto i = size_t{0}; i < outline.size(); ++i) {
            ++counts[r3d::classify_outline_edge(
                {.a = outline[i],
                 .b = outline[(i + 1) % outline.size()],
                 .front = 0.5f,
                 .rear = -0.5f,
                 .left = -0.5f,
                 .right = 0.5f})];
        }
        CHECK(counts[r3d::outline_edge::chamfer] == 1);
        CHECK(counts[r3d::outline_edge::front] == 1);
        CHECK(counts[r3d::outline_edge::rear] == 1);
        CHECK(counts[r3d::outline_edge::left] == 1);
        CHECK(counts[r3d::outline_edge::right] == 1);
    }
}

TEST_CASE("r3d_only_floored_vehicle_tiles_raise_the_item_surface", "[r3d][vehicle]") {
    const auto height_with = [](const std::vector<const char*>& ids) {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        for (const auto id : ids) { REQUIRE(veh.install_part(mount, vpart_id(id), true) >= 0); }
        return r3d::vehicle_surface_height(veh, mount);
    };
    CHECK(height_with({"frame_horizontal"}) == Approx(0.0f));
    CHECK(height_with({"frame_horizontal", "wheel_mount_medium", "wheel"}) == Approx(0.0f));
    CHECK(height_with({"frame_horizontal", "seat"}) == Approx(r3d::chassis_floor_h));
    CHECK(height_with({"frame_horizontal", "aisle_horizontal"}) == Approx(r3d::chassis_floor_h));
}

namespace {
auto install_all(vehicle& veh, const tripoint_mnt_veh& mount, const std::vector<const char*>& ids)
    -> std::vector<int> {
    auto indices = std::vector<int>{};
    for (const auto id : ids) {
        const auto index = veh.install_part(mount, vpart_id(id), true);
        REQUIRE(index >= 0);
        indices.push_back(index);
    }
    return indices;
}

auto has_layer_role(
    const r3d::vehicle_render_profile& profile, const int part_idx,
    const r3d::vehicle_render_role role) -> bool {
    return std::ranges::any_of(profile.layers, [&](const auto& layer) {
        return layer.part_idx == part_idx
            && std::ranges::find(layer.roles, role) != layer.roles.end();
    });
}
} // namespace

TEST_CASE("r3d_open_door_stops_being_a_tall_wall", "[r3d][vehicle]") {
    const auto id =
        GENERATE("door", "door_opaque", "door_sliding", "door_trunk", "hatch", "hatch_opaque");
    CAPTURE(id);
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    const auto parts = install_all(veh, mount, {"frame_horizontal", id});
    const auto door = parts[1];
    REQUIRE(veh.part_info(door).has_flag("OPENABLE"));

    auto profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.is_openable);
    CHECK(profile.is_tall_cabin);
    CHECK(profile.height == r3d::vehicle_cabin_h);
    CHECK(has_layer_role(profile, door, r3d::vehicle_render_role::openable));
    CHECK(has_layer_role(profile, door, r3d::vehicle_render_role::door));
    CHECK(profile.is_door);
    // BOARDABLE on a closed door describes game movement, not an exposed interior floor.
    CHECK_FALSE(has_layer_role(profile, door, r3d::vehicle_render_role::interior));
    CHECK_FALSE(profile.is_interior);
    // Door pockets are real cargo: their contents still rest on the chassis floor.
    // Sliding and trunk doors have no cargo or floor of their own.
    const auto closed_surface =
        id == std::string_view("door_sliding") || id == std::string_view("door_trunk")
            ? 0.0f
            : r3d::chassis_floor_h;
    CHECK(r3d::vehicle_surface_height(veh, mount) == Approx(closed_surface));

    veh.part(door).open = true;
    profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.is_openable);
    CHECK(profile.is_interior);
    CHECK_FALSE(profile.is_tall_cabin);
    CHECK(profile.height == r3d::chassis_floor_h);
    CHECK(has_layer_role(profile, door, r3d::vehicle_render_role::interior));
    CHECK(std::ranges::any_of(profile.layers, [door](const auto& layer) {
        return layer.part_idx == door && layer.open;
    }));
    CHECK(r3d::vehicle_surface_height(veh, mount) == Approx(r3d::chassis_floor_h));
}

TEST_CASE("r3d_closed_door_preserves_an_independent_floor_on_its_mount", "[r3d][vehicle]") {
    const auto floor = GENERATE("seat", "aisle_horizontal", "trunk_flat");
    CAPTURE(floor);
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    const auto parts = install_all(veh, mount, {"frame_horizontal", floor, "door_sliding"});
    const auto door = parts.back();
    REQUIRE_FALSE(veh.cpart(door).open);
    const auto profile = r3d::vehicle_profile(veh, mount);
    CHECK_FALSE(has_layer_role(profile, door, r3d::vehicle_render_role::interior));
    CHECK((
        has_layer_role(profile, parts[1], r3d::vehicle_render_role::interior) || profile.is_cargo));
    CHECK(profile.is_tall_cabin);
    CHECK(profile.height == Approx(r3d::vehicle_cabin_h));
    CHECK(r3d::vehicle_surface_height(veh, mount) == Approx(r3d::chassis_floor_h));
}

TEST_CASE("r3d_door_role_covers_real_doors_but_not_window_coverings", "[r3d][vehicle]") {
    const auto check_door_role = [](const std::vector<const char*>& ids, const bool expected) {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        const auto parts = install_all(veh, mount, ids);
        for (const auto open : {false, true}) {
            CAPTURE(open);
            veh.part(parts.back()).open = open;
            const auto profile = r3d::vehicle_profile(veh, mount);
            CHECK(
                has_layer_role(profile, parts.back(), r3d::vehicle_render_role::door) == expected);
            CHECK(profile.is_door == expected);
        }
        veh.set_hp(veh.part(parts.back()), 0);
        REQUIRE(veh.cpart(parts.back()).is_broken());
        const auto broken = r3d::vehicle_profile(veh, mount);
        CHECK(has_layer_role(broken, parts.back(), r3d::vehicle_render_role::door) == expected);
        CHECK_FALSE(broken.is_door);
    };
    for (const auto id :
         {"door", "door_opaque", "door_sliding", "door_trunk", "hatch", "hatch_opaque",
          "door_internal", "door_shutter"}) {
        CAPTURE(id);
        check_door_role({"frame_horizontal", id}, true);
    }
    for (const auto id : {"v_curtain", "v_shutter"}) {
        CAPTURE(id);
        check_door_role({"frame_horizontal", "windshield_horizontal_front", id}, false);
    }
    check_door_role({"frame_horizontal", "windshield_horizontal_front"}, false);
}

TEST_CASE("r3d_broken_non_wheel_parts_keep_layers_but_drop_profile_flags", "[r3d][vehicle]") {
    const auto break_part = [](vehicle& veh, const int idx) {
        veh.set_hp(veh.part(idx), 0);
        REQUIRE(veh.cpart(idx).is_broken());
    };
    SECTION("windshield") {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        const auto parts =
            install_all(veh, mount, {"frame_horizontal", "windshield_horizontal_front"});
        REQUIRE(r3d::vehicle_profile(veh, mount).is_windshield);
        break_part(veh, parts[1]);
        const auto profile = r3d::vehicle_profile(veh, mount);
        CHECK_FALSE(profile.is_windshield);
        CHECK_FALSE(profile.is_tall_cabin);
        CHECK(profile.height == r3d::vehicle_body_h);
        CHECK(std::ranges::any_of(profile.layers, [&](const auto& layer) {
            return layer.part_idx == parts[1] && layer.broken;
        }));
    }
    SECTION("door") {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        const auto parts = install_all(veh, mount, {"frame_horizontal", "door"});
        break_part(veh, parts[1]);
        const auto profile = r3d::vehicle_profile(veh, mount);
        CHECK_FALSE(profile.is_openable);
        CHECK_FALSE(profile.is_tall_cabin);
        CHECK(has_layer_role(profile, parts[1], r3d::vehicle_render_role::openable));
    }
    SECTION("roof and shaped panel") {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        const auto parts = install_all(veh, mount, {"frame_horizontal", "halfboard_nw", "roof"});
        REQUIRE(r3d::vehicle_profile(veh, mount).has_roof);
        break_part(veh, parts[2]);
        break_part(veh, parts[1]);
        const auto profile = r3d::vehicle_profile(veh, mount);
        CHECK_FALSE(profile.has_roof);
        CHECK_FALSE(profile.show_roof);
        CHECK(profile.roof_part_idx == -1);
        CHECK(profile.corner == r3d::panel_corner::none);
    }
    SECTION("seat") {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        const auto parts = install_all(veh, mount, {"frame_horizontal", "seat"});
        REQUIRE(r3d::vehicle_profile(veh, mount).is_interior);
        break_part(veh, parts[1]);
        CHECK_FALSE(r3d::vehicle_profile(veh, mount).is_interior);
        CHECK(r3d::vehicle_surface_height(veh, mount) == Approx(0.0f));
    }
}

TEST_CASE("r3d_broken_wheel_does_not_hide_intact_wheel_on_same_mount", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    const auto parts = install_all(veh, mount, {"frame_horizontal", "wheel_mount_medium", "wheel"});
    veh.set_hp(veh.part(parts[2]), 0);
    const auto profile = r3d::vehicle_profile(veh, mount);
    CHECK(profile.has_wheel);
    const auto* const layer = r3d::preferred_wheel_layer(profile.layers);
    REQUIRE(layer != nullptr);
    CHECK(layer->part_idx == parts[2]);
    CHECK(layer->broken);
}

TEST_CASE(
    "r3d_several_panels_on_one_mount_choose_a_corner_independent_of_install_order",
    "[r3d][vehicle]") {
    struct panel_example {
        const char* id;
        r3d::panel_corner corner;
    };
    // The profile can represent one corner: use NW, NE, SW, SE priority for conflicts.
    const auto panels = std::array{
        panel_example{.id = "halfboard_nw", .corner = r3d::panel_corner::front_left},
        panel_example{.id = "halfboard_ne", .corner = r3d::panel_corner::front_right},
        panel_example{.id = "halfboard_sw", .corner = r3d::panel_corner::rear_left},
        panel_example{.id = "halfboard_se", .corner = r3d::panel_corner::rear_right}};
    for (auto first = size_t{0}; first < panels.size(); ++first) {
        for (auto second = first + 1; second < panels.size(); ++second) {
            for (const auto reverse : {false, true}) {
                CAPTURE(panels[first].id, panels[second].id, reverse);
                auto veh = vehicle(vproto_id("none"), 0, 0);
                const auto mount = tripoint_mnt_veh::zero();
                const auto parts = install_all(
                    veh, mount,
                    {"frame_horizontal", panels[reverse ? second : first].id,
                     panels[reverse ? first : second].id});
                CHECK(r3d::vehicle_profile(veh, mount).corner == panels[first].corner);
                // Broken panels keep their sprite layer but cannot override an intact corner.
                const auto preferred = parts[reverse ? 2 : 1];
                veh.set_hp(veh.part(preferred), 0);
                REQUIRE(veh.cpart(preferred).is_broken());
                CHECK(r3d::vehicle_profile(veh, mount).corner == panels[second].corner);
            }
        }
    }
}

TEST_CASE("r3d_unshaped_panels_do_not_erase_an_authored_corner", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    install_all(veh, mount, {"frame_horizontal", "halfboard_se", "halfboard_horizontal"});
    CHECK(r3d::vehicle_profile(veh, mount).corner == r3d::panel_corner::rear_right);
}

TEST_CASE("r3d_main_part_is_the_highest_z_order_below_nine", "[r3d][vehicle]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto mount = tripoint_mnt_veh::zero();
    const auto parts = install_all(veh, mount, {"frame_horizontal", "halfboard_horizontal"});
    auto profile = r3d::vehicle_profile(veh, mount);
    REQUIRE(profile.main_part_idx >= 0);
    auto expected = -1;
    for (const auto idx : parts) {
        if (veh.part_info(idx).z_order < 9
            && (expected < 0 || veh.part_info(idx).z_order > veh.part_info(expected).z_order)) {
            expected = idx;
        }
    }
    CHECK(profile.main_part_idx == expected);
    CHECK(veh.part_info(profile.main_part_idx).z_order < 9);
    for (const auto& layer : profile.layers) {
        if (veh.part_info(layer.part_idx).z_order < 9) {
            CHECK(veh.part_info(layer.part_idx).z_order
                  <= veh.part_info(profile.main_part_idx).z_order);
        }
    }
    // Parts with z_order 9 or more (roof accessories) never become the main part.
    install_all(veh, mount, {"roof", "solar_panel"});
    profile = r3d::vehicle_profile(veh, mount);
    REQUIRE(profile.main_part_idx >= 0);
    CHECK(veh.part_info(profile.main_part_idx).z_order < 9);
}

TEST_CASE("r3d_surface_height_follows_floor_parts_not_frames_or_panels", "[r3d][vehicle]") {
    const auto height_with = [](const std::vector<const char*>& ids) {
        auto veh = vehicle(vproto_id("none"), 0, 0);
        const auto mount = tripoint_mnt_veh::zero();
        install_all(veh, mount, ids);
        return r3d::vehicle_surface_height(veh, mount);
    };
    CHECK(height_with({"frame_horizontal", "halfboard_horizontal"}) == Approx(0.0f));
    CHECK(height_with({"frame_horizontal", "roof"}) == Approx(0.0f));
    CHECK(height_with({"frame_horizontal", "seat", "roof"}) == Approx(r3d::chassis_floor_h));
    CHECK(height_with({"frame_horizontal", "aisle_horizontal", "halfboard_horizontal"})
          == Approx(r3d::chassis_floor_h));
    CHECK(height_with({"frame_horizontal", "trunk_flat"}) == Approx(r3d::chassis_floor_h));
}

#if defined(TILES)
#    include "avatar.h"
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "game.h"
#    include "item.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "path_info.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"
#    include "vehicle/vpart_range.h"

#    include <cstdint>
#    include <fstream>
#    include <iterator>
#    include <string>
#    include <unordered_map>
#    include <vector>

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
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = {}};
    const auto full = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(full.empty());
    view.light = [&origin](const tripoint_bub_ms& p) {
        return p == origin ? r3d::pack_light(255, 255, 255) : r3d::pack_light(0, 0, 0);
    };
    const auto partial = r3d::build_vehicle_mesh(view);
    CHECK(partial.vertices.size() == full.vertices.size());
    CHECK(partial.indices == full.indices);
    // Unlit mounts are drawn dim, not with the light of the lit one.
    const auto rgb_count = [](const auto& lights, uint32_t color) {
        return std::ranges::count_if(lights, [color](uint32_t l) {
            return (l & 0x00ffffffu) == (color & 0x00ffffffu);
        });
    };
    const auto white = r3d::pack_light(255, 255, 255);
    CHECK(rgb_count(partial.light, white) < rgb_count(full.light, white));
    CHECK(rgb_count(partial.light, r3d::pack_light(70, 68, 74)) > 0);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("roof"), true) >= 0);
    const auto roofed = r3d::build_vehicle_mesh(view);
    const auto wheel_mount = tripoint_mnt_veh(1, 0, 0);
    REQUIRE(veh->install_part(wheel_mount, vpart_id("wheel_mount_medium"), true) >= 0);
    REQUIRE(veh->install_part(wheel_mount, vpart_id("wheel"), true) >= 0);
    CHECK(r3d::vehicle_profile(*veh, wheel_mount).has_wheel);
    const auto wheeled = r3d::build_vehicle_mesh(view);
    CHECK(wheeled.vertices.size() > roofed.vertices.size() + 48);
    const auto& wheel_art =
        cache.named({.category = C_VEHICLE_PART, .id = "vp_wheel", .tiles = *tilecontext}).main();
    REQUIRE(wheel_art.valid);
    CHECK(
        std::ranges::count_if(
            wheeled.vertices,
            [&wheel_art](const auto& vertex) {
                return vertex.u >= wheel_art.u0 && vertex.u <= wheel_art.u1
                    && vertex.v >= wheel_art.v0 && vertex.v <= wheel_art.v1;
            })
        >= 100);
    const auto vox_path = PATH_INFO::datadir() + "raw/r3d/vehicle_parts/wheel.vox";
    auto vox_input = std::ifstream(vox_path, std::ios::binary);
    REQUIRE(vox_input.good());
    const auto vox_bytes = std::vector<uint8_t>(std::istreambuf_iterator<char>(vox_input), {});
    const auto vox_model = r3d::read_vox_model(vox_bytes);
    REQUIRE(vox_model.has_value());
    const auto vox_surface = r3d::extract_vox_surface(*vox_model);
    REQUIRE(vox_surface.has_value());
    auto vox_palette = r3d::sprite_ref{};
    vox_palette.valid = true;
    const auto wheel_models = std::unordered_map<std::string, r3d::vox_asset>{
        {"wheel", {.model = *vox_model, .surface = *vox_surface, .palette = vox_palette}}};
    view.vehicle_vox_parts = &wheel_models;
    const auto voxel_wheeled = r3d::build_vehicle_mesh(view);
    CHECK(voxel_wheeled.vertices.size() > wheeled.vertices.size());
    CHECK(voxel_wheeled.indices.size() > wheeled.indices.size());
    // The voxel wheel's own vertices (those the sprite wheel mesh lacks) stay inside the wheel's
    // extent: a lone centreline wheel is 0.58 long and 0.26 wide, centred in the cell at mount x=1,
    // and as tall as vehicle_wheel_h from the chassis base.
    auto known = std::set<std::array<float, 3>>{};
    for (const auto& v : wheeled.vertices) { known.insert({v.x, v.y, v.z}); }
    auto low = std::array<float, 3>{1e9f, 1e9f, 1e9f};
    auto high = std::array<float, 3>{-1e9f, -1e9f, -1e9f};
    for (const auto& v : voxel_wheeled.vertices) {
        if (known.contains({v.x, v.y, v.z})) { continue; }
        const auto position = std::array<float, 3>{v.x, v.y, v.z};
        for (auto axis = 0; axis < 3; ++axis) {
            low[axis] = std::min(low[axis], position[axis]);
            high[axis] = std::max(high[axis], position[axis]);
        }
    }
    constexpr auto eps = 0.001f;
    constexpr auto base = r3d::floor_height + 0.02f;
    REQUIRE(high[0] >= low[0]);
    CHECK(low[0] >= 1.5f - 0.29f - eps);
    CHECK(high[0] <= 1.5f + 0.29f + eps);
    CHECK(high[0] - low[0] > 0.45f);
    CHECK(low[1] >= -0.5f - 0.13f - eps);
    CHECK(high[1] <= -0.5f + 0.13f + eps);
    CHECK(high[1] - low[1] > 0.2f);
    CHECK(low[2] >= base - eps);
    CHECK(high[2] <= base + r3d::vehicle_wheel_h + eps);
    CHECK(high[2] - low[2] > 0.28f);
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
        if (std::abs(v.z - (r3d::floor_height + 0.02f + r3d::vehicle_body_h)) < 0.001f) {
            ++roof_quads;
        }
        // No quad may sample the missing-art roof placeholder over the body cap.
        auto placeholder = placeholder_sheet_vertices.contains(i);
        for (auto j = size_t{0}; j < 4; ++j) {
            const auto& corner = roofed.vertices[i + j];
            placeholder &=
                corner.u >= roof_art.u0 && corner.u <= roof_art.u1 && corner.v >= roof_art.v0
                && corner.v <= roof_art.v1 && corner.z > r3d::vehicle_body_h;
        }
        CHECK_FALSE(placeholder);
    }
    CHECK(roof_quads > 0);
    // The roof rests on the half-height panel walls instead of floating at the cabin line.
    CHECK(std::ranges::none_of(roofed.vertices, [](const auto& vertex) {
        return std::abs(vertex.z - (r3d::floor_height + 0.02f + r3d::vehicle_cabin_h)) < 0.001f;
    }));
    view.light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(0, 0, 0); };
    CHECK(r3d::build_vehicle_mesh(view).empty());
}
TEST_CASE("r3d_adjacent_shaped_panels_skip_shared_side_walls", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    const auto mesh_vertices = [&](const int second_x) {
        auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
        REQUIRE(veh != nullptr);
        for (const auto x : {0, second_x}) {
            const auto mount = tripoint_mnt_veh(x, 0, 0);
            REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
            REQUIRE(veh->install_part(mount, vpart_id("halfboard_nw"), true) >= 0);
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
        const auto count = r3d::build_vehicle_mesh(view).vertices.size();
        here.destroy_vehicle(veh);
        return count;
    };
    const auto apart = mesh_vertices(3);
    const auto adjacent = mesh_vertices(1);
    REQUIRE(apart > 0);
    CHECK(adjacent < apart);
}
TEST_CASE(
    "r3d_live_vehicle_visibility_uses_perception_not_terrain_tint",
    "[r3d][vehicle][tiles][visibility]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    g->place_player(origin + point_west);
    auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    for (auto x = 0; x < 3; ++x) {
        REQUIRE(
            veh->install_part(tripoint_mnt_veh(x, 0, 0), vpart_id("frame_horizontal"), true) >= 0);
    }
    const auto cargo_index = veh->install_part(tripoint_mnt_veh(2, 0, 0), vpart_id("trunk"), true);
    REQUIRE(cargo_index >= 0);
    here.build_map_cache(0);
    here.update_visibility_cache(0);
    REQUIRE(here.get_visibility_variables_cache().variables_set);
    auto& perception = here.access_cache(0);
    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    auto tint = r3d::pack_light(255, 255, 255);
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [&tint](const tripoint_bub_ms& /*p*/) { return tint; },
        .markers = {},
        .uprights = {},
        .surface = {},
        .vehicle_mount_visible = [&here](const tripoint_bub_ms& p) { return here.pl_sees(p, -1); }};
    // Bright-only means the player detects light there, not the vehicle itself.
    for (const auto level :
         {lit_level::DARK, lit_level::MEMORIZED, lit_level::BLANK, lit_level::BRIGHT_ONLY}) {
        std::ranges::fill(perception.visibility_cache, level);
        for (const auto color :
             {r3d::pack_light(255, 255, 255), r3d::pack_light(205, 188, 160),
              r3d::pack_light(1, 1, 1)}) {
            tint = color;
            CHECK(r3d::build_vehicle_mesh(view).empty());
        }
    }
    std::ranges::fill(perception.visibility_cache, lit_level::DARK);
    perception.visibility_cache[perception.idx(origin.x(), origin.y())] = lit_level::LOW;
    // Perceived low-light geometry remains even if its presentation colour is black.
    tint = r3d::pack_light(0, 0, 0);
    const auto partial = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(partial.empty());
    std::ranges::fill(perception.visibility_cache, lit_level::LIT);
    const auto full = r3d::build_vehicle_mesh(view);
    CHECK(partial.vertices.size() == full.vertices.size());
    CHECK(partial.indices == full.indices);
    std::ranges::fill(perception.visibility_cache, lit_level::DARK);
    perception.visibility_cache[perception.idx(origin.x(), origin.y())] = lit_level::LOW;
    REQUIRE_FALSE(veh->add_item(cargo_index, item::spawn("rock")));
    // Contents cannot change the presentation of an unseen cargo mount.
    const auto hidden_cargo = r3d::build_vehicle_mesh(view);
    CHECK(hidden_cargo.vertices.size() == partial.vertices.size());
    CHECK(hidden_cargo.indices == partial.indices);
    const auto cargo_pos = veh->mount_to_bubble(tripoint_mnt_veh(2, 0, 0));
    perception.visibility_cache[perception.idx(cargo_pos.x(), cargo_pos.y())] = lit_level::LOW;
    // Without cargo highlight overlay, items in cargo do not add extra quads to the vehicle mesh.
    CHECK(r3d::build_vehicle_mesh(view).vertices.size() == hidden_cargo.vertices.size());
}

TEST_CASE("r3d_car_hatch_blueprint_rules", "[r3d][vehicle][tiles][car_hatch]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    g->place_player(origin + point_west);
    auto* const veh = here.add_vehicle(vproto_id("car_hatch"), origin, 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);

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
        .surface = {},
        .vehicle_mount_visible = [](const tripoint_bub_ms& /*p*/) { return true; }};

    const auto mesh = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(mesh.empty());
    REQUIRE(mesh.vertices.size() >= 4);

    // Verify presence of sloped quads (both z_low ~0.55f and z_high ~0.85f on same quad)
    auto has_sloped_windshield = false;
    auto has_sloped_hatch = false;
    auto has_hood_deck = false;
    auto has_rear_deck = false;
    auto front_wall_max_z = 0.0f;
    for (auto q = size_t{0}; q + 3 < mesh.vertices.size(); q += 4) {
        auto min_z = std::numeric_limits<float>::max();
        auto max_z = std::numeric_limits<float>::lowest();
        auto min_x = std::numeric_limits<float>::max();
        auto max_x = std::numeric_limits<float>::lowest();
        for (auto i = q; i < q + 4; ++i) {
            min_z = std::min(min_z, mesh.vertices[i].z);
            max_z = std::max(max_z, mesh.vertices[i].z);
            min_x = std::min(min_x, mesh.vertices[i].x);
            max_x = std::max(max_x, mesh.vertices[i].x);
        }
        const auto rel_min_x = min_x - 0.5f;
        const auto rel_max_x = max_x - 0.5f;
        // Sloped surface rising ~0.30f longitudinally between body_h and cabin_h
        if (std::abs(max_z - min_z - (r3d::vehicle_cabin_h - r3d::vehicle_body_h)) < 0.05f) {
            if (rel_min_x > 0.0f) {
                has_sloped_windshield = true;
            } else if (rel_max_x < 0.0f) {
                has_sloped_hatch = true;
            }
        }
        // Horizontal deck at body_h
        const auto expected_deck_z = r3d::floor_height + 0.02f + r3d::vehicle_body_h;
        if (std::abs(max_z - min_z) < 0.01f && std::abs(min_z - expected_deck_z) < 0.05f) {
            if (rel_min_x > 1.0f) {
                has_hood_deck = true;
            } else if (rel_max_x < -1.0f) {
                has_rear_deck = true;
            }
        }
        // Vertical front bumper wall
        if (std::abs(max_x - min_x) < 0.02f && rel_max_x > 2.0f && max_z > min_z) {
            front_wall_max_z = std::max(front_wall_max_z, max_z);
        }
    }
    CHECK(has_sloped_windshield);
    CHECK(has_sloped_hatch);
    CHECK(has_hood_deck);
    CHECK(has_rear_deck);
    CHECK(front_wall_max_z >= (r3d::floor_height + 0.02f + r3d::vehicle_body_h) - 0.02f);
}
TEST_CASE("r3d_vehicle_solid_faces_sample_opaque_pixels", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("halfboard_nw"), true) >= 0);
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
    auto solid_faces = 0;
    for (auto q = size_t{0}; q + 3 < mesh.vertices.size(); q += 4) {
        const auto& first = mesh.vertices[q];
        auto constant = true;
        for (auto i = size_t{1}; i < 4; ++i) {
            constant =
                constant && mesh.vertices[q + i].u == first.u && mesh.vertices[q + i].v == first.v;
        }
        if (!constant) { continue; }
        ++solid_faces;
        auto matched = false;
        for (auto rotation = 0; rotation < 4; ++rotation) {
            const auto donor = tilecontext->get_sprite_draw_data(
                {.id = "vp_halfboard_nw",
                 .category = C_VEHICLE_PART,
                 .pos = tripoint_bub_ms(0, 0, 0),
                 .rotation = rotation,
                 .variant = ""});
            if (!donor || !donor->solid_uv || !donor->alpha_mask) { continue; }
            if (first.u != (*donor->solid_uv)[0] || first.v != (*donor->solid_uv)[1]) { continue; }
            auto sheet_matches = false;
            for (const auto& batch : mesh.batches) {
                for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
                    sheet_matches =
                        sheet_matches || (mesh.indices[i] == q && batch.sheet == donor->texture);
                }
            }
            if (!sheet_matches) { continue; }
            matched = true;
            const auto x = static_cast<int>(
                (first.u - donor->uv0.first) / (donor->uv1.first - donor->uv0.first)
                * donor->dimensions.w);
            const auto y = static_cast<int>(
                (first.v - donor->uv0.second) / (donor->uv1.second - donor->uv0.second)
                * donor->dimensions.h);
            REQUIRE(x >= 0);
            REQUIRE(y >= 0);
            REQUIRE(x < donor->dimensions.w);
            REQUIRE(y < donor->dimensions.h);
            CHECK((*donor->alpha_mask)[static_cast<size_t>(y * donor->dimensions.w + x)] != 0);
        }
        CHECK(matched);
    }
    CHECK(solid_faces > 0);
}

TEST_CASE("r3d_first_person_driver_sees_through_own_windshield", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    auto* const veh =
        here.add_vehicle(vproto_id("car"), origin + tripoint(4, 0, 0), 0_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    auto controls = veh->get_avail_parts("CONTROLS");
    REQUIRE(controls.part_count() > 0);
    const auto seat = (*controls.begin()).pos();
    here.board_vehicle(seat, &you);
    you.controlling_vehicle = true;
    REQUIRE(veh->player_in_control(you));

    auto cache = r3d::tile_info_cache{};
    const auto mesh_size = [&](const r3d::camera& cam) {
        const auto view = r3d::entity_view{
            .tiles = *tilecontext,
            .cache = cache,
            .cam = cam,
            .center = origin,
            .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
            .markers = {},
            .uprights = {},
            .surface = {}};
        return r3d::build_vehicle_mesh(view).indices.size();
    };
    auto fp_cam = r3d::camera{};
    fp_cam.mode = r3d::camera_mode::first_person;
    const auto orbit = mesh_size(r3d::camera{});
    const auto driving_fp = mesh_size(fp_cam);
    // Only the driver's own glass is dropped; the rest of the body stays.
    CHECK(driving_fp < orbit);
    CHECK(driving_fp > orbit / 2);
    you.controlling_vehicle = false;
    CHECK(mesh_size(fp_cam) == orbit);
}

#endif
