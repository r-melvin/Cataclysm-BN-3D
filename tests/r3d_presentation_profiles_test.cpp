#include "catch/catch.hpp"
#include "path_info.h"
#include "r3d_furniture_profiles.h"
#include "r3d_presentation_profiles.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

TEST_CASE("r3d_profile_face_sources_validate_identity_crop_and_ambiguity", "[r3d][profiles]") {
    const auto bad_source = GENERATE(
        R"({"face":"side","tileset":"","sprite":"t_wall"})",
        R"({"face":"front","tileset":"test","sprite":"t_wall"})",
        R"({"face":"top","tileset":"test","sprite":"t_wall","crop":[0,0,2,1]})",
        R"({"face":"top","tileset":"test","sprite":"t_wall","crop":[0.5,0,0.5,1]})",
        R"({"face":"side","tileset":"test","sprite":"t_wall","rotation":4})",
        R"({"face":"side","tileset":"test","sprite":""})");
    auto input = std::istringstream(
        std::string(
            R"({"version":1,"profiles":[{"category":"terrain","id":"t_wall","face_sources":[)")
        + bad_source + "]}]}");
    CHECK_THROWS(r3d::load_presentation_profiles(input));
    auto duplicate = std::istringstream(R"({"version":1,"profiles":[{
        "category":"terrain","id":"t_wall","face_sources":[
        {"face":"top","tileset":"test","sprite":"t_wall"},
        {"face":"top","tileset":"test","sprite":"t_grass"}]}]})");
    CHECK_THROWS(r3d::load_presentation_profiles(duplicate));
}

TEST_CASE("r3d_furniture_authored_bounds_rotate_contacts_without_stretching", "[r3d][profiles]") {
    const auto facing = GENERATE(0, 1, 2, 3);
    const auto flip = GENERATE(false, true);
    const auto contact = GENERATE(false, true);
    const auto back = std::array<
        r3d::face_bit, 4>{r3d::face_north, r3d::face_east, r3d::face_south, r3d::face_west};
    const auto profile =
        r3d::furniture_proportions{.size = {0.4f, 0.6f, 0.8f}, .anchor = {0.25f, 0.75f, 0.2f}};
    const auto bounds = r3d::furniture_profile_bounds(
        {.fitted = {.x0 = -0.42f, .x1 = 1.0f, .y0 = 0.0f, .y1 = 0.9f},
         .profile = profile,
         .facing = facing,
         .flip = flip,
         .wall_contacts = static_cast<uint8_t>(contact ? back[facing] : 0)});
    CHECK(bounds.x1 - bounds.x0 == Approx(facing % 2 == 0 ? 0.4f : 0.6f));
    CHECK(bounds.y1 - bounds.y0 == Approx(facing % 2 == 0 ? 0.6f : 0.4f));
    if (contact) {
        switch (facing) {
            case 0:
                CHECK(bounds.y0 == Approx(-0.42f));
                break;
            case 1:
                CHECK(bounds.x1 == Approx(1.42f));
                break;
            case 2:
                CHECK(bounds.y1 == Approx(1.42f));
                break;
            case 3:
                CHECK(bounds.x0 == Approx(-0.42f));
                break;
        }
    } else {
        // The local anchor maps to the tile centre in every orientation.
        const auto ax = flip ? 0.75f : 0.25f;
        const auto anchors = std::array<std::array<float, 2>, 4>{
            {{ax, 0.75f}, {0.25f, ax}, {1.0f - ax, 0.25f}, {0.75f, 1.0f - ax}}};
        CHECK(bounds.x0 + anchors[facing][0] * (bounds.x1 - bounds.x0) == Approx(0.5f));
        CHECK(bounds.y0 + anchors[facing][1] * (bounds.y1 - bounds.y0) == Approx(0.5f));
    }
    const auto vertical = r3d::furniture_profile_height(profile, 0.9f);
    CHECK(vertical.base_offset == Approx(-0.16f));
    CHECK(vertical.top == Approx(0.74f));
}

