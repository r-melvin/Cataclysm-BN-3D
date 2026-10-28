#include "catacharset.h"
#include "catch/catch.hpp"
#include "r3d.h"
#include "r3d_targeting_panel.h"

#include <optional>
#include <string>
#include <vector>

#if defined(TILES)
#    include <SDL3/SDL.h>
#endif

TEST_CASE("r3d_targeting_panel_lifecycle_and_data_preservation", "[r3d]") {
#if defined(TILES)
    r3d::clear_targeting_panel_data();
    CHECK_FALSE(r3d::has_targeting_panel());

    auto data = r3d::targeting_panel_data{};
    data.title = "Firing Glock 19";
    data.mode_text = "Semi-Auto (1)";
    data.ammo_text = "9x19mm (15/15)";
    data.recoil_text = "Recoil: None";
    data.target_name = "Zombie";
    data.target_detail = "Hostile";
    data.range = 5;
    data.max_range = 14;
    data.elevation = 0;
    data.out_of_range = false;
    data.out_of_ammo = false;
    data.has_target = true;
    data.can_switch_mode = true;
    data.can_switch_ammo = false;

    data.presets = {
        {.action_id = "FIRE",
         .label = "Current",
         .shortcut_text = "f",
         .moves_cost = 80,
         .chance_great = 25,
         .chance_normal = 55,
         .chance_graze = 15,
         .has_accuracy = true,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = true},
        {.action_id = "AIMED_SHOT",
         .label = "Aimed Shot",
         .shortcut_text = "a",
         .moves_cost = 120,
         .chance_great = 40,
         .chance_normal = 50,
         .chance_graze = 8,
         .has_accuracy = true,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false},
        {.action_id = "CAREFUL_SHOT",
         .label = "Careful Shot",
         .shortcut_text = "c",
         .moves_cost = 160,
         .chance_great = 55,
         .chance_normal = 40,
         .chance_graze = 4,
         .has_accuracy = true,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false},
        {.action_id = "PRECISE_SHOT",
         .label = "Precise Shot",
         .shortcut_text = "p",
         .moves_cost = 210,
         .chance_great = 70,
         .chance_normal = 28,
         .chance_graze = 1,
         .has_accuracy = true,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false}};

    data.aim_only = r3d::targeting_action_item{
        .action_id = "AIM",
        .label = "Aim",
        .shortcut_text = ".",
        .moves_cost = 10,
        .chance_great = 0,
        .chance_normal = 0,
        .chance_graze = 0,
        .has_accuracy = false,
        .enabled = true,
        .is_aim_only = true,
        .is_primary = false};

    data.controls = {
        {.action_id = "SWITCH_MODE",
         .label = "Mode",
         .shortcut_text = "s",
         .moves_cost = -1,
         .chance_great = 0,
         .chance_normal = 0,
         .chance_graze = 0,
         .has_accuracy = false,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false},
        {.action_id = "PREV_TARGET",
         .label = "< Prev",
         .shortcut_text = "<",
         .moves_cost = -1,
         .chance_great = 0,
         .chance_normal = 0,
         .chance_graze = 0,
         .has_accuracy = false,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false},
        {.action_id = "NEXT_TARGET",
         .label = "Next >",
         .shortcut_text = ">",
         .moves_cost = -1,
         .chance_great = 0,
         .chance_normal = 0,
         .chance_graze = 0,
         .has_accuracy = false,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false},
        {.action_id = "QUIT",
         .label = "Cancel",
         .shortcut_text = "Esc",
         .moves_cost = -1,
         .chance_great = 0,
         .chance_normal = 0,
         .chance_graze = 0,
         .has_accuracy = false,
         .enabled = true,
         .is_aim_only = false,
         .is_primary = false}};

    r3d::set_targeting_panel_data(data);
    REQUIRE(r3d::has_targeting_panel());

    const auto& retrieved = r3d::get_targeting_panel_data();
    CHECK(retrieved.title == "Firing Glock 19");
    CHECK(retrieved.mode_text == "Semi-Auto (1)");
    CHECK(retrieved.ammo_text == "9x19mm (15/15)");
    CHECK(retrieved.range == 5);
    CHECK(retrieved.max_range == 14);
    CHECK(retrieved.target_name == "Zombie");

    REQUIRE(retrieved.presets.size() == 4);
    CHECK(retrieved.presets[0].label == "Current");
    CHECK(retrieved.presets[0].shortcut_text == "f");
    CHECK(retrieved.presets[0].moves_cost == 80);
    CHECK(retrieved.presets[0].chance_great == 25);

    CHECK(retrieved.presets[1].label == "Aimed Shot");
    CHECK(retrieved.presets[1].shortcut_text == "a");
    CHECK(retrieved.presets[1].moves_cost == 120);

    CHECK(retrieved.presets[2].label == "Careful Shot");
    CHECK(retrieved.presets[2].shortcut_text == "c");
    CHECK(retrieved.presets[2].moves_cost == 160);

    CHECK(retrieved.presets[3].label == "Precise Shot");
    CHECK(retrieved.presets[3].shortcut_text == "p");
    CHECK(retrieved.presets[3].moves_cost == 210);

    REQUIRE(retrieved.aim_only.has_value());
    CHECK(retrieved.aim_only->action_id == "AIM");
    CHECK(retrieved.aim_only->is_aim_only);
    CHECK(retrieved.aim_only->moves_cost == 10);
    CHECK(retrieved.aim_only->shortcut_text == ".");

    r3d::clear_targeting_panel_data();
    CHECK_FALSE(r3d::has_targeting_panel());
    CHECK_FALSE(r3d::targeting_panel_contains(50.0f, 50.0f));
#endif
}

