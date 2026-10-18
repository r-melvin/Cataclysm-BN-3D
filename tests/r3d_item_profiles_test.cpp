#include "catch/catch.hpp"
#include "r3d_item_profiles.h"

TEST_CASE("r3d_cylinder_items_use_repeated_sprite_geometry", "[r3d][items]") {
    for (const auto id :
         {"carbondioxide_tank", "hydrogen_tank", "nitrogen_tank", "oxygen_cylinder", "oxygen_tank",
          "smoxygen_tank", "scuba_tank", "small_scuba_tank", "tinyweldtank", "weldtank",
          "gradcylinder"}) {
        const auto profile = r3d::item_profile(id);
        CHECK(profile.geometry == r3d::item_geometry::sprite_volume);
        CHECK(
            profile.depth_fraction
            == Approx(
                id == std::string_view("small_scuba_tank") || id == std::string_view("tinyweldtank")
                    ? 0.28f
                    : 0.38f));
    }
}

TEST_CASE("r3d_unprofiled_items_use_shallow_sprite_volume", "[r3d][items]") {
    const auto profile = r3d::item_profile("unknown_item");
    CHECK(profile.geometry == r3d::item_geometry::sprite_volume);
    CHECK(profile.depth_fraction == Approx(0.22f));
}

#if defined(TILES)
#    include "avatar.h"
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "item.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "options.h"
#    include "options_helpers.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"

TEST_CASE("r3d_voxel_item_menu_option_switches_tank_geometry", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("carbondioxide_tank", calendar::turn));

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
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto& art = cache.named(
        {.category = C_ITEM, .id = "carbondioxide_tank", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE_FALSE(art.backdrop.valid);
    REQUIRE(art.main().alpha_mask);
    REQUIRE(art.main().alpha_width > 0);
    REQUIRE(art.main().alpha_height > 0);
    REQUIRE(art.main().voxel_surface);

    const auto voxelized = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto billboard = r3d::build_entity_mesh(view);
    CHECK(voxelized.vertices.size() > billboard.vertices.size() + 16);
    CHECK(voxelized.indices.size() > billboard.indices.size() + 24);
}

TEST_CASE("r3d_voxel_item_option_extrudes_generic_item_sprites", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("rock", calendar::turn));

    auto cache = r3d::tile_info_cache{};
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = r3d::camera{},
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto& art = cache.named({.category = C_ITEM, .id = "rock", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE_FALSE(art.backdrop.valid);
    REQUIRE(art.main().alpha_mask);
    REQUIRE(art.main().voxel_surface);

    const auto voxelized = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto billboard = r3d::build_entity_mesh(view);
    CHECK(voxelized.vertices.size() > billboard.vertices.size() + 16);
    CHECK(voxelized.indices.size() > billboard.indices.size() + 24);
}

TEST_CASE("r3d_authored_voxel_item_uses_voxel_mesh_and_respects_menu_option", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("carbondioxide_tank", calendar::turn));

    auto model = r3d::vox_model{
        .size_x = 2,
        .size_y = 2,
        .size_z = 4,
        .palette = {},
        .cells = {{.x = 0, .y = 0, .z = 0, .color_index = 1}}};
    const auto surface = r3d::extract_vox_surface(model);
    REQUIRE(surface.has_value());
    auto palette = r3d::sprite_ref{};
    palette.valid = true;
    const auto assets = std::unordered_map<std::string, r3d::vox_asset>{
        {"carbondioxide_tank",
         {.model = std::move(model),
          .surface = *surface,
          .palette = palette}}};

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
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; },
        .item_vox_models = &assets};

    const auto authored = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto fallback = r3d::build_entity_mesh(view);
    CHECK(authored.vertices.size() == fallback.vertices.size() + 20);
    CHECK(authored.indices.size() == fallback.indices.size() + 30);
}
#endif