TEST_CASE("r3d_furniture_omitted_axes_preserve_native_fitting", "[r3d][profiles]") {
    const auto profile = r3d::furniture_proportions{};
    const auto fitted = r3d::box_layout{.x0 = -0.42f, .x1 = 1.0f, .y0 = -0.42f, .y1 = 0.29f};
    const auto bounds = r3d::furniture_profile_bounds({.fitted = fitted, .profile = profile});
    CHECK(bounds.x0 == fitted.x0);
    CHECK(bounds.x1 == fitted.x1);
    CHECK(bounds.y0 == fitted.y0);
    CHECK(bounds.y1 == fitted.y1);
    const auto vertical = r3d::furniture_profile_height(profile, 0.9f);
    CHECK(vertical.top == 0.9f);
    CHECK(vertical.base_offset == 0.0f);
}

#if defined(TILES)
#    include "cata_tiles.h"
#    include "map/map.h"
#    include "map/mapdata.h"
#    include "map/submap.h"
#    include "r3d_mesh.h"
#    include "r3d_profile_materials.h"
#    include "sdltiles.h"
#    include "tile_helpers.h"

TEST_CASE(
    "r3d_wall_profile_uses_bound_donor_sheet_and_crop_then_restores_fallback",
    "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    cache.set_wall_sides(std::make_shared<r3d::wall_sides>());
    const auto original = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    REQUIRE(original.elevation.valid);
    const auto donor =
        cache.named({.category = C_TERRAIN, .id = "t_grass", .tiles = *tilecontext}).main();
    REQUIRE(donor.valid);
    auto source = r3d::profile_face_source{
        .face = r3d::profile_face::top,
        .tileset = "UltimateCataclysm",
        .sprite = "t_grass",
        .crop = {0.25f, 0.25f, 0.75f, 0.75f}};
    const auto face = GENERATE(r3d::profile_face::top, r3d::profile_face::side);
    source.face = face;
    const auto scenario = GENERATE(0, 1, 2);
    CAPTURE(scenario);
    if (scenario == 1) { source.tileset = "another_tileset"; }
    if (scenario == 2) { source.sprite = "missing_profile_donor"; }
    auto profile = r3d::presentation_profile{.face_sources = {source}};
    cache.set_presentation_profiles({{"terrain:t_wall", profile}});
    const auto art = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    const auto& material = face == r3d::profile_face::top ? art.cap : art.side;
    const auto& fallback = face == r3d::profile_face::top ? original.cap : original.side;
    if (scenario != 0) {
        CHECK(material.sheet == fallback.sheet);
        CHECK(material.valid == fallback.valid);
        CHECK(material.u0 == fallback.u0);
        return;
    }
    const auto cropped = r3d::crop_profile_sprite(
        {.sprite = donor, .source = source, .tileset = "UltimateCataclysm"});
    REQUIRE(cropped);
    REQUIRE(material.valid);
    CHECK(material.sheet == donor.sheet);
    CHECK(material.owner == donor.owner);
    CHECK(material.u0 == Approx(cropped->u0));
    CHECK(material.v1 == Approx(cropped->v1));
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    sm.set_ter(point_sm_ms(5, 5), t_wall);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    auto found_cap = false;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != donor.sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& vertex = mesh.vertices[mesh.indices[i]];
            if (vertex.z == Approx(art.sh.height) && vertex.u == Approx(cropped->u0)
                && vertex.v == Approx(cropped->v0)) {
                found_cap = true;
            }
        }
    }
    CHECK(found_cap);
    cache.set_presentation_profiles({});
    const auto restored = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    CHECK(restored.cap.sheet == original.cap.sheet);
    CHECK(restored.cap.valid == original.cap.valid);
    CHECK(restored.cap.u0 == original.cap.u0);
}

