#include "catch/catch.hpp"
#include "r3d_shapes.h"
#include "r3d_vehicle_rules.h"
#include "r3d_vehicle_sections.h"
#include "r3d_vehicle_silhouette.h"
#include "vehicle/veh_type.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string_view>
#include <vector>

TEST_CASE(
    "r3d_vehicle_sections_ignore_accessories_and_unconnected_frames", "[r3d][vehicle][sections]") {
    auto veh = vehicle(vproto_id("car_hatch"), 0, 0);
    const auto original = r3d::describe_vehicle_sections(veh);
    REQUIRE(original.bodies.size() == 1);
    REQUIRE_FALSE(original.canopies.empty());
    const auto bounds = original.bodies.front().bounds;
    const auto anchor = original.canopies.front().mounts.front();
    const auto height = original.cells.at(anchor).height;
    const auto extra = tripoint_mnt_veh(bounds.front + 4, bounds.left - 5, 0);
    REQUIRE(veh.install_part(extra, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(extra, vpart_id("wing_mirror_left"), true) >= 0);
    const auto changed = r3d::describe_vehicle_sections(veh);
    REQUIRE(changed.bodies.size() == 1);
    CHECK(changed.bodies.front().bounds.front == bounds.front);
    CHECK(changed.bodies.front().bounds.rear == bounds.rear);
    CHECK(changed.bodies.front().bounds.left == bounds.left);
    CHECK(changed.bodies.front().bounds.right == bounds.right);
    CHECK(changed.cells.at(extra).body == -1);
    CHECK(changed.cells.at(anchor).height == Approx(height));
    // Opening a leaf changes its presentation state, not the body's cross-section.
    for (auto i = 0; i < veh.part_count(); ++i) {
        if (veh.part_info(i).location == "door") { veh.part(i).open = true; }
    }
    const auto opened = r3d::describe_vehicle_sections(veh);
    CHECK(opened.cells.at(anchor).height == Approx(height));
    CHECK(opened.bodies.front().bounds.left == bounds.left);
}

TEST_CASE(
    "r3d_vehicle_sections_separate_glazed_cab_and_enclosed_cargo", "[r3d][vehicle][sections]") {
    const auto veh = vehicle(vproto_id("cube_van"), 0, 0);
    const auto sections = r3d::describe_vehicle_sections(veh);
    const auto cab = sections.cells.at(tripoint_mnt_veh(0, 1, 0)).canopy;
    const auto cargo = sections.cells.at(tripoint_mnt_veh(-3, 1, 0)).canopy;
    REQUIRE(cab >= 0);
    REQUIRE(cargo >= 0);
    CHECK(cab != cargo);
    CHECK(sections.canopies[cab].glazed);
    CHECK_FALSE(sections.canopies[cargo].glazed);
    CHECK(sections.canopies[cargo].box);
    CHECK(sections.canopies[cargo].height > sections.canopies[cab].height);
    // This bulkhead belongs to the taller box, without joining both roof components.
    CHECK(sections.cells.at(tripoint_mnt_veh(-1, 1, 0)).height
          == Approx(sections.canopies[cargo].height));
}

TEST_CASE(
    "r3d_vehicle_sections_preserve_mount_levels_and_disconnected_bodies",
    "[r3d][vehicle][sections]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    for (const auto mount :
         {tripoint_mnt_veh(0, 0, 0), tripoint_mnt_veh(0, 0, 1), tripoint_mnt_veh(4, 0, 0)}) {
        REQUIRE(veh.install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh.install_part(mount, vpart_id("halfboard_horizontal"), true) >= 0);
    }
    const auto sections = r3d::describe_vehicle_sections(veh);
    CHECK(sections.bodies.size() == 3);
    CHECK(sections.cells.at(tripoint_mnt_veh(0, 0, 0)).body
          != sections.cells.at(tripoint_mnt_veh(0, 0, 1)).body);
    CHECK(sections.cells.at(tripoint_mnt_veh(0, 0, 0)).body
          != sections.cells.at(tripoint_mnt_veh(4, 0, 0)).body);
}

TEST_CASE(
    "r3d_section_shoulder_fit_has_a_shared_center_and_preserves_partition_edges",
    "[r3d][vehicle][sections]") {
    const auto fit = [](const float x) {
        return r3d::vehicle_section_inset(
            {.position = x, .left = 4.1f, .right = 6.9f, .amount = 0.24f});
    };
    CHECK(fit(5.5f) == Approx(5.5f));
    CHECK(fit(4.1f) == Approx(4.34f));
    CHECK(fit(6.9f) == Approx(6.66f));
    CHECK(fit(5.0f) + fit(6.0f) == Approx(11.0f));
    CHECK(fit(5.0f) < fit(5.5f));
}

TEST_CASE(
    "r3d_wheel_openings_follow_the_native_axle_and_leave_outer_wall_intact",
    "[r3d][vehicle][sections]") {
    const auto strips = r3d::vehicle_wheel_opening(
        {.from = -0.5f,
         .to = 0.5f,
         .bottom = 0.1f,
         .center = 0.0f,
         .center_z = 0.2f,
         .radius_forward = 0.3f,
         .radius_up = 0.18f});
    REQUIRE(strips.size() > 3);
    CHECK(strips.front().from == Approx(-0.5f));
    CHECK(strips.back().to == Approx(0.5f));
    CHECK(strips.front().bottom_from == Approx(0.1f));
    CHECK(strips.front().bottom_to == Approx(0.1f));
    CHECK(strips.back().bottom_from == Approx(0.1f));
    auto crest = 0.0f;
    for (auto i = size_t{0}; i < strips.size(); ++i) {
        const auto& strip = strips[i];
        CHECK(strip.to > strip.from);
        if (i > 0) { CHECK(strip.from == Approx(strips[i - 1].to)); }
        crest = std::max({crest, strip.bottom_from, strip.bottom_to});
        if ((strip.from + strip.to) * 0.5f > -0.3f && (strip.from + strip.to) * 0.5f < 0.3f) {
            for (const auto& point :
                 {std::array{strip.from, strip.bottom_from},
                  std::array{strip.to, strip.bottom_to}}) {
                const auto x = point[0] / 0.3f;
                const auto z = (point[1] - 0.2f) / 0.18f;
                CHECK(x * x + z * z == Approx(1.0f).margin(1e-4));
            }
        }
    }
    CHECK(crest == Approx(0.38f));
    const auto plain = r3d::vehicle_wheel_opening(
        {.from = -0.5f,
         .to = 0.5f,
         .bottom = 0.1f,
         .center = 0.0f,
         .center_z = 0.2f,
         .radius_forward = 0.0f,
         .radius_up = 0.18f});
    REQUIRE(plain.size() == 1);
    CHECK(plain.front().bottom_from == Approx(0.1f));
    CHECK(plain.front().bottom_to == Approx(0.1f));
}