TEST_CASE("r3d_targeting_panel_hit_testing_and_controls", "[r3d]") {
#if defined(TILES)
    r3d::clear_targeting_panel_data();

    // Create a software SDL renderer for testing drawing and hit bounds
    auto* surface = SDL_CreateSurface(800, 600, SDL_PIXELFORMAT_RGBA8888);
    REQUIRE(surface != nullptr);
    auto* renderer = SDL_CreateSoftwareRenderer(surface);
    REQUIRE(renderer != nullptr);

    auto data = r3d::targeting_panel_data{};
    data.title = "Firing Rifle";
    data.mode_text = "Burst (3)";
    data.ammo_text = "5.56mm (30/30)";
    data.range = 8;
    data.max_range = 20;

    data.presets = {
        {.action_id = "FIRE",
         .label = "Current",
         .shortcut_text = "f",
         .moves_cost = 90,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = true},
        {.action_id = "AIMED_SHOT",
         .label = "Aimed Shot",
         .shortcut_text = "a",
         .moves_cost = 135,
         .has_accuracy = true,
         .enabled = true}};

    data.aim_only = r3d::targeting_action_item{
        .action_id = "AIM",
        .label = "Aim",
        .shortcut_text = ".",
        .moves_cost = 10,
        .enabled = true,
        .is_aim_only = true};

    data.controls =
        {{.action_id = "SWITCH_MODE", .label = "Mode", .shortcut_text = "s", .enabled = true},
         {.action_id = "QUIT", .label = "Cancel", .shortcut_text = "Esc", .enabled = true}};

    r3d::set_targeting_panel_data(data);

    auto texts = std::vector<r3d::hud_text>{};
    const auto draw_opts = r3d::draw_targeting_panel_options{
        .renderer = renderer,
        .view_x = 0.0f,
        .view_y = 0.0f,
        .view_w = 800.0f,
        .view_h = 600.0f,
        .texts = texts,
        .cell_w = 8.0f,
        .cell_h = 16.0f};
    r3d::draw_targeting_panel(draw_opts);

    // Texts should have been queued
    CHECK_FALSE(texts.empty());

    // Panel is positioned at bottom-left: view_x + 16 = 16.0f, width is clamped to 380.0f
    // Points far away outside the panel should NOT be contained
    CHECK_FALSE(r3d::targeting_panel_contains(500.0f, 100.0f));
    CHECK_FALSE(r3d::targeting_panel_contains(0.0f, 0.0f));

    // Point inside the panel bounds
    const auto panel_center_x = 16.0f + 190.0f; // 206.0f
    const auto panel_bottom_y = 600.0f - 30.0f; // 570.0f (inside panel margin)
    CHECK(r3d::targeting_panel_contains(panel_center_x, panel_bottom_y));

    // Hover move over Preset 1 button (y ~ 470.0f)
    const auto preset_btn_y = 470.0f;
    CHECK(r3d::targeting_panel_mouse_move(panel_center_x, preset_btn_y));
    // Moving again within the same button does not trigger a hover change
    CHECK_FALSE(r3d::targeting_panel_mouse_move(panel_center_x + 10.0f, preset_btn_y));

    // Click on Preset 1 button
    const auto preset_click = r3d::targeting_panel_mouse_click(panel_center_x, preset_btn_y);
    REQUIRE(preset_click.has_value());
    CHECK(*preset_click == "FIRE");

    // Click on Cancel button (bottom auxiliary row: x ~ 286.0f, y ~ 556.0f)
    const auto cancel_btn_x = panel_center_x + 80.0f;
    const auto cancel_btn_y = 556.0f;
    const auto click_res = r3d::targeting_panel_mouse_click(cancel_btn_x, cancel_btn_y);
    REQUIRE(click_res.has_value());
    CHECK(*click_res == "QUIT");

    // Moving hover over Cancel button changes hover
    CHECK(r3d::targeting_panel_mouse_move(cancel_btn_x, cancel_btn_y));
    // Moving hover to empty margin inside panel clears button hover (returns true)
    const auto panel_margin_x = 20.0f;
    const auto panel_margin_y = 385.0f;
    CHECK(r3d::targeting_panel_mouse_move(panel_margin_x, panel_margin_y));
    // Moving within empty margin again leaves hover unchanged (returns false)
    CHECK_FALSE(r3d::targeting_panel_mouse_move(panel_margin_x + 2.0f, panel_margin_y));

    r3d::clear_targeting_panel_data();
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
#endif
}