TEST_CASE("r3d_opening_profiles_preserve_native_geometry_and_leaf_art", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = std::string(
        GENERATE("t_window", "t_window_open", "t_window_frame", "t_window_empty", "t_door_c",
                 "t_door_o", "t_door_b", "t_door_frame"));
    const auto thin = GENERATE(false, true);
    const auto along_y = GENERATE(false, true);
    CAPTURE(id, thin, along_y);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    const auto terrain_id = ter_str_id(id).id();
    sm.set_ter(point_sm_ms(5, 5), terrain_id);
    sm.set_ter(point_sm_ms(along_y ? 5 : 4, along_y ? 4 : 5), t_wall);
    sm.set_ter(point_sm_ms(along_y ? 5 : 6, along_y ? 6 : 5), t_wall);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    cache.set_thin_walls(thin);
    const auto original = cache.terrain({.id = terrain_id, .tiles = *tilecontext});
    REQUIRE(original.sh.kind == r3d::shape_kind::pane);
    if (id == "t_window_frame" || id == "t_window_empty") {
        CHECK(original.window);
        CHECK_FALSE(terrain_id.obj().has_flag("WINDOW"));
    }
    const auto baseline = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    auto cap_source = r3d::profile_face_source{
        .face = r3d::profile_face::top,
        .tileset = "UltimateCataclysm",
        .sprite = "t_grass",
        .rotation = 1,
        .crop = {0.23f, 0.19f, 0.71f, 0.83f}};
    auto side_source = cap_source;
    side_source.face = r3d::profile_face::side;
    cache.set_presentation_profiles(
        {{"terrain:" + id, {.face_sources = {cap_source, side_source}}}});
    const auto art = cache.terrain({.id = terrain_id, .tiles = *tilecontext});
    REQUIRE(art.cap.valid);
    REQUIRE(art.side.valid);
    const auto donor =
        cache.named({.category = C_NONE, .id = "t_grass", .tiles = *tilecontext, .rota = 1}).main();
    CHECK(art.cap.sheet == donor.sheet);
    CHECK(art.side.sheet == donor.sheet);
    CHECK(art.sprite.sheet == original.sprite.sheet);
    CHECK(art.sprite.u0 == original.sprite.u0);
    CHECK(art.open_door == original.open_door);
    CHECK(art.closed_door_id == original.closed_door_id);
    CHECK(art.see_through == original.see_through);
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    CHECK(mesh.indices.size() == baseline.indices.size());
    const auto positions = [](const auto& geometry) {
        auto result = std::vector<std::array<float, 3>>{};
        for (const auto& vertex : geometry.vertices) {
            result.push_back({vertex.x, vertex.y, vertex.z});
        }
        std::ranges::sort(result);
        return result;
    };
    CHECK(positions(mesh) == positions(baseline));
    const auto raw_uvs = r3d::uvs_t{
        {{art.cap.u0, art.cap.v0},
         {art.cap.u1, art.cap.v0},
         {art.cap.u1, art.cap.v1},
         {art.cap.u0, art.cap.v1}}};
    const auto expected = r3d::orient_material_uvs(art.cap, raw_uvs)[0];
    const auto inset = thin ? 0.5f - r3d::wall_half_thickness : 0.0f;
    const auto cap_x = 5.0f + (along_y ? inset : 0.0f);
    const auto cap_y = -(5.0f + (along_y ? 0.0f : inset));
    auto found_cap = false;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != donor.sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& vertex = mesh.vertices[mesh.indices[i]];
            if (vertex.x == Approx(cap_x) && vertex.y == Approx(cap_y) && vertex.z == Approx(1.0f)
                && vertex.u == Approx(expected[0]) && vertex.v == Approx(expected[1])) {
                found_cap = true;
            }
        }
    }
    CHECK(found_cap);
}

TEST_CASE(
    "r3d_material_uv_orientation_keeps_partial_regions_in_the_source_crop", "[r3d][profiles]") {
    const auto sprite =
        r3d::sprite_ref{.u0 = 0.2f, .v0 = 0.3f, .u1 = 0.6f, .v1 = 0.9f, .quarter_turns = 1};
    const auto part = r3d::uvs_t{{{0.3f, 0.6f}, {0.4f, 0.6f}, {0.4f, 0.9f}, {0.3f, 0.9f}}};
    const auto oriented = r3d::orient_material_uvs(sprite, part);
    // The first point is (1/4, 1/2) of the crop: clockwise rotation maps it to (1/2, 3/4).
    CHECK(oriented[0][0] == Approx(0.4f));
    CHECK(oriented[0][1] == Approx(0.75f));
    auto unrotated = sprite;
    unrotated.quarter_turns = 0;
    CHECK(r3d::orient_material_uvs(unrotated, part) == part);
}