TEST_CASE("r3d_cabin_proportions_follow_installed_parts", "[r3d][vehicle][silhouette]") {
    const auto sports = vehicle(vproto_id("car_sports"), 0, 0);
    const auto sedan = vehicle(vproto_id("car"), 0, 0);
    const auto suv = vehicle(vproto_id("suv"), 0, 0);
    const auto van = vehicle(vproto_id("cube_van"), 0, 0);
    const auto heli = vehicle(vproto_id("2seater2"), 0, 0);
    const auto sports_shape = r3d::describe_vehicle(sports);
    const auto sedan_shape = r3d::describe_vehicle(sedan);
    const auto suv_shape = r3d::describe_vehicle(suv);
    const auto van_shape = r3d::describe_vehicle(van);
    const auto heli_shape = r3d::describe_vehicle(heli);
    CHECK(sports_shape.enclosed);
    CHECK(sedan_shape.enclosed);
    CHECK(suv_shape.enclosed);
    CHECK(van_shape.enclosed);
    CHECK(sports_shape.cabin_height < sedan_shape.cabin_height);
    CHECK(sedan_shape.cabin_height < suv_shape.cabin_height);
    CHECK(suv_shape.cabin_height < van_shape.cabin_height);
    CHECK(heli_shape.rotorcraft);
    CHECK(heli_shape.cabin_rear == -2);
    CHECK(heli_shape.tail_rear == -5);
    CHECK(heli_shape.tail_center == Approx(0.5f));
    CHECK(heli_shape.main_rotors == 1);
    CHECK(heli_shape.rotor_forward == Approx(-1.0f));
    CHECK(heli_shape.rotor_right == Approx(0.5f));
    const auto tandem = vehicle(vproto_id("helicopter_osprey_2a"), 0, 0);
    CHECK(r3d::describe_vehicle(tandem).main_rotors == 2);
    CHECK_FALSE(sedan_shape.rotorcraft);
    // A roof on a custom open platform must not infer a complete enclosed cabin.
    auto platform = vehicle(vproto_id("none"), 0, 0);
    REQUIRE(
        platform.install_part(tripoint_mnt_veh::zero(), vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(platform.install_part(tripoint_mnt_veh::zero(), vpart_id("roof"), true) >= 0);
    CHECK_FALSE(r3d::describe_vehicle(platform).enclosed);
}

TEST_CASE("r3d_airframe_shapes_only_existing_aft_panels", "[r3d][vehicle][silhouette]") {
    auto heli = vehicle(vproto_id("2seater2"), 0, 0);
    const auto shape = r3d::describe_vehicle(heli);
    const auto section = [&](const tripoint_mnt_veh& mount) {
        return r3d::describe_airframe_section({.veh = heli, .silhouette = shape, .mount = mount});
    };
    const auto boom = tripoint_mnt_veh(-3, 0, 0);
    CHECK(section(boom) == r3d::airframe_section::boom);
    CHECK(section(tripoint_mnt_veh(-5, -1, 0)) == r3d::airframe_section::stabilizer);
    CHECK(section(tripoint_mnt_veh(-5, 0, 0)) == r3d::airframe_section::fin);
    CHECK(section(tripoint_mnt_veh(0, 0, 0)) == r3d::airframe_section::cabin);
    CHECK(section(tripoint_mnt_veh(-8, 0, 0)) == r3d::airframe_section::cabin);
    // Player-installed cargo keeps its native usable footprint rather than becoming a boom.
    REQUIRE(vpart_id("box").is_valid());
    REQUIRE(heli.install_part(boom, vpart_id("box"), true) >= 0);
    CHECK(section(boom) == r3d::airframe_section::cabin);
    // Road-vehicle quarterpanels do not acquire aircraft styling.
    const auto car = vehicle(vproto_id("car"), 0, 0);
    CHECK(
        r3d::describe_airframe_section(
            {.veh = car,
             .silhouette = r3d::describe_vehicle(car),
             .mount = tripoint_mnt_veh(-3, 0, 0)})
        == r3d::airframe_section::cabin);
}

TEST_CASE("r3d_tire_proportions_use_the_installed_wheel_dimensions", "[r3d][vehicle][silhouette]") {
    auto veh = vehicle(vproto_id("none"), 0, 0);
    const auto origin = tripoint_mnt_veh::zero();
    const auto other = tripoint_mnt_veh(1, 0, 0);
    REQUIRE(veh.install_part(origin, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh.install_part(other, vpart_id("frame_horizontal"), true) >= 0);
    const auto regular = veh.install_part(origin, vpart_id("wheel"), true);
    const auto wide = veh.install_part(other, vpart_id("wheel_wide"), true);
    REQUIRE(regular >= 0);
    REQUIRE(wide >= 0);
    const auto regular_shape = r3d::native_wheel_dimensions(veh.cpart(regular));
    const auto wide_shape = r3d::native_wheel_dimensions(veh.cpart(wide));
    CHECK(regular_shape.radius_forward
          == Approx(veh.cpart(regular).wheel_diameter() * 0.001f * 0.5f));
    CHECK(wide_shape.radius_forward > regular_shape.radius_forward);
    CHECK(wide_shape.width > regular_shape.width);
    CHECK(wide_shape.radius_forward == Approx(wide_shape.radius_up * 1.8f));
    veh.set_hp(veh.part(wide), 0);
    CHECK(
        r3d::native_wheel_dimensions(veh.cpart(wide)).radius_forward == wide_shape.radius_forward);
}

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
        REQUIRE(outline.size() == 8);
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
        CHECK(area == Approx(0.978883f));
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
        CHECK(counts[r3d::outline_edge::chamfer] == 4);
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
#    include "r3d_vehicle_lighting.h"
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
    // installed native dimensions, centred in the cell at mount x=1.
    const auto wheel_index =
        r3d::preferred_wheel_layer(r3d::vehicle_profile(*veh, wheel_mount).layers)->part_idx;
    const auto wheel_size = r3d::native_wheel_dimensions(veh->cpart(wheel_index));
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
    CHECK(low[0] >= 1.5f - wheel_size.radius_forward - eps);
    CHECK(high[0] <= 1.5f + wheel_size.radius_forward + eps);
    CHECK(high[0] - low[0] > wheel_size.radius_forward * 1.7f);
    CHECK(low[1] >= -0.5f - wheel_size.width * 0.5f - eps);
    CHECK(high[1] <= -0.5f + wheel_size.width * 0.5f + eps);
    CHECK(high[1] - low[1] > wheel_size.width * 0.8f);
    CHECK(low[2] >= base - eps);
    CHECK(high[2] <= base + wheel_size.radius_up * 2.0f + eps);
    CHECK(high[2] - low[2] > wheel_size.radius_up * 1.7f);
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

    const auto cabin_height = r3d::describe_vehicle(*veh).cabin_height;
    // Verify presence of sloped quads (both z_low ~0.55f and z_high ~0.85f on same quad)
    auto has_sloped_windshield = false;
    auto has_sloped_hatch = false;
    auto has_sloped_hood = false;
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
        if (std::abs(max_z - min_z - (cabin_height - r3d::vehicle_body_h)) < 0.05f) {
            if (rel_min_x > 0.0f) {
                has_sloped_windshield = true;
            } else if (rel_max_x < 0.0f) {
                has_sloped_hatch = true;
            }
        }
        const auto expected_deck_z = r3d::floor_height + 0.02f + r3d::vehicle_body_h;
        // The bonnet drops toward the nose and has a shallow transverse crown.
        if (rel_min_x > 1.0f && max_z > expected_deck_z - 0.02f && max_z <= expected_deck_z + 0.046f
            && min_z < max_z - 0.04f) {
            has_sloped_hood = true;
        }
        if (rel_max_x < -1.0f && std::abs(max_z - min_z) < 0.01f
            && std::abs(min_z - expected_deck_z) < 0.05f) {
            has_rear_deck = true;
        }
        // Vertical front bumper wall
        if (std::abs(max_x - min_x) < 0.02f && rel_max_x > 2.0f && max_z > min_z) {
            front_wall_max_z = std::max(front_wall_max_z, max_z);
        }
    }
    CHECK(has_sloped_windshield);
    CHECK(has_sloped_hatch);
    CHECK(has_sloped_hood);
    CHECK(has_rear_deck);
    CHECK(
        front_wall_max_z
        == Approx(r3d::floor_height + 0.02f + r3d::vehicle_body_h - 0.10f + 0.045f).margin(0.01f));
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

TEST_CASE("r3d_vehicle_mounts_keep_their_material_without_double_tint", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto panels = std::array{"halfboard_horizontal", "woodhalfboard_horizontal", "roof"};
    for (auto i = 0; i < static_cast<int>(panels.size()); ++i) {
        const auto mount = tripoint_mnt_veh(i, 0, 0);
        REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh->install_part(mount, vpart_id(panels[i]), true) >= 0);
    }
    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    const auto light = r3d::pack_light(220, 200, 180);
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [light](const tripoint_bub_ms& /*p*/) { return light; },
        .markers = {},
        .uprights = {},
        .surface = {}};
    const auto mesh = r3d::build_vehicle_mesh(view);
    const auto deck_height = r3d::floor_height + 0.02f + r3d::vehicle_body_h;
    for (auto i = 0; i < static_cast<int>(panels.size()); ++i) {
        CAPTURE(panels[i]);
        auto material =
            panels[i] == std::string_view("roof")
                ? cache.fixture_palette()
                : cache
                      .named({.category = C_VEHICLE_PART,
                              .id = std::string("vp_") + panels[i],
                              .tiles = *tilecontext})
                      .main();
        if (panels[i] == std::string_view("roof")) {
            material.solid_uv = std::array<float, 2>{1.5f / 7.0f, 0.5f};
        }
        REQUIRE(material.valid);
        REQUIRE(material.solid_uv);
        auto decks = 0;
        for (auto q = size_t{0}; q < mesh.vertices.size(); q += 4) {
            const auto quad = std::span(mesh.vertices).subspan(q, 4);
            const auto roof = panels[i] == std::string_view("roof");
            const auto projected_area =
                (quad[1].x - quad[0].x) * (quad[3].y - quad[0].y)
                - (quad[1].y - quad[0].y) * (quad[3].x - quad[0].x);
            if (std::abs(projected_area) < 1e-6f
                || !std::ranges::all_of(quad, [i, deck_height, roof](const auto& vertex) {
                       return vertex.x >= i && vertex.x <= i + 1
                           && (roof ? vertex.z >= deck_height - 1e-6f
                                          && vertex.z <= deck_height + 0.101f
                                    : vertex.z == Approx(deck_height));
                   })) {
                continue;
            }
            ++decks;
            for (auto c = size_t{0}; c < 4; ++c) {
                CHECK(quad[c].u == Approx((*material.solid_uv)[0]));
                CHECK(quad[c].v == Approx((*material.solid_uv)[1]));
                const auto actual = mesh.light[q + c];
                for (const auto shift : {0, 8, 16}) {
                    const auto channel = (actual >> shift) & 255u;
                    const auto source = (light >> shift) & 255u;
                    CHECK(channel <= source);
                    CHECK(channel >= static_cast<unsigned>(source * (roof ? 0.92f : 1.0f)));
                }
            }
            auto bound = false;
            for (const auto& batch : mesh.batches) {
                const auto indices =
                    std::span(mesh.indices).subspan(batch.first_index, batch.index_count);
                if (std::ranges::contains(indices, static_cast<uint32_t>(q))) {
                    CHECK(batch.sheet == material.sheet);
                    bound = true;
                }
            }
            CHECK(bound);
        }
        CHECK(decks > 0);
    }
}