TEST_CASE("r3d_targeting_panel_no_text_overlap_and_vats_percentage_badge", "[r3d]") {
#if defined(TILES)
    r3d::clear_targeting_panel_data();

    auto* surface = SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA8888);
    REQUIRE(surface != nullptr);
    auto* renderer = SDL_CreateSoftwareRenderer(surface);
    REQUIRE(renderer != nullptr);

    auto data = r3d::targeting_panel_data{};
    data.title = "Firing Savage 111F";
    data.mode_text = "auto (1)";
    data.ammo_text = "Ammo: .308 Winchester (3/5)";
    data.recoil_text = "Recoil: None";
    data.target_name = "feral human";
    data.target_detail = "Hostile";
    data.range = 1;
    data.max_range = 36;
    data.elevation = 0;
    data.has_target = true;
    data.can_switch_mode = true;
    data.can_switch_ammo = true;

    data.presets = {
        {.action_id = "FIRE",
         .label = "to fire",
         .shortcut_text = "f",
         .moves_cost = 130,
         .chance_great = 15,
         .chance_normal = 45,
         .chance_graze = 25,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = true},
        {.action_id = "AIMED_SHOT",
         .label = "Aimed Shot",
         .shortcut_text = "a",
         .moves_cost = 150,
         .chance_great = 30,
         .chance_normal = 45,
         .chance_graze = 15,
         .has_accuracy = true,
         .enabled = true},
        {.action_id = "CAREFUL_SHOT",
         .label = "Careful Shot",
         .shortcut_text = "c",
         .moves_cost = 185,
         .chance_great = 50,
         .chance_normal = 38,
         .chance_graze = 7,
         .has_accuracy = true,
         .enabled = true},
        {.action_id = "PRECISE_SHOT",
         .label = "Precise Shot",
         .shortcut_text = "p",
         .moves_cost = 230,
         .chance_great = 70,
         .chance_normal = 28,
         .chance_graze = 1,
         .has_accuracy = true,
         .enabled = true}};

    data.aim_only = r3d::targeting_action_item{
        .action_id = "AIM",
        .label = "Aim",
        .shortcut_text = ".",
        .moves_cost = 10,
        .enabled = true,
        .is_aim_only = true};

    data.controls =
        {{.action_id = "SWITCH_MODE", .label = "Mode", .shortcut_text = "s", .enabled = true},
         {.action_id = "SWITCH_AMMO", .label = "Ammo", .shortcut_text = "o", .enabled = true},
         {.action_id = "PREV_TARGET", .label = "< Prev", .shortcut_text = "<", .enabled = true},
         {.action_id = "NEXT_TARGET", .label = "Next >", .shortcut_text = ">", .enabled = true},
         {.action_id = "QUIT", .label = "Cancel", .shortcut_text = "Esc", .enabled = true}};

    r3d::set_targeting_panel_data(data);
    REQUIRE(r3d::effective_targeting_hit_chance().has_value());
    CHECK(*r3d::effective_targeting_hit_chance() == 85);

    for (const auto test_cell_w : {8.0f, 16.0f}) {
        auto texts = std::vector<r3d::hud_text>{};
        r3d::draw_targeting_panel(
            {.renderer = renderer,
             .view_x = 0.0f,
             .view_y = 0.0f,
             .view_w = 1280.0f,
             .view_h = 720.0f,
             .texts = texts,
             .cell_w = test_cell_w,
             .cell_h = 16.0f,
             .target_screen_pos = r3d::vec3{.x = 640.0f, .y = 260.0f, .z = 0.5f},
             .target_head_screen_pos = r3d::vec3{.x = 640.0f, .y = 210.0f, .z = 0.5f}});

        REQUIRE(texts.size() >= 16);
        CHECK(texts.back().text == "85%");

        // Verify no two HUD text bounding boxes overlap
        for (size_t i = 0; i < texts.size(); ++i) {
            const auto w_i = static_cast<float>(utf8_width(texts[i].text)) * test_cell_w;
            const auto l_i =
                texts[i].align < 0
                    ? texts[i].x
                    : (texts[i].align > 0 ? texts[i].x - w_i : texts[i].x - w_i * 0.5f);
            const auto r_i = l_i + w_i;
            const auto t_i = texts[i].y;
            const auto b_i = texts[i].y + 16.0f;

            // Every panel text (all except the VATS badge on the enemy) must lie inside the panel
            if (i + 1 < texts.size()) {
                CHECK(r3d::targeting_panel_contains(l_i + 1.0f, t_i + 2.0f));
                CHECK(r3d::targeting_panel_contains(std::max(l_i + 1.0f, r_i - 1.0f), t_i + 2.0f));
            } else {
                // VATS percentage callout on the enemy must not overlap the panel
                CHECK_FALSE(r3d::targeting_panel_contains(l_i, t_i));
            }

            for (size_t j = i + 1; j < texts.size(); ++j) {
                const auto w_j = static_cast<float>(utf8_width(texts[j].text)) * test_cell_w;
                const auto l_j =
                    texts[j].align < 0
                        ? texts[j].x
                        : (texts[j].align > 0 ? texts[j].x - w_j : texts[j].x - w_j * 0.5f);
                const auto r_j = l_j + w_j;
                const auto t_j = texts[j].y;
                const auto b_j = texts[j].y + 16.0f;

                const auto horiz_overlap = l_i < r_j - 0.5f && r_i > l_j + 0.5f;
                const auto vert_overlap = t_i < b_j - 0.5f && b_i > t_j + 0.5f;
                INFO("cell_w=" << test_cell_w << " text[" << i << "]='" << texts[i].text << "' ["
                               << l_i << "," << r_i << "] vs text[" << j << "]='" << texts[j].text
                               << "' [" << l_j << "," << r_j << "]");
                CHECK_FALSE((horiz_overlap && vert_overlap));
            }
        }
    }

    r3d::clear_targeting_panel_data();
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
#endif
}