TEST_CASE("r3d_furniture_face_profiles_preserve_fronts_and_geometry", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = std::string(GENERATE("f_locker", "f_chair", "f_trashcan", "f_table"));
    const auto face =
        GENERATE(r3d::profile_face::top, r3d::profile_face::side, r3d::profile_face::front);
    CAPTURE(id, static_cast<int>(face));
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    const auto furniture = furn_str_id(id).id();
    sm.set_furn(point_sm_ms(5, 5), furniture);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto original = cache.furniture({.id = furniture, .tiles = *tilecontext});
    REQUIRE(original.sh.kind == r3d::shape_kind::prism);
    const auto baseline = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    cache.set_presentation_profiles(
        {{"furniture:" + id,
          {.face_sources = {
               {.face = face,
                .tileset = "UltimateCataclysm",
                .sprite = "t_grass",
                .crop = {0.13f, 0.23f, 0.67f, 0.79f}}}}}});
    const auto art = cache.furniture({.id = furniture, .tiles = *tilecontext});
    CHECK(art.main().sheet == original.main().sheet);
    CHECK(art.main().u0 == original.main().u0);
    CHECK(art.depth_scale == original.depth_scale);
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    CHECK(mesh.indices.size() == baseline.indices.size());
    auto changed = 0;
    auto preserved_front = 0;
    auto front_y = 0.0f;
    for (const auto& vertex : baseline.vertices) {
        if (vertex.x >= 5.0f && vertex.x <= 6.0f && vertex.y <= -5.0f && vertex.y >= -6.0f
            && vertex.z == Approx(art.sh.height)) {
            front_y = std::min(front_y, vertex.y);
        }
    }
    for (auto index = size_t(0); index < baseline.vertices.size(); index += 4) {
        const auto& first = baseline.vertices[index];
        auto belongs = first.z == Approx(art.sh.height);
        for (auto corner = size_t(0); corner < 4; ++corner) {
            const auto& vertex = baseline.vertices[index + corner];
            belongs = belongs && vertex.x >= 5.0f && vertex.x <= 6.0f && vertex.y <= -5.0f
                   && vertex.y >= -6.0f;
        }
        if (!belongs) { continue; }
        auto matched = false;
        for (auto target = size_t(0); target < mesh.vertices.size(); target += 4) {
            auto same = true;
            auto different_uv = false;
            auto top = true;
            auto front = true;
            for (auto corner = size_t(0); corner < 4; ++corner) {
                const auto& before = baseline.vertices[index + corner];
                const auto& after = mesh.vertices[target + corner];
                same = same && before.x == after.x && before.y == after.y && before.z == after.z;
                different_uv = different_uv || before.u != after.u || before.v != after.v;
                top = top && before.z == Approx(art.sh.height);
                front = front && before.y == Approx(front_y);
            }
            if (!same) { continue; }
            matched = true;
            const auto should_change =
                face == r3d::profile_face::top ? top
                : face == r3d::profile_face::front
                    ? front
                    : !top && !front;
            CHECK(different_uv == should_change);
            if (different_uv) {
                ++changed;
                const auto& material =
                    face == r3d::profile_face::top ? art.cap
                    : face == r3d::profile_face::front
                        ? art.front
                        : art.side;
                REQUIRE(material.valid);
                // Check the bound draw batch as well as UVs; different sheets can share
                // coordinates.
                const auto vertex_index = static_cast<uint32_t>(target);
                auto correct_sheet = false;
                for (const auto& batch : mesh.batches) {
                    for (auto i = batch.first_index; i < batch.first_index + batch.index_count;
                         ++i) {
                        if (mesh.indices[i] == vertex_index && batch.sheet == material.sheet) {
                            correct_sheet = true;
                        }
                    }
                }
                CHECK(correct_sheet);
            }
            if (front && !different_uv) { ++preserved_front; }
            break;
        }
        CHECK(matched);
    }
    CHECK(changed > 0);
    if (face != r3d::profile_face::front) { CHECK(preserved_front > 0); }
}

