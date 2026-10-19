#include "catch/catch.hpp"
#include "r3d_shapes.h"
#include "r3d_vehicle_rules.h"
#include "vehicle/veh_type.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_part.h"

#include <algorithm>
#include <cmath>
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
    const auto detail = r3d::vehicle_render_layer{
        .part_idx = 9, .roles = {r3d::vehicle_render_role::detail}};
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

#if defined(TILES)
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "path_info.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"

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
#endif