TEST_CASE("r3d_targeting_sightline_color_and_inline_aim_tiers", "[r3d]") {
#if defined(TILES)
    const auto col_low = r3d::sightline_color_for_hit_chance(2, false);
    const auto col_mid = r3d::sightline_color_for_hit_chance(40, false);
    const auto col_high = r3d::sightline_color_for_hit_chance(100, false);
    const auto col_oor = r3d::sightline_color_for_hit_chance(95, true);

    // 2% hit chance is red-dominant
    CHECK((col_low & 0xFFu) > 220u);
    CHECK(((col_low >> 8u) & 0xFFu) < 75u);
    // 40% hit chance is amber (both red and green high, blue low)
    CHECK((col_mid & 0xFFu) > 230u);
    CHECK(((col_mid >> 8u) & 0xFFu) > 175u);
    CHECK(((col_mid >> 16u) & 0xFFu) < 75u);
    // 100% hit chance is bright green-dominant
    CHECK((col_high & 0xFFu) < 100u);
    CHECK(((col_high >> 8u) & 0xFFu) > 225u);
    // Out-of-range forces red regardless of percentage
    CHECK((col_oor & 0xFFu) > 220u);
    CHECK(((col_oor >> 8u) & 0xFFu) < 70u);

    r3d::clear_targeting_panel_data();
    auto* surface = SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA8888);
    REQUIRE(surface != nullptr);
    auto* renderer = SDL_CreateSoftwareRenderer(surface);
    REQUIRE(renderer != nullptr);

    auto data = r3d::targeting_panel_data{};
    data.title = "Firing Desert Eagle .44";
    data.mode_text = "semi (1)";
    data.ammo_text = "Ammo: .44 Magnum (10/10)";
    data.recoil_text = "Recoil: High";
    data.has_target = true;
    data.target_name = "zombie";
    data.range = 6;
    data.max_range = 16;
    data.aim_tiers = {
        {.action_id = "SELECT_AIM_0",
         .label = "Immediate",
         .shortcut_text = "1",
         .chance_great = 1,
         .chance_normal = 4,
         .chance_graze = 7,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = true},
        {.action_id = "SELECT_AIM_1",
         .label = "Regular",
         .shortcut_text = "2",
         .chance_great = 5,
         .chance_normal = 16,
         .chance_graze = 5,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = false},
        {.action_id = "SELECT_AIM_2",
         .label = "Careful",
         .shortcut_text = "3",
         .chance_great = 12,
         .chance_normal = 28,
         .chance_graze = 9,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = false},
        {.action_id = "SELECT_AIM_3",
         .label = "Precise",
         .shortcut_text = "4",
         .chance_great = 25,
         .chance_normal = 42,
         .chance_graze = 11,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = false}};
    data.presets = {
        {.action_id = "FIRE",
         .label = "to fire",
         .shortcut_text = "f",
         .moves_cost = 80,
         .chance_great = 1,
         .chance_normal = 4,
         .chance_graze = 7,
         .has_accuracy = true,
         .enabled = true,
         .is_primary = true}};
    r3d::set_targeting_panel_data(data);

    auto texts = std::vector<r3d::hud_text>{};
    r3d::draw_targeting_panel(
        {.renderer = renderer,
         .view_x = 0.0f,
         .view_y = 0.0f,
         .view_w = 1280.0f,
         .view_h = 720.0f,
         .texts = texts,
         .cell_w = 8.0f,
         .cell_h = 16.0f,
         .target_screen_pos = r3d::vec3{.x = 640.0f, .y = 260.0f, .z = 0.5f},
         .target_head_screen_pos = r3d::vec3{.x = 640.0f, .y = 210.0f, .z = 0.5f}});

    // Find the "[4] Precise" pill text and verify clicking it triggers SELECT_AIM_3
    auto precise_pos = std::optional<r3d::hud_text>{};
    for (const auto& t : texts) {
        if (t.text.find("Precise") != std::string::npos) {
            precise_pos = t;
            break;
        }
    }
    REQUIRE(precise_pos.has_value());
    CHECK(r3d::targeting_panel_mouse_move(precise_pos->x, precise_pos->y + 4.0f));
    REQUIRE(r3d::effective_targeting_hit_chance().has_value());
    CHECK(*r3d::effective_targeting_hit_chance() == 78);
    const auto clicked = r3d::targeting_panel_mouse_click(precise_pos->x, precise_pos->y + 4.0f);
    REQUIRE(clicked.has_value());
    CHECK(*clicked == "SELECT_AIM_3");

    r3d::clear_targeting_panel_data();
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
#endif
}