TEST_CASE("r3d_furniture_profiles_parse_explicit_front_sources", "[r3d][profiles]") {
    auto input = std::istringstream(R"({"version":1,"profiles":[{
      "category":"furniture","id":"f_locker","face_sources":[{
      "face":"front","tileset":"UltimateCataclysm","sprite":"f_locker"}]}]})");
    const auto profiles = r3d::load_presentation_profiles(input);
    REQUIRE(profiles.at("furniture:f_locker").face_sources.size() == 1);
    CHECK(profiles.at("furniture:f_locker").face_sources[0].face == r3d::profile_face::front);
}
TEST_CASE("r3d_furniture_profile_sizes_reach_mesh_and_surface_cache", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto scale = GENERATE(1.0f, 0.1f);
    const auto id = std::string(GENERATE("f_locker", "f_chair", "f_trashcan", "f_table"));
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    const auto furniture = furn_str_id(id).id();
    sm.set_furn(point_sm_ms(5, 5), furniture);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto original = cache.furniture({.id = furniture, .tiles = *tilecontext});
    const auto baseline = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    cache.set_presentation_profiles(
        {{"furniture:" + id,
          {.size = {0.4f * scale, 0.6f * scale, 0.8f}, .support_anchor = {0.25f, 0.75f, 0.2f}}}});
    const auto art = cache.furniture({.id = furniture, .tiles = *tilecontext});
    CHECK(art.sh.height == Approx(0.74f));
    CHECK(art.base_offset == Approx(-0.16f));
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    auto low = std::array<float, 3>{100, 100, 100};
    auto high = std::array<float, 3>{-100, -100, -100};
    for (auto i = size_t(0); i < mesh.vertices.size(); i += 4) {
        auto furniture_face = false;
        for (auto j = size_t(0); j < 4; ++j) {
            furniture_face = furniture_face || mesh.vertices[i + j].z != Approx(r3d::floor_height);
        }
        if (!furniture_face) { continue; }
        for (auto j = size_t(0); j < 4; ++j) {
            const auto& vertex = mesh.vertices[i + j];
            const auto point = std::array<float, 3>{vertex.x, vertex.y, vertex.z};
            for (auto axis = size_t(0); axis < 3; ++axis) {
                low[axis] = std::min(low[axis], point[axis]);
                high[axis] = std::max(high[axis], point[axis]);
            }
        }
    }
    // No wall or row: the authored point sits at the centre of the cell, above the floor.
    CHECK(low[0] == Approx(5.5f - 0.1f * scale));
    CHECK(high[0] == Approx(5.5f + 0.3f * scale));
    CHECK(low[1] == Approx(-5.5f - 0.15f * scale));
    CHECK(high[1] == Approx(-5.5f + 0.45f * scale));
    CHECK(low[2] == Approx(-0.06f));
    CHECK(high[2] == Approx(art.sh.height));
    cache.set_presentation_profiles({});
    CHECK(
        cache.furniture({.id = furniture, .tiles = *tilecontext}).sh.height == original.sh.height);
    const auto restored = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    REQUIRE(restored.vertices.size() == baseline.vertices.size());
    for (auto i = size_t(0); i < baseline.vertices.size(); ++i) {
        CHECK(restored.vertices[i].x == baseline.vertices[i].x);
        CHECK(restored.vertices[i].y == baseline.vertices[i].y);
        CHECK(restored.vertices[i].z == baseline.vertices[i].z);
    }
}
TEST_CASE("r3d_missing_wall_art_uses_material_instead_of_unknown_glyph", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("hoder", {}, false, true, false);
    REQUIRE_FALSE(tilecontext->get_sprite_draw_data(
        {.id = "t_wall_w", .category = C_TERRAIN, .pos = tripoint_bub_ms(0, 0, 0), .variant = ""}));
    auto cache = r3d::tile_info_cache{};
    const auto generic = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    const auto missing = cache.terrain({.id = ter_str_id("t_wall_w").id(), .tiles = *tilecontext});
    REQUIRE(generic.main().valid);
    CHECK(missing.main().sheet == generic.main().sheet);
    CHECK(missing.main().u0 == generic.main().u0);
    CHECK(missing.main().v0 == generic.main().v0);
    // Even with native wall textures selected, missing art must use the material atlas.
    auto sides = std::make_shared<r3d::wall_sides>();
    sides->atlas = generic.main().owner;
    auto width = 0.0f;
    auto height = 0.0f;
    REQUIRE(SDL_GetTextureSize(sides->atlas.get(), &width, &height));
    sides->width = static_cast<int>(width);
    sides->height = static_cast<int>(height);
    sides->cell = std::min(sides->width, sides->height);
    sides->names = {"plaster"};
    sides->rules.by_prefix = {{"t_wall_", "plaster"}};
    sides->colors = {{"plaster", 0xc4baa0u}};
    cache.set_wall_sides(sides);
    cache.set_plaster_atlas(false);
    const auto wall = cache.terrain({.id = ter_str_id("t_wall_w").id(), .tiles = *tilecontext});
    CHECK(wall.side.valid);
    CHECK(wall.cap.valid);
    CHECK(wall.side.sheet == sides->atlas.get());
    CHECK_FALSE(wall.elevation.valid);
}
TEST_CASE("r3d_wall_elevation_uses_horizontal_edge_art", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = std::string(GENERATE("t_wall", "t_wall_w"));
    const auto rotation = GENERATE(0, 1, 2, 3);
    auto cache = r3d::tile_info_cache{};
    cache.set_wall_sides(std::make_shared<r3d::wall_sides>());
    const auto art = cache.terrain(
        {.id = ter_str_id(id).id(), .tiles = *tilecontext, .subtile = 2, .rota = rotation});
    const auto expected = tilecontext->get_sprite_draw_data(
        {.id = id,
         .category = C_TERRAIN,
         .pos = tripoint_bub_ms(0, 0, 0),
         .subtile = 2,
         .rotation = 1,
         .variant = ""});
    REQUIRE(expected);
    REQUIRE(art.elevation.valid);
    auto width = 0.0f;
    auto height = 0.0f;
    REQUIRE(SDL_GetTextureSize(expected->texture, &width, &height));
    CHECK(art.elevation.sheet == expected->texture);
    CHECK(art.elevation.u0 == Approx(expected->uv0.first + 0.5f / width));
    CHECK(art.elevation.v0 == Approx(expected->uv0.second + 0.5f / height));
    CHECK(art.elevation.u1 == Approx(expected->uv1.first - 0.5f / width));
    CHECK(art.elevation.v1 == Approx(expected->uv1.second - 0.5f / height));
}
TEST_CASE("r3d_bundled_trashcan_materials_are_opaque_and_attached", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    REQUIRE(input.good());
    const auto profiles = r3d::load_presentation_profiles(input);
    const auto& profile = profiles.at("furniture:f_trashcan");
    REQUIRE(profile.face_sources.size() == 3);
    for (const auto& source : profile.face_sources) {
        const auto donor = tilecontext->get_sprite_draw_data(
            {.id = source.sprite,
             .category = C_NONE,
             .pos = tripoint_bub_ms(0, 0, 0),
             .variant = ""});
        REQUIRE(donor);
        REQUIRE(donor->alpha_mask);
        const auto width = donor->dimensions.w;
        const auto height = donor->dimensions.h;
        for (auto y = static_cast<int>(source.crop[1] * height); y < source.crop[3] * height; ++y) {
            for (auto x = static_cast<int>(source.crop[0] * width); x < source.crop[2] * width;
                 ++x) {
                CHECK((*donor->alpha_mask)[static_cast<size_t>(y * width + x)] != 0);
            }
        }
    }
    auto cache = r3d::tile_info_cache{};
    cache.set_presentation_profiles(profiles);
    const auto art = cache.furniture({.id = furn_str_id("f_trashcan").id(), .tiles = *tilecontext});
    CHECK(art.sh.height == Approx(r3d::floor_height + profile.size[2]));
    CHECK(art.cap.valid);
    CHECK(art.front.valid);
    CHECK(art.side.valid);
    CHECK(art.front.sheet == art.main().sheet);
    CHECK(art.side.sheet == art.main().sheet);
    CHECK(art.cap.sheet == art.main().sheet);
}
#endif