TEST_CASE(
    "r3d_native_helicopter_rotors_extend_beyond_their_mount_and_keep_broken_hubs",
    "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto mount = tripoint_mnt_veh::zero();
    REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(mount, vpart_id("roof"), true) >= 0);
    const auto rotor = veh->install_part(mount, vpart_id("small_civilian_rotors"), true);
    REQUIRE(rotor >= 0);
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
    const auto intact = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(intact.vertices.empty());
    const auto span = [](const auto& mesh) {
        return std::ranges::max(mesh.vertices, {}, &r3d::vertex::x).x
             - std::ranges::min(mesh.vertices, {}, &r3d::vertex::x).x;
    };
    CHECK(span(intact) > 6.0f);
    CHECK(std::ranges::max(intact.vertices, {}, &r3d::vertex::z).z > r3d::vehicle_cabin_h + 0.2f);
    veh->set_hp(veh->part(rotor), 0);
    const auto broken = r3d::build_vehicle_mesh(view);
    CHECK(span(broken) < 2.0f);
    CHECK(std::ranges::max(broken.vertices, {}, &r3d::vertex::z).z > r3d::vehicle_cabin_h + 0.2f);
}

TEST_CASE(
    "r3d_roofs_match_adjacent_body_art_and_keep_distinct_native_paint", "[r3d][vehicle][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = here.add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto body_mount = tripoint_mnt_veh::zero();
    const auto roof_mount = tripoint_mnt_veh(0, 1, 0);
    REQUIRE(veh->install_part(body_mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(body_mount, vpart_id("halfboard_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(roof_mount, vpart_id("frame_horizontal"), true) >= 0);
    const auto roof_part = veh->install_part(roof_mount, vpart_id("roof"), true);
    REQUIRE(roof_part >= 0);
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
    const auto& body =
        cache
            .named(
                {.category = C_VEHICLE_PART, .id = "vp_halfboard_horizontal", .tiles = *tilecontext})
            .main();
    REQUIRE(body.solid_uv);
    const auto plain = r3d::build_vehicle_mesh(view);
    auto roof_uses_body = false;
    for (const auto& batch : plain.batches) {
        if (batch.sheet != body.sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& v = plain.vertices[plain.indices[i]];
            if (v.y < -1.0f && v.u == Approx((*body.solid_uv)[0])
                && v.v == Approx((*body.solid_uv)[1])) {
                roof_uses_body = true;
            }
        }
    }
    CHECK(roof_uses_body);
    auto red = RGBColor{};
    red.r = 240;
    red.g = 30;
    red.b = 20;
    red.a = 255;
    veh->part(roof_part).set_color(red, red);
    const auto painted = r3d::build_vehicle_mesh(view);
    auto explicit_paint = false;
    for (const auto& batch : painted.batches) {
        if (batch.sheet != cache.fixture_palette().sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto index = painted.indices[i];
            const auto& v = painted.vertices[index];
            if (v.y < -1.0f && v.u == Approx(6.5f / 7.0f) && (painted.light[index] & 255u) > 200u
                && ((painted.light[index] >> 8) & 255u) < 40u) {
                explicit_paint = true;
            }
        }
    }
    CHECK(explicit_paint);
}

TEST_CASE(
    "r3d_single_helicopter_rotors_center_over_cabin_and_turn_vertically_at_tail",
    "[r3d][vehicle][tiles][silhouette]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("2seater2"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto shape = r3d::describe_vehicle(*veh);
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
    const auto base = r3d::floor_height + 0.02f;
    const auto pivot = veh->pivot_anchor[0];
    const auto offset = veh->bub_ms_location() - origin;
    const auto expected_x = offset.x() + 0.5f + shape.rotor_forward - pivot.x();
    const auto expected_y = -(offset.y() + 0.5f + shape.rotor_right - pivot.y());
    auto mast_left = 1e9f;
    auto mast_right = -1e9f;
    auto mast_front = -1e9f;
    auto mast_back = 1e9f;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != cache.fixture_palette().sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& v = mesh.vertices[mesh.indices[i]];
            if (v.u != Approx(1.5f / 7.0f) || v.z != Approx(base + shape.cabin_height + 0.24f)) {
                continue;
            }
            mast_left = std::min(mast_left, v.y);
            mast_right = std::max(mast_right, v.y);
            mast_back = std::min(mast_back, v.x);
            mast_front = std::max(mast_front, v.x);
        }
    }
    REQUIRE(mast_left < mast_right);
    CHECK((mast_left + mast_right) * 0.5f == Approx(expected_y));
    CHECK((mast_back + mast_front) * 0.5f == Approx(expected_x));
    const auto tail_x = offset.x() + 0.5f + shape.tail_rear - pivot.x();
    struct tail_span {
        float low = 1e9f;
        float high = -1e9f;
        float bottom = 1e9f;
        float top = -1e9f;
    };
    const auto tail_bounds = [&](const auto& model) {
        auto bounds = tail_span{};
        for (const auto& batch : model.batches) {
            if (batch.sheet != cache.fixture_palette().sheet) { continue; }
            for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
                const auto& v = model.vertices[model.indices[i]];
                if (v.u != Approx(2.5f / 7.0f) || v.x < tail_x - 0.8f || v.x > tail_x + 0.8f
                    || v.z > base + shape.cabin_height + 0.24f) {
                    continue;
                }
                bounds.low = std::min(bounds.low, v.x);
                bounds.high = std::max(bounds.high, v.x);
                bounds.bottom = std::min(bounds.bottom, v.z);
                bounds.top = std::max(bounds.top, v.z);
            }
        }
        return bounds;
    };
    const auto tail = tail_bounds(mesh);
    REQUIRE(tail.low < tail.high);
    CHECK(tail.high - tail.low > 0.9f);
    CHECK(tail.top - tail.bottom > 0.2f);
    // Damage to existing fin panels shortens the visual tail blades to stubs.
    for (auto i = 0; i < veh->part_count(); ++i) {
        const auto& part = veh->cpart(i);
        if (part.mount.x() == shape.tail_rear && veh->part_flag(i, "HALF_BOARD")
            && !veh->part_info(i).id.str().ends_with("_cover")) {
            veh->set_hp(veh->part(i), 0);
        }
    }
    const auto broken = tail_bounds(r3d::build_vehicle_mesh(view));
    REQUIRE(broken.low < broken.high);
    CHECK(broken.high - broken.low < tail.high - tail.low);
}

TEST_CASE("r3d_rooftop_accessories_clear_the_faceted_roof", "[r3d][vehicle][tiles][silhouette]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("roof"), true) >= 0);
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
    const auto roof = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(roof.vertices.empty());
    const auto roof_top = std::ranges::max(roof.vertices, {}, &r3d::vertex::z).z;
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("solar_panel"), true) >= 0);
    const auto panel = r3d::build_vehicle_mesh(view);
    CHECK(panel.vertices.size() > roof.vertices.size());
    CHECK(std::ranges::max(panel.vertices, {}, &r3d::vertex::z).z >= roof_top + 0.002f);
}

