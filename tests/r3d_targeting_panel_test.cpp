#include "catch/catch.hpp"

#include <optional>
#include <string>
#include <vector>

#include "r3d.h"
#include "r3d_targeting_panel.h"

#if defined(TILES)
#include <SDL3/SDL.h>
#endif

TEST_CASE( "r3d_targeting_panel_lifecycle_and_data_preservation", "[r3d]" )
{
#if defined(TILES)
    r3d::clear_targeting_panel_data();
    CHECK_FALSE( r3d::has_targeting_panel() );

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
        {
            .action_id = "FIRE",
            .label = "Current",
            .shortcut_text = "f",
            .moves_cost = 80,
            .chance_great = 25,
            .chance_normal = 55,
            .chance_graze = 15,
            .has_accuracy = true,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = true
        },
        {
            .action_id = "AIMED_SHOT",
            .label = "Aimed Shot",
            .shortcut_text = "a",
            .moves_cost = 120,
            .chance_great = 40,
            .chance_normal = 50,
            .chance_graze = 8,
            .has_accuracy = true,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        },
        {
            .action_id = "CAREFUL_SHOT",
            .label = "Careful Shot",
            .shortcut_text = "c",
            .moves_cost = 160,
            .chance_great = 55,
            .chance_normal = 40,
            .chance_graze = 4,
            .has_accuracy = true,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        },
        {
            .action_id = "PRECISE_SHOT",
            .label = "Precise Shot",
            .shortcut_text = "p",
            .moves_cost = 210,
            .chance_great = 70,
            .chance_normal = 28,
            .chance_graze = 1,
            .has_accuracy = true,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        }
    };

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
        .is_primary = false
    };

    data.controls = {
        {
            .action_id = "SWITCH_MODE",
            .label = "Mode",
            .shortcut_text = "s",
            .moves_cost = -1,
            .chance_great = 0,
            .chance_normal = 0,
            .chance_graze = 0,
            .has_accuracy = false,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        },
        {
            .action_id = "PREV_TARGET",
            .label = "< Prev",
            .shortcut_text = "<",
            .moves_cost = -1,
            .chance_great = 0,
            .chance_normal = 0,
            .chance_graze = 0,
            .has_accuracy = false,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        },
        {
            .action_id = "NEXT_TARGET",
            .label = "Next >",
            .shortcut_text = ">",
            .moves_cost = -1,
            .chance_great = 0,
            .chance_normal = 0,
            .chance_graze = 0,
            .has_accuracy = false,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        },
        {
            .action_id = "QUIT",
            .label = "Cancel",
            .shortcut_text = "Esc",
            .moves_cost = -1,
            .chance_great = 0,
            .chance_normal = 0,
            .chance_graze = 0,
            .has_accuracy = false,
            .enabled = true,
            .is_aim_only = false,
            .is_primary = false
        }
    };

    r3d::set_targeting_panel_data( data );
    REQUIRE( r3d::has_targeting_panel() );

    const auto &retrieved = r3d::get_targeting_panel_data();
    CHECK( retrieved.title == "Firing Glock 19" );
    CHECK( retrieved.mode_text == "Semi-Auto (1)" );
    CHECK( retrieved.ammo_text == "9x19mm (15/15)" );
    CHECK( retrieved.range == 5 );
    CHECK( retrieved.max_range == 14 );
    CHECK( retrieved.target_name == "Zombie" );

    REQUIRE( retrieved.presets.size() == 4 );
    CHECK( retrieved.presets[0].label == "Current" );
    CHECK( retrieved.presets[0].shortcut_text == "f" );
    CHECK( retrieved.presets[0].moves_cost == 80 );
    CHECK( retrieved.presets[0].chance_great == 25 );

    CHECK( retrieved.presets[1].label == "Aimed Shot" );
    CHECK( retrieved.presets[1].shortcut_text == "a" );
    CHECK( retrieved.presets[1].moves_cost == 120 );

    CHECK( retrieved.presets[2].label == "Careful Shot" );
    CHECK( retrieved.presets[2].shortcut_text == "c" );
    CHECK( retrieved.presets[2].moves_cost == 160 );

    CHECK( retrieved.presets[3].label == "Precise Shot" );
    CHECK( retrieved.presets[3].shortcut_text == "p" );
    CHECK( retrieved.presets[3].moves_cost == 210 );

    REQUIRE( retrieved.aim_only.has_value() );
    CHECK( retrieved.aim_only->action_id == "AIM" );
    CHECK( retrieved.aim_only->is_aim_only );
    CHECK( retrieved.aim_only->moves_cost == 10 );
    CHECK( retrieved.aim_only->shortcut_text == "." );

    r3d::clear_targeting_panel_data();
    CHECK_FALSE( r3d::has_targeting_panel() );
    CHECK_FALSE( r3d::targeting_panel_contains( 50.0f, 50.0f ) );
