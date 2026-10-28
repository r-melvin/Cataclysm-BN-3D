#pragma once
#ifndef CATA_SRC_R3D_TARGETING_PANEL_H
#define CATA_SRC_R3D_TARGETING_PANEL_H

#include <optional>
#include <string>
#include <vector>

#if defined(TILES)
#include <SDL3/SDL.h>
#include "coordinates.h"
#include "r3d.h"
#include "r3d_math.h"
#endif

namespace r3d
{

#if defined(TILES)

/// An individual combat action or aim preset displayed in the 3D targeting panel.
struct targeting_action_item {
    std::string action_id;       ///< Action ID to execute, e.g. "FIRE", "AIM", "AIMED_SHOT", etc.
    std::string
    label;           ///< Display name, e.g. "Current", "Aimed Shot", "Careful Shot", "Aim", "Mode"
    std::string
    shortcut_text;   ///< Visual shortcut label, e.g. "f", "a", "c", "p", ".", "s", "o", "Esc"
    int moves_cost = -1;         ///< Moves to attack/aim, or -1 if not applicable
    int chance_great = 0;        ///< Critical hit chance percentage (0-100)
    int chance_normal = 0;       ///< Normal hit chance percentage (0-100)
    int chance_graze = 0;        ///< Grazing hit chance percentage (0-100)
    bool has_accuracy = false;   ///< Whether this action provides accuracy numbers
    bool enabled = true;         ///< Whether this action is currently selectable
    bool is_aim_only = false;    ///< True if this is the aim-only action
    bool is_primary = false;     ///< True if this is the primary fire action
};

/// Combat information to display in the 3D compact targeting panel.
struct targeting_panel_data {
    std::string title;           ///< e.g. "Firing Glock 19", "Throwing Grenade"
    std::string mode_text;       ///< e.g. "Semi-Auto (1)"
    std::string ammo_text;       ///< e.g. "9x19mm (15/15)"
    std::string recoil_text;     ///< e.g. "Recoil: None"
    std::string target_name;     ///< Target creature/object name or ""
    std::string target_detail;   ///< Target health status, armor, or details
    int range = 0;               ///< Current distance to cursor/target
    int max_range = 0;           ///< Maximum range of weapon
    int elevation = 0;           ///< Elevation difference (dst.z - src.z)
    bool out_of_range = false;   ///< Whether target is beyond max range
    bool out_of_ammo = false;    ///< Whether weapon is out of ammo
    bool can_switch_mode = false;///< Whether switching gun mode is possible
    bool can_switch_ammo = false;///< Whether switching ammo is possible
    bool has_target = false;     ///< Whether a creature target is selected
    std::vector<targeting_action_item> aim_tiers;  ///< Inline aim tier pills ([1] Immediate .. [4] Precise)
    std::vector<targeting_action_item> presets;    ///< Aim presets & firing actions
    std::optional<targeting_action_item> aim_only; ///< Dedicated aim-only action
    std::vector<targeting_action_item>
    controls;   ///< Auxiliary controls: mode, ammo, prev, next, cancel
    std::optional<tripoint_bub_ms> player_pos{};   ///< Shooter position in bubble coordinates
    std::optional<tripoint_bub_ms> target_pos{};   ///< Aimed tile/creature position in bubble coordinates
    int hit_chance = 0;                            ///< Total percentage chance to hit (0-100)
    int great_chance = 0;                          ///< Critical hit percentage (0-100)
    int normal_chance = 0;                         ///< Normal hit percentage (0-100)
    int graze_chance = 0;                          ///< Grazing hit percentage (0-100)
    bool show_hit_chance = false;                  ///< Whether VATS hit percentage callout is shown
};

/// Registration and query API for 3D targeting panel
auto set_targeting_panel_data( const targeting_panel_data &data ) -> void;
auto clear_targeting_panel_data() -> void;
auto has_targeting_panel() -> bool;
auto get_targeting_panel_data() -> const targeting_panel_data &; // *NOPAD*
auto effective_targeting_hit_chance() -> std::optional<int>;

/// Checks if a screen pixel coordinate (x, y) falls inside the targeting panel bounds.
auto targeting_panel_contains( float x, float y ) -> bool;

/// Updates hover state on mouse move and returns true if hover changed.
auto targeting_panel_mouse_move( float x, float y ) -> bool;

/// Handles a mouse click at screen pixel (x, y). Returns the action ID if a button was clicked.
auto targeting_panel_mouse_click( float x, float y ) -> std::optional<std::string>;

struct draw_targeting_panel_options {
    SDL_Renderer *renderer;
    float view_x;
    float view_y;
    float view_w;
    float view_h;
    std::vector<hud_text> &texts;
    float cell_w;
    float cell_h;
    std::optional<vec3> target_screen_pos{};
    std::optional<vec3> target_head_screen_pos{};
};

/// Draws the compact graphical panel onto the SDL_Renderer.
auto draw_targeting_panel( const draw_targeting_panel_options &options ) -> void;

/// Transient shot feedback event emitted when a ranged weapon fires.
struct shot_fx_event {
    tripoint_bub_ms origin{};
    tripoint_bub_ms impact{};
    std::optional<tripoint_bub_ms> hit_creature{};
    int recoil = 0;
};

/// Color-graded RGBA8 light colour (pack_light format: R low byte, G byte 1, B byte 2, 0xFF byte 3)
/// for the 3D chest-height sightline and VATS accuracy bar (red -> amber -> bright green).
auto sightline_color_for_hit_chance( int hit_pct, bool out_of_range = false ) -> uint32_t;

/// Records a fired shot for 3D tracer, muzzle flash, target hit flash, and camera recoil impulse.
auto record_shot_fx( const shot_fx_event &event ) -> void;

/// Returns true while shot tracers, muzzle flashes, recoil spring, or VATS camera ease-out are animating.
auto combat_fx_active() -> bool;

#else

inline auto has_targeting_panel() -> bool { return false; }
inline auto targeting_panel_contains( float /*x*/, float /*y*/ ) -> bool { return false; }
inline auto targeting_panel_mouse_move( float /*x*/, float /*y*/ ) -> bool { return false; }
inline auto targeting_panel_mouse_click( float /*x*/, float /*y*/ ) -> std::optional<std::string> { return std::nullopt; }
inline auto combat_fx_active() -> bool { return false; }

#endif

} // namespace r3d

#endif // CATA_SRC_R3D_TARGETING_PANEL_H