TEST_CASE(
    "r3d_vehicle_native_glass_and_lamps_reach_their_exterior_faces",
    "[r3d][vehicle][tiles][materials]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    const auto tileset = GENERATE("UltimateCataclysm", "hoder");
    tilecontext->load_tileset(tileset, {}, false, true, false);
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    auto cache = r3d::tile_info_cache{};
    cache.set_presentation_profiles(r3d::load_presentation_profiles(input));
    const auto& glass_art = cache.named(
        {.category = C_VEHICLE_PART, .id = "vp_windshield_horizontal_front", .tiles = *tilecontext});
    REQUIRE(glass_art.glass.valid);
    const auto& lamp_art = cache.named(
        {.category = C_VEHICLE_PART, .id = "vp_halfboard_sw", .tiles = *tilecontext});
    REQUIRE(lamp_art.rear.valid);
    REQUIRE(lamp_art.cap.valid);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("car_hatch"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    for (auto i = 0; i < veh->part_count(); ++i) {
        veh->set_hp(veh->part(i), veh->part_info(i).durability);
    }
    const auto cam = r3d::camera{};
    const auto light = r3d::pack_light(220, 200, 180);
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [light](const tripoint_bub_ms& /*p*/) { return light; },
        .markers = {},
        .uprights = {},
        .surface = {}};
    const auto mesh = r3d::build_vehicle_mesh(view);
    auto glass_faces = 0;
    auto lamp_faces = 0;
    for (const auto& batch : mesh.batches) {
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; i += 6) {
            const auto index = mesh.indices[i];
            const auto quad = std::span(mesh.vertices).subspan(index, 4);
            const auto uses = [&](const r3d::sprite_ref& source) {
                return batch.sheet == source.sheet
                    && std::ranges::all_of(
                           quad,
                           [&source](const auto& v) {
                               return v.u >= source.u0 - 1e-6f && v.u <= source.u1 + 1e-6f
                                   && v.v >= source.v0 - 1e-6f && v.v <= source.v1 + 1e-6f;
                           })
                    && std::ranges::any_of(quad, [&quad](const auto& v) {
                           return std::abs(v.u - quad[0].u) > 1e-6f;
                       });
            };
            if (uses(glass_art.glass)) {
                ++glass_faces;
                CHECK(std::ranges::max(quad, {}, &r3d::vertex::z).z
                      > std::ranges::min(quad, {}, &r3d::vertex::z).z);
                for (auto c = 0; c < 4; ++c) {
                    const auto color = mesh.light[index + c];
                    const auto red = (color & 255u) / 220.0f;
                    const auto green = ((color >> 8) & 255u) / 200.0f;
                    const auto blue = ((color >> 16) & 255u) / 180.0f;
                    CHECK(red >= 0.64f);
                    CHECK(red <= 1.0f);
                    CHECK(red == Approx(green).margin(0.01f));
                    CHECK(red == Approx(blue).margin(0.01f));
                }
            }
            if (uses(lamp_art.rear)) {
                ++lamp_faces;
                CHECK(std::ranges::max(quad, {}, &r3d::vertex::z).z
                      > std::ranges::min(quad, {}, &r3d::vertex::z).z);
                CHECK(std::ranges::all_of(quad, [&quad](const auto& v) {
                    return v.x == Approx(quad[0].x);
                }));
            }
        }
    }
    CHECK(glass_faces > 0);
    CHECK(lamp_faces > 0);
    for (const auto state : {open_, broken}) {
        const auto& state_art = cache.named(
            {.category = C_VEHICLE_PART,
             .id = "vp_door_front_left",
             .tiles = *tilecontext,
             .subtile = state});
        CHECK_FALSE(state_art.glass.valid);
        CHECK_FALSE(state_art.cap.valid);
    }
    cache.set_presentation_profiles({});
    CHECK_FALSE(
        cache
            .named({.category = C_VEHICLE_PART,
                    .id = "vp_windshield_horizontal_front",
                    .tiles = *tilecontext})
            .glass.valid);
}