#endif
}

TEST_CASE( "r3d_targeting_panel_hit_testing_and_controls", "[r3d]" )
{
#if defined(TILES)
    r3d::clear_targeting_panel_data();

    // Create a software SDL renderer for testing drawing and hit bounds
    auto *surface = SDL_CreateSurface( 800, 600, SDL_PIXELFORMAT_RGBA8888 );
    REQUIRE( surface != nullptr );
    auto *renderer = SDL_CreateSoftwareRenderer( surface );
    REQUIRE( renderer != nullptr );

    auto data = r3d::targeting_panel_data{};
    data.title = "Firing Rifle";
    data.mode_text = "Burst (3)";
    data.ammo_text = "5.56mm (30/30)";
    data.range = 8;
    data.max_range = 20;

    data.presets = {
        {
            .action_id = "FIRE",
            .label = "Current",
            .shortcut_text = "f",
            .moves_cost = 90,
            .has_accuracy = true,
            .enabled = true,
            .is_primary = true
        },
        {
            .action_id = "AIMED_SHOT",
            .label = "Aimed Shot",
            .shortcut_text = "a",
            .moves_cost = 135,
            .has_accuracy = true,
            .enabled = true
        }
    };

    data.aim_only = r3d::targeting_action_item{
        .action_id = "AIM",
        .label = "Aim",
        .shortcut_text = ".",
        .moves_cost = 10,
        .enabled = true,
        .is_aim_only = true
    };

    data.controls = {
        {
            .action_id = "SWITCH_MODE",
            .label = "Mode",
            .shortcut_text = "s",
            .enabled = true
        },
        {
            .action_id = "QUIT",
            .label = "Cancel",
            .shortcut_text = "Esc",
            .enabled = true
        }
    };

    r3d::set_targeting_panel_data( data );

    auto texts = std::vector<r3d::hud_text>{};
    const auto draw_opts = r3d::draw_targeting_panel_options{
        .renderer = renderer,
        .view_x = 0.0f,
        .view_y = 0.0f,
        .view_w = 800.0f,
        .view_h = 600.0f,
        .texts = texts,
        .cell_w = 8.0f,
        .cell_h = 16.0f
    };
    r3d::draw_targeting_panel( draw_opts );

    // Texts should have been queued
    CHECK_FALSE( texts.empty() );

    // Panel is positioned at bottom-left: view_x + 16 = 16.0f, width is clamped to 380.0f
    // Points far away outside the panel should NOT be contained
    CHECK_FALSE( r3d::targeting_panel_contains( 500.0f, 100.0f ) );
    CHECK_FALSE( r3d::targeting_panel_contains( 0.0f, 0.0f ) );

    // Point inside the panel bounds
    const auto panel_center_x = 16.0f + 190.0f; // 206.0f
    const auto panel_bottom_y = 600.0f - 30.0f; // 570.0f (inside panel margin)
    CHECK( r3d::targeting_panel_contains( panel_center_x, panel_bottom_y ) );

    // Hover move over Preset 1 button (y ~ 470.0f)
    const auto preset_btn_y = 470.0f;
    CHECK( r3d::targeting_panel_mouse_move( panel_center_x, preset_btn_y ) );
    // Moving again within the same button does not trigger a hover change
    CHECK_FALSE( r3d::targeting_panel_mouse_move( panel_center_x + 10.0f, preset_btn_y ) );

    // Click on Preset 1 button
    const auto preset_click = r3d::targeting_panel_mouse_click( panel_center_x, preset_btn_y );
    REQUIRE( preset_click.has_value() );
    CHECK( *preset_click == "FIRE" );

    // Click on Cancel button (bottom auxiliary row: x ~ 286.0f, y ~ 556.0f)
    const auto cancel_btn_x = panel_center_x + 80.0f;
    const auto cancel_btn_y = 556.0f;
    const auto click_res = r3d::targeting_panel_mouse_click( cancel_btn_x, cancel_btn_y );
    REQUIRE( click_res.has_value() );
    CHECK( *click_res == "QUIT" );

    // Moving hover over Cancel button changes hover
    CHECK( r3d::targeting_panel_mouse_move( cancel_btn_x, cancel_btn_y ) );
    // Moving hover to empty margin inside panel clears button hover (returns true)
    const auto panel_margin_x = 20.0f;
    const auto panel_margin_y = 385.0f;
    CHECK( r3d::targeting_panel_mouse_move( panel_margin_x, panel_margin_y ) );
    // Moving within empty margin again leaves hover unchanged (returns false)
    CHECK_FALSE( r3d::targeting_panel_mouse_move( panel_margin_x + 2.0f, panel_margin_y ) );

    r3d::clear_targeting_panel_data();
    SDL_DestroyRenderer( renderer );
    SDL_DestroySurface( surface );
#endif
}