TEST_CASE(
    "r3d_central_roofs_reach_connected_exterior_materials", "[r3d][vehicle][tiles][materials]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    cache.set_presentation_profiles(r3d::load_presentation_profiles(input));
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    auto central_roof = -1;
    for (auto y = 0; y < 4; ++y) {
        const auto mount = tripoint_mnt_veh(0, y, 0);
        REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        const auto part =
            veh->install_part(mount, vpart_id(y == 3 ? "halfboard_horizontal" : "roof"), true);
        REQUIRE(part >= 0);
        if (y == 0) { central_roof = part; }
    }
    // An unclassified shutter's dark projected art must not supply the roof above it.
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("door_shutter"), true) >= 0);
    const auto& material =
        cache
            .named(
                {.category = C_VEHICLE_PART, .id = "vp_halfboard_horizontal", .tiles = *tilecontext})
            .main();
    REQUIRE(material.solid_uv);
    const auto& painted_roof =
        cache.named({.category = C_VEHICLE_PART, .id = "vp_roof", .tiles = *tilecontext}).cap;
    REQUIRE(painted_roof.valid);
    REQUIRE(painted_roof.solid_uv);
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
    const auto plain = r3d::build_vehicle_mesh(view);
    const auto has_roof_sample = [&](const r3d::chunk_mesh& mesh, const bool red) {
        const auto& source = red ? painted_roof : material;
        for (const auto& batch : mesh.batches) {
            if (batch.sheet != source.sheet) { continue; }
            for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
                const auto index = mesh.indices[i];
                const auto& v = mesh.vertices[index];
                if (v.y < -1.0f || v.y > 0.0f || v.z < r3d::floor_height + r3d::vehicle_body_h
                    || v.u != Approx((*source.solid_uv)[0])
                    || v.v != Approx((*source.solid_uv)[1])) {
                    continue;
                }
                if (!red
                    || ((mesh.light[index] & 255u) > 200u
                        && ((mesh.light[index] >> 8) & 255u) < 40u)) {
                    return true;
                }
            }
        }
        return false;
    };
    CHECK(has_roof_sample(plain, false));
    auto red = RGBColor{};
    red.r = 240;
    red.g = 30;
    red.b = 20;
    red.a = 255;
    veh->part(central_roof).set_color(red, red);
    CHECK(has_roof_sample(r3d::build_vehicle_mesh(view), true));
}

TEST_CASE(
    "r3d_rotors_reuse_native_mechanism_art_and_exclude_underbody_thrusters",
    "[r3d][vehicle][tiles][materials]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UNDEAD_PEOPLE_BASE", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    cache.set_presentation_profiles(r3d::load_presentation_profiles(input));
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    REQUIRE(veh->install_part(tripoint_mnt_veh::zero(), vpart_id("frame_horizontal"), true) >= 0);
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
    const auto thruster =
        veh->install_part(tripoint_mnt_veh::zero(), vpart_id("vtol_thruster"), true);
    REQUIRE(thruster >= 0);
    const auto underbody = r3d::build_vehicle_mesh(view);
    CHECK(std::ranges::max(underbody.vertices, {}, &r3d::vertex::x).x
              - std::ranges::min(underbody.vertices, {}, &r3d::vertex::x).x
          <= 1.01f);
    const auto rotor =
        veh->install_part(tripoint_mnt_veh::zero(), vpart_id("small_civilian_rotors"), true);
    REQUIRE(rotor >= 0);
    const auto& art = cache.named(
        {.category = C_VEHICLE_PART, .id = "vp_small_civilian_rotors", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE(art.side.valid);
    REQUIRE(art.side.solid_uv);
    CHECK(art.side.sheet == art.main().sheet);
    CHECK(art.side.u0 == Approx(art.main().u0 + (art.main().u1 - art.main().u0) * 0.125f));
    CHECK(art.side.v0 == Approx(art.main().v0 + (art.main().v1 - art.main().v0) * 0.125f));
    CHECK(art.side.u1 == Approx(art.main().u0 + (art.main().u1 - art.main().u0) * 0.3125f));
    CHECK(art.side.v1 == Approx(art.main().v0 + (art.main().v1 - art.main().v0) * 0.25f));
    const auto mesh = r3d::build_vehicle_mesh(view);
    auto blade_samples = 0;
    auto hub_art = 0;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != art.main().sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& v = mesh.vertices[mesh.indices[i]];
            if (std::abs(v.x - 0.5f) > 2.0f && v.u == Approx((*art.side.solid_uv)[0])
                && v.v == Approx((*art.side.solid_uv)[1])) {
                ++blade_samples;
            }
            if (v.z == Approx(r3d::floor_height + 0.02f + r3d::vehicle_cabin_h + 0.32f)
                && v.u == Approx(art.main().u0)) {
                ++hub_art;
                CHECK(std::abs(v.x - 0.5f) <= 0.501f);
            }
        }
    }
    CHECK(blade_samples > 0);
    CHECK(hub_art > 0);
}

TEST_CASE(
    "r3d_live_vehicle_surface_light_ignores_ground_memory_without_changing_visibility",
    "[r3d][vehicle][tiles][materials]") {
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
    REQUIRE(veh);
    for (auto x = 0; x < 2; ++x) {
        const auto mount = tripoint_mnt_veh(x, 0, 0);
        REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh->install_part(mount, vpart_id("roof"), true) >= 0);
    }
    here.build_map_cache(0);
    here.update_visibility_cache(0);
    auto& native = here.access_cache(0);
    const auto adjacent = veh->mount_to_bubble(tripoint_mnt_veh(1, 0, 0));
    native.lm[native.idx(origin.x(), origin.y())] = LIGHT_AMBIENT_LIT * 2;
    native.lm[native.idx(adjacent.x(), adjacent.y())] = LIGHT_AMBIENT_LOW;
    std::ranges::fill(native.visibility_cache, lit_level::MEMORIZED);
    const auto light = r3d::vehicle_surface_lighting(here);
    const auto lit = light(origin);
    const auto dim = light(adjacent);
    CHECK(lit != dim);
    CHECK((lit & 255u) == ((lit >> 8) & 255u));
    CHECK((dim & 255u) == ((dim >> 16) & 255u));
    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(205, 188, 160); },
        .markers = {},
        .uprights = {},
        .surface = {},
        .vehicle_mount_visible = [&here](const tripoint_bub_ms& p) { return here.pl_sees(p, -1); },
        .vehicle_surface_light = r3d::vehicle_surface_lighting(here)};
    CHECK(r3d::build_vehicle_mesh(view).empty());
    native.visibility_cache[native.idx(origin.x(), origin.y())] = lit_level::LOW;
    const auto mesh = r3d::build_vehicle_mesh(view);
    REQUIRE_FALSE(mesh.empty());
    auto roof_faces = 0;
    for (auto q = size_t{0}; q < mesh.vertices.size(); q += 4) {
        const auto quad = std::span(mesh.vertices).subspan(q, 4);
        const auto area =
            (quad[1].x - quad[0].x) * (quad[3].y - quad[0].y)
            - (quad[1].y - quad[0].y) * (quad[3].x - quad[0].x);
        if (std::abs(area) < 1e-6f || !std::ranges::all_of(quad, [](const auto& vertex) {
                return vertex.z >= r3d::floor_height + r3d::vehicle_body_h;
            })) {
            continue;
        }
        ++roof_faces;
        for (auto c = 0; c < 4; ++c) {
            const auto color = mesh.light[q + c];
            CHECK((color & 255u) == ((color >> 8) & 255u));
            CHECK((color & 255u) == ((color >> 16) & 255u));
        }
    }
    CHECK(roof_faces > 2);
    CHECK(light(adjacent) == dim);
    CHECK(std::ranges::any_of(mesh.light, [dim](const auto color) {
        return (color & 0x00ffffffu) == (dim & 0x00ffffffu);
    }));
}

TEST_CASE(
    "r3d_connected_roof_finish_does_not_follow_nearest_projected_panel",
    "[r3d][vehicle][tiles][materials]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    const auto tileset = GENERATE("UltimateCataclysm", "hoder");
    tilecontext->load_tileset(tileset, {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    cache.set_presentation_profiles(r3d::load_presentation_profiles(input));
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    auto joined = true;
    auto door_cap = false;
    SECTION("continuous_roof") {}
    SECTION("closed_door_cap_continues_the_roof") { door_cap = true; }
    SECTION("unroofed_gap_separates_finishes") { joined = false; }
    for (auto y = -1; y <= 3; ++y) {
        const auto mount = tripoint_mnt_veh(0, y, 0);
        REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
        if (y == -1 || y == 3) {
            REQUIRE(
                veh->install_part(
                    mount, vpart_id(y == -1 ? "halfboard_horizontal" : "board_vertical"), true)
                >= 0);
        } else if (y == 1 && door_cap) {
            REQUIRE(veh->install_part(mount, vpart_id("door_shutter"), true) >= 0);
        } else if (y != 1 || joined) {
            REQUIRE(veh->install_part(mount, vpart_id("roof"), true) >= 0);
        }
    }
    const auto& full =
        cache.named({.category = C_VEHICLE_PART, .id = "vp_board_vertical", .tiles = *tilecontext})
            .cap;
    const auto& half =
        cache
            .named(
                {.category = C_VEHICLE_PART, .id = "vp_halfboard_horizontal", .tiles = *tilecontext})
            .main();
    REQUIRE(full.valid);
    REQUIRE(full.solid_uv);
    REQUIRE(half.solid_uv);
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
    for (const auto y : {0, 1, 2, 3}) {
        if (y == 1 && !joined) { continue; }
        // The exterior full panel occupies the inner 0.4 tiles of its edge cell.
        const auto sections = r3d::describe_vehicle_sections(*veh);
        const auto& section_cell = sections.cells.at(tripoint_mnt_veh(0, y, 0));
        const auto row = section_cell.row;
        const auto shoulder =
            section_cell.canopy >= 0 && sections.canopies[section_cell.canopy].box
                    && !sections.canopies[section_cell.canopy].glazed
                ? 0.09f
                : 0.24f;
        const auto left = static_cast<float>(row.left) + (row.right - row.left >= 2 ? 0.1f : -0.5f);
        const auto right =
            static_cast<float>(row.right) + (row.right - row.left >= 2 ? -0.1f : 0.5f);
        const auto fit = [&](float p) {
            return r3d::vehicle_section_inset(
                {.position = p, .left = left, .right = right, .amount = shoulder});
        };
        const auto local_left = y - 0.5f;
        const auto local_right = y == 3 ? y - 0.1f : y + 0.5f;
        const auto fitted = y != 3 || section_cell.profile.has_roof || section_cell.profile.is_door;
        const auto min_y = y == 3 ? -4.0f : -0.5f - (fitted ? fit(local_right) : local_right);
        const auto max_y = y == 3 ? -3.0f : -0.5f - (fitted ? fit(local_left) : local_left);
        const auto& expected = joined || y >= 2 ? full : half;
        CAPTURE(tileset, y, joined, door_cap, min_y, max_y, section_cell.height);
        auto roof_faces = 0;
        for (const auto& batch : mesh.batches) {
            for (auto i = batch.first_index; i < batch.first_index + batch.index_count; i += 6) {
                const auto first = mesh.indices[i];
                const auto quad = std::span(mesh.vertices).subspan(first, 4);
                const auto cy = (quad[0].y + quad[1].y + quad[2].y + quad[3].y) * 0.25f;
                const auto area =
                    (quad[1].x - quad[0].x) * (quad[3].y - quad[0].y)
                    - (quad[1].y - quad[0].y) * (quad[3].x - quad[0].x);
                const auto roof_base =
                    r3d::floor_height + 0.02f + r3d::roof_cap_height(section_cell.height);
                const auto low = std::ranges::min(quad, {}, &r3d::vertex::z).z;
                const auto high = std::ranges::max(quad, {}, &r3d::vertex::z).z;
                if (cy <= min_y + 1e-5f || cy >= max_y - 1e-5f || std::abs(area) < 1e-6f
                    || high - low > 0.11f || !std::ranges::all_of(quad, [roof_base](const auto& v) {
                           return v.z >= roof_base - 1e-5f;
                       })) {
                    continue;
                }
                ++roof_faces;
                CHECK(batch.sheet == expected.sheet);
                for (const auto& vertex : quad) {
                    CHECK(vertex.u == Approx((*expected.solid_uv)[0]));
                    CHECK(vertex.v == Approx((*expected.solid_uv)[1]));
                }
            }
        }
        CHECK(roof_faces > 0);
    }
}

TEST_CASE("r3d_solid_vehicle_shoulders_join_the_inset_roof", "[r3d][vehicle][tiles][silhouette]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto mount = tripoint_mnt_veh::zero();
    REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
    REQUIRE(veh->install_part(mount, vpart_id("board_vertical"), true) >= 0);
    REQUIRE(veh->install_part(mount, vpart_id("roof"), true) >= 0);
    SECTION("exposed_wall") {}
    SECTION("lower_neighbor_still_needs_a_roof_shoulder") {
        const auto neighbor = tripoint_mnt_veh(-1, 0, 0);
        REQUIRE(veh->install_part(neighbor, vpart_id("frame_horizontal"), true) >= 0);
        REQUIRE(veh->install_part(neighbor, vpart_id("halfboard_horizontal"), true) >= 0);
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
    const auto height = r3d::floor_height + 0.02f + r3d::vehicle_cabin_h;
    for (const auto x : {0.18f, 0.82f}) {
        for (const auto y : {-0.24f, -0.76f}) {
            auto joins = 0;
            for (auto q = size_t{0}; q < mesh.vertices.size(); q += 4) {
                const auto quad = std::span(mesh.vertices).subspan(q, 4);
                if (std::ranges::any_of(quad, [x, y, height](const auto& v) {
                        return std::abs(v.x - x) < 1e-5f && std::abs(v.y - y) < 1e-5f
                            && std::abs(v.z - height) < 1e-5f;
                    })) {
                    ++joins;
                }
            }
            // Two roof skirts and two wall shoulders meet at each inset corner.
            // A roof merely floating above vertical walls only supplies the skirts.
            CHECK(joins >= 4);
        }
    }
}

TEST_CASE(
    "r3d_native_wheel_arch_cuts_skin_and_skirt_without_stretching_face_art",
    "[r3d][vehicle][tiles][sections]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto* const veh = get_map().add_vehicle(vproto_id("none"), origin, 0_degrees, 0, 0);
    REQUIRE(veh);
    const auto panel = GENERATE("halfboard_ne", "windshield");
    CAPTURE(panel);
    // Deliberately offset the native centreline: mount sign cannot pick wheel sides.
    for (auto x = -1; x <= 1; ++x) {
        for (auto y = 2; y <= 4; ++y) {
            const auto mount = tripoint_mnt_veh(x, y, 0);
            REQUIRE(veh->install_part(mount, vpart_id("frame_horizontal"), true) >= 0);
            REQUIRE(
                veh->install_part(
                    mount, vpart_id(x == 0 && y == 4 ? panel : "halfboard_horizontal"), true)
                >= 0);
        }
    }
    const auto wheel = veh->install_part(tripoint_mnt_veh(0, 4, 0), vpart_id("wheel"), true);
    REQUIRE(wheel >= 0);
    auto cache = r3d::tile_info_cache{};
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    cache.set_presentation_profiles(r3d::load_presentation_profiles(input));
    const auto& art =
        cache
            .named({.category = C_VEHICLE_PART,
                    .id = std::string("vp_") + panel,
                    .tiles = *tilecontext})
            .side;
    REQUIRE(art.valid);
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
    auto uv_derivative = std::optional<float>{};
    const auto check_arch = [&](const bool installed) {
        const auto mesh = r3d::build_vehicle_mesh(view);
        auto samples = 0;
        auto skirt_samples = 0;
        for (const auto& batch : mesh.batches) {
            for (auto i = batch.first_index; i < batch.first_index + batch.index_count; i += 6) {
                const auto quad = std::span(mesh.vertices).subspan(mesh.indices[i], 4);
                // Painted outer side, directly across the native axle. Wheel faces
                // and the inward arch reveal have different coordinates/materials.
                const auto skin = std::ranges::all_of(quad, [](const auto& v) {
                    return std::abs(v.y + 4.4f) < 1e-4f;
                });
                const auto skirt = std::ranges::all_of(quad, [](const auto& v) {
                    return std::abs(v.y + 4.32f) < 1e-4f;
                });
                if (batch.sheet != art.sheet || (!skin && !skirt)) { continue; }
                const auto low_x = std::ranges::min(quad, {}, &r3d::vertex::x).x;
                const auto high_x = std::ranges::max(quad, {}, &r3d::vertex::x).x;
                if (low_x > 0.5001f || high_x < 0.4999f || high_x - low_x < 0.001f) { continue; }
                const auto bottom = std::ranges::min(quad, {}, &r3d::vertex::z).z;
                const auto top = std::ranges::max(quad, {}, &r3d::vertex::z).z;
                if (top - bottom < 0.001f) { continue; }
                ++samples;
                skirt_samples += skirt;
                if (top >= r3d::floor_height + 0.02f + r3d::vehicle_body_h - 0.001f) {
                    const auto derivative = std::abs(
                        (quad[1].u - quad[0].u) / (quad[1].x - quad[0].x));
                    if (!uv_derivative) { uv_derivative = derivative; }
                    CHECK(derivative == Approx(*uv_derivative).margin(1e-5f));
                    if (!installed) {
                        CHECK(std::abs(quad[1].u - quad[0].u) == Approx(std::abs(art.u1 - art.u0)));
                    }
                }
                if (installed) {
                    const auto dimensions = r3d::native_wheel_dimensions(veh->cpart(wheel));
                    CHECK(bottom >= r3d::floor_height + 0.02f + dimensions.radius_up * 1.98f);
                } else {
                    CHECK(bottom < r3d::floor_height + 0.02f + r3d::vehicle_clearance_h + 0.001f);
                }
            }
        }
        CHECK(samples > 0);
        CHECK((skirt_samples == 0) == (installed || std::string_view(panel) == "halfboard_ne"));
    };
    check_arch(true);
    veh->part(wheel).removed = true;
    check_arch(false);
}


#endif
