#include "r3d_targeting_panel.h"

#if defined(TILES)

#include <algorithm>
#include <cmath>

#include "catacharset.h"
#include "string_formatter.h"
#include "translations.h"

namespace r3d
{

namespace
{

struct clickable_button {
    SDL_FRect rect{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::string action_id;
    bool enabled = true;
};

auto s_active_panel = std::optional<targeting_panel_data> {};
auto s_panel_bounds = SDL_FRect{ 0.0f, 0.0f, 0.0f, 0.0f };
auto s_buttons = std::vector<clickable_button> {};
auto s_hovered_action = std::string{};

auto point_in_rect( float px, float py, const SDL_FRect &r ) -> bool
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

auto rects_overlap( const SDL_FRect &a, const SDL_FRect &b ) -> bool
{
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

auto text_px( const std::string &s, float cell_w ) -> float
{
    return static_cast<float>( utf8_width( s ) ) * cell_w;
}

auto fit_text_to_px( const std::string &s, float max_px, float cell_w ) -> std::string
{
    if( max_px <= 0.0f ) {
        return {};
    }
    const auto max_cells = static_cast<int>( std::floor( max_px / std::max( cell_w, 1.0f ) ) );
    if( max_cells <= 0 ) {
        return {};
    }
    const auto wrapped = utf8_wrapper( s );
    if( static_cast<int>( wrapped.display_width() ) <= max_cells ) {
        return s;
    }
    return wrapped.substr_display( 0, static_cast<size_t>( max_cells ) ).str();
}

auto format_status_line( const targeting_panel_data &data ) -> std::string
{
    auto status_line = data.mode_text;
    if( !data.ammo_text.empty() ) {
        if( !status_line.empty() ) {
            status_line += "  |  ";
        }
        status_line += data.ammo_text;
    }
    if( !data.recoil_text.empty() ) {
        if( !status_line.empty() ) {
            status_line += "  |  ";
        }
        status_line += data.recoil_text;
    }
    return status_line;
}

auto format_target_line( const targeting_panel_data &data ) -> std::string
{
    auto target_line = std::string{};
    if( data.out_of_range ) {
        target_line = string_format( _( "Range: %d/%d (OUT OF RANGE)" ), data.range, data.max_range );
    } else {
        target_line = string_format( _( "Range: %d/%d  Elev: %d" ), data.range, data.max_range,
                                     data.elevation );
    }
    if( data.has_target && !data.target_name.empty() ) {
        target_line += "  •  " + data.target_name;
        if( !data.target_detail.empty() ) {
            target_line += " (" + data.target_detail + ")";
        }
    }
    return target_line;
}

auto compute_panel_width( const targeting_panel_data &data, float cell_w, float view_w ) -> float
{
    auto needed_inner_w = 0.0f;
    needed_inner_w = std::max( needed_inner_w, text_px( data.title, cell_w ) );

    const auto status_line = format_status_line( data );
    if( !status_line.empty() ) {
        needed_inner_w = std::max( needed_inner_w, text_px( status_line, cell_w ) );
    }

    const auto target_line = format_target_line( data );
    needed_inner_w = std::max( needed_inner_w, text_px( target_line, cell_w ) );

    for( const auto &preset : data.presets ) {
        const auto left_label = string_format( "[%s] %s", preset.shortcut_text, preset.label );
        auto row_w = text_px( left_label, cell_w ) + 12.0f;
        if( preset.moves_cost >= 0 ) {
            const auto cost_label = string_format( _( "%d moves" ), preset.moves_cost );
            row_w += text_px( cost_label, cell_w ) + 14.0f;
        }
        if( preset.has_accuracy ) {
            const auto acc_label = string_format( "%d%% / %d%% / %d%%",
                                                  preset.chance_great, preset.chance_normal,
                                                  preset.chance_graze );
            row_w += text_px( acc_label, cell_w ) + 14.0f;
        }
        needed_inner_w = std::max( needed_inner_w, row_w );
    }

    if( data.aim_only.has_value() ) {
        const auto &aim = *data.aim_only;
        const auto aim_label = string_format( "[%s] %s", aim.shortcut_text, _( "Aim (steady aim)" ) );
        auto row_w = text_px( aim_label, cell_w ) + 12.0f;
        if( aim.moves_cost >= 0 ) {
            const auto cost_label = string_format( _( "%d moves" ), aim.moves_cost );
            row_w += text_px( cost_label, cell_w ) + 14.0f;
        }
        needed_inner_w = std::max( needed_inner_w, row_w );
    }

    if( !data.aim_tiers.empty() ) {
        auto max_tier_w = 0.0f;
        for( const auto &tier : data.aim_tiers ) {
            const auto tier_label = string_format( "[%s] %s", tier.shortcut_text, tier.label );
            max_tier_w = std::max( max_tier_w, text_px( tier_label, cell_w ) + 12.0f );
        }
        const auto n_tiers = static_cast<float>( data.aim_tiers.size() );
        const auto gap = 6.0f;
        needed_inner_w = std::max( needed_inner_w,
                                   n_tiers * max_tier_w + std::max( 0.0f, n_tiers - 1.0f ) * gap );
    }

    if( !data.controls.empty() ) {
        auto max_ctrl_w = 0.0f;
        for( const auto &ctrl : data.controls ) {
            const auto ctrl_label = string_format( "[%s] %s", ctrl.shortcut_text, ctrl.label );
            max_ctrl_w = std::max( max_ctrl_w, text_px( ctrl_label, cell_w ) + 12.0f );
        }
        const auto n_ctrl = static_cast<float>( data.controls.size() );
        const auto gap = 6.0f;
        needed_inner_w = std::max( needed_inner_w,
                                   n_ctrl * max_ctrl_w + std::max( 0.0f, n_ctrl - 1.0f ) * gap );
    }

    const auto desired_w = std::max( 380.0f, needed_inner_w + 20.0f );
    const auto max_w = std::max( 280.0f, view_w - 32.0f );
    return std::clamp( desired_w, 280.0f, max_w );
}

auto compute_panel_height( const targeting_panel_data &data, float cell_h ) -> float
{
    const auto line_h = std::max( cell_h, 14.0f );
    auto total_h = 16.0f; // top and bottom margins

    // Header: title + mode/ammo/recoil
    total_h += line_h + 2.0f;
    if( !data.mode_text.empty() || !data.ammo_text.empty() || !data.recoil_text.empty() ) {
        total_h += line_h + 4.0f;
    }
    total_h += 6.0f; // separator

    // Target info
    total_h += line_h + 6.0f;
    total_h += 6.0f; // separator

    // Presets
    const auto preset_btn_h = line_h + 6.0f;
    if( !data.aim_tiers.empty() ) {
        total_h += preset_btn_h + 4.0f;
        total_h += 6.0f; // separator
    }
    for( size_t i = 0; i < data.presets.size(); ++i ) {
        total_h += preset_btn_h + 4.0f;
    }

    // Aim-only action
    if( data.aim_only.has_value() ) {
        total_h += 6.0f; // separator
        total_h += preset_btn_h + 4.0f;
    }

    // Controls
    if( !data.controls.empty() ) {
        total_h += 6.0f; // separator
        total_h += preset_btn_h + 4.0f;
    }

    return total_h;
}

auto draw_target_vats_overlay( const draw_targeting_panel_options &options,
                               const targeting_panel_data &data ) -> void
{
    const auto hit_opt = effective_targeting_hit_chance();
    if( !hit_opt || ( !options.target_screen_pos && !options.target_head_screen_pos ) ) {
        return;
    }
    auto *const renderer = options.renderer;
    const auto view_x = options.view_x;
    const auto view_y = options.view_y;
    const auto view_w = options.view_w;
    const auto view_h = options.view_h;
    auto &texts = options.texts;
    const auto cell_w = std::max( options.cell_w, 6.0f );
    const auto line_h = std::max( options.cell_h, 14.0f );

    const auto hit_pct = std::clamp( *hit_opt, 0, 100 );
    const auto torso = options.target_screen_pos.value_or( *options.target_head_screen_pos );
    const auto head = options.target_head_screen_pos.value_or(
                          vec3{ .x = torso.x, .y = torso.y - 34.0f, .z = torso.z } );

    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // 1. VATS target reticle brackets around the enemy torso
    const auto rx = torso.x;
    const auto ry = torso.y;
    const auto half_r = 16.0f;
    const auto corner = 6.0f;
    SDL_SetRenderDrawColor( renderer, 245, 48, 42, 230 );
    // Top-left bracket
    SDL_RenderLine( renderer, rx - half_r, ry - half_r, rx - half_r + corner, ry - half_r );
    SDL_RenderLine( renderer, rx - half_r, ry - half_r, rx - half_r, ry - half_r + corner );
    // Top-right bracket
    SDL_RenderLine( renderer, rx + half_r - corner, ry - half_r, rx + half_r, ry - half_r );
    SDL_RenderLine( renderer, rx + half_r, ry - half_r, rx + half_r, ry - half_r + corner );
    // Bottom-left bracket
    SDL_RenderLine( renderer, rx - half_r, ry + half_r - corner, rx - half_r, ry + half_r );
    SDL_RenderLine( renderer, rx - half_r, ry + half_r, rx - half_r + corner, ry + half_r );
    // Bottom-right bracket
    SDL_RenderLine( renderer, rx + half_r - corner, ry + half_r, rx + half_r, ry + half_r );
    SDL_RenderLine( renderer, rx + half_r, ry + half_r - corner, rx + half_r, ry + half_r );

    // 2. VATS percentage callout badge above/on the targeted enemy
    const auto pct_text = string_format( "%d%%", hit_pct );
    const auto pct_w = text_px( pct_text, cell_w );
    const auto badge_w = std::max( pct_w + 24.0f, 76.0f );
    const auto badge_h = line_h + 18.0f;

    auto badge_x = std::clamp( head.x - badge_w * 0.5f, view_x + 8.0f,
                               std::max( view_x + 8.0f, view_x + view_w - badge_w - 8.0f ) );
    auto badge_y = std::clamp( head.y - badge_h - 12.0f, view_y + 28.0f,
                               std::max( view_y + 28.0f, view_y + view_h - badge_h - 8.0f ) );

    const auto padded_panel = SDL_FRect{
        s_panel_bounds.x - 6.0f, s_panel_bounds.y - 6.0f,
        s_panel_bounds.w + 12.0f, s_panel_bounds.h + 12.0f
    };
    const auto badge_rect_test = SDL_FRect{ badge_x, badge_y, badge_w, badge_h };
    if( s_panel_bounds.w > 0.0f && rects_overlap( badge_rect_test, padded_panel ) ) {
        if( s_panel_bounds.y - badge_h - 8.0f >= view_y + 28.0f ) {
            badge_y = s_panel_bounds.y - badge_h - 8.0f;
        } else {
            badge_x = std::min( std::max( view_x + 8.0f, view_x + view_w - badge_w - 8.0f ),
                                s_panel_bounds.x + s_panel_bounds.w + 8.0f );
        }
    }

    const auto badge_rect = SDL_FRect{ badge_x, badge_y, badge_w, badge_h };

    // Leader line from enemy to VATS callout box
    const auto anchor_x = std::clamp( head.x, badge_x + 8.0f, badge_x + badge_w - 8.0f );
    const auto anchor_y = badge_y + badge_h <= head.y ? ( badge_y + badge_h ) : badge_y;
    SDL_SetRenderDrawColor( renderer, 245, 55, 48, 210 );
    SDL_RenderLine( renderer, head.x, head.y, anchor_x, anchor_y );

    // Callout background and border
    SDL_SetRenderDrawColor( renderer, 22, 12, 14, 235 );
    SDL_RenderFillRect( renderer, &badge_rect );

    SDL_SetRenderDrawColor( renderer, 245, 55, 48, 245 );
    SDL_RenderRect( renderer, &badge_rect );

    // Segmented VATS accuracy bar inside the callout
    const auto bar_x = badge_x + 8.0f;
    const auto bar_y = badge_y + line_h + 6.0f;
    const auto bar_w = badge_w - 16.0f;
    const auto bar_h = 6.0f;
    const auto bar_bg = SDL_FRect{ bar_x, bar_y, bar_w, bar_h };
    SDL_SetRenderDrawColor( renderer, 48, 22, 24, 220 );
    SDL_RenderFillRect( renderer, &bar_bg );

    const auto fill_w = bar_w * ( static_cast<float>( hit_pct ) / 100.0f );
    if( fill_w > 0.5f ) {
        const auto bar_fill = SDL_FRect{ bar_x, bar_y, fill_w, bar_h };
        const auto col = sightline_color_for_hit_chance( hit_pct, data.out_of_range );
        const auto cr = static_cast<uint8_t>( col & 0xFFu );
        const auto cg = static_cast<uint8_t>( ( col >> 8u ) & 0xFFu );
        const auto cb = static_cast<uint8_t>( ( col >> 16u ) & 0xFFu );
        SDL_SetRenderDrawColor( renderer, cr, cg, cb, 245 );
        SDL_RenderFillRect( renderer, &bar_fill );
    }

    // Percentage label centered in the VATS callout
    const auto fit_pct = fit_text_to_px( pct_text, badge_w - 8.0f, cell_w );
    if( !fit_pct.empty() ) {
        const auto pct_color = ( data.out_of_range || hit_pct < 25 ) ? 9 : ( hit_pct < 65 ? 14 : 10 );
        texts.push_back( {
            .x = badge_x + badge_w * 0.5f,
            .y = badge_y + 3.0f,
            .text = fit_pct,
            .color = pct_color,
            .align = 0,
            .boxed = false
        } );
    }
}

} // namespace

auto set_targeting_panel_data( const targeting_panel_data &data ) -> void
{
    s_active_panel = data;
    if( !s_active_panel->show_hit_chance && !s_active_panel->presets.empty() ) {
        const targeting_action_item *chosen = nullptr;
        for( const auto &preset : s_active_panel->presets ) {
            if( preset.has_accuracy && preset.is_primary ) {
                chosen = &preset;
                break;
            }
        }
        if( !chosen ) {
            for( const auto &preset : s_active_panel->presets ) {
                if( preset.has_accuracy ) {
                    chosen = &preset;
                    break;
                }
            }
        }
        if( chosen ) {
            s_active_panel->great_chance = std::clamp( chosen->chance_great, 0, 100 );
            s_active_panel->normal_chance = std::clamp( chosen->chance_normal, 0, 100 );
            s_active_panel->graze_chance = std::clamp( chosen->chance_graze, 0, 100 );
            s_active_panel->hit_chance = std::clamp(
                                             chosen->chance_great + chosen->chance_normal + chosen->chance_graze, 0, 100 );
            s_active_panel->show_hit_chance = s_active_panel->has_target;
        }
    }
}

auto clear_targeting_panel_data() -> void
{
    s_active_panel.reset();
    s_panel_bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    s_buttons.clear();
    s_hovered_action.clear();
}

auto has_targeting_panel() -> bool
{
    return s_active_panel.has_value();
}

auto get_targeting_panel_data() -> const targeting_panel_data & // *NOPAD*
{
    static const auto empty = targeting_panel_data{};
    return s_active_panel ? *s_active_panel : empty;
}

auto sightline_color_for_hit_chance( int hit_pct, bool out_of_range ) -> uint32_t
{
    const auto pack = []( int r, int g, int b ) -> uint32_t {
        const auto cr = static_cast<uint32_t>( std::clamp( r, 0, 255 ) );
        const auto cg = static_cast<uint32_t>( std::clamp( g, 0, 255 ) );
        const auto cb = static_cast<uint32_t>( std::clamp( b, 0, 255 ) );
        return 0xFF000000u | ( cb << 16u ) | ( cg << 8u ) | cr;
    };
    if( out_of_range ) {
        return pack( 235, 55, 50 );
    }
    const auto pct = std::clamp( hit_pct, 0, 100 );
    if( pct <= 40 ) {
        const auto t = static_cast<float>( pct ) / 40.0f;
        const auto r = static_cast<int>( std::round( 235.0f + ( 245.0f - 235.0f ) * t ) );
        const auto g = static_cast<int>( std::round( 55.0f + ( 190.0f - 55.0f ) * t ) );
        const auto b = static_cast<int>( std::round( 50.0f + ( 55.0f - 50.0f ) * t ) );
        return pack( r, g, b );
    }
    const auto t = static_cast<float>( pct - 40 ) / 60.0f;
    const auto r = static_cast<int>( std::round( 245.0f + ( 85.0f - 245.0f ) * t ) );
    const auto g = static_cast<int>( std::round( 190.0f + ( 240.0f - 190.0f ) * t ) );
    const auto b = static_cast<int>( std::round( 55.0f + ( 110.0f - 55.0f ) * t ) );
    return pack( r, g, b );
}

auto effective_targeting_hit_chance() -> std::optional<int>
{
    if( !s_active_panel ) {
        return std::nullopt;
    }
    const auto &data = *s_active_panel;
    if( !s_hovered_action.empty() ) {
        for( const auto &tier : data.aim_tiers ) {
            if( tier.action_id == s_hovered_action && tier.has_accuracy ) {
                return std::clamp(
                           tier.chance_great + tier.chance_normal + tier.chance_graze, 0, 100 );
            }
        }
        for( const auto &preset : data.presets ) {
            if( preset.action_id == s_hovered_action && preset.has_accuracy ) {
                return std::clamp(
                           preset.chance_great + preset.chance_normal + preset.chance_graze, 0, 100 );
            }
        }
    }
    if( data.show_hit_chance ) {
        return std::clamp( data.hit_chance, 0, 100 );
    }
    if( data.has_target ) {
        for( const auto &preset : data.presets ) {
            if( preset.has_accuracy && preset.is_primary ) {
                return std::clamp(
                           preset.chance_great + preset.chance_normal + preset.chance_graze, 0, 100 );
            }
        }
        for( const auto &preset : data.presets ) {
            if( preset.has_accuracy ) {
                return std::clamp(
                           preset.chance_great + preset.chance_normal + preset.chance_graze, 0, 100 );
            }
        }
    }
    return std::nullopt;
}

auto targeting_panel_contains( float x, float y ) -> bool
{
    if( !s_active_panel || s_panel_bounds.w <= 0.0f || s_panel_bounds.h <= 0.0f ) {
        return false;
    }
    return point_in_rect( x, y, s_panel_bounds );
}

auto targeting_panel_mouse_move( float x, float y ) -> bool
{
    if( !s_active_panel ) {
        return false;
    }
    auto newly_hovered = std::string{};
    for( const auto &btn : s_buttons ) {
        if( btn.enabled && point_in_rect( x, y, btn.rect ) ) {
            newly_hovered = btn.action_id;
            break;
        }
    }
    if( newly_hovered != s_hovered_action ) {
        s_hovered_action = newly_hovered;
        return true;
    }
    return false;
}

auto targeting_panel_mouse_click( float x, float y ) -> std::optional<std::string>
{
    if( !s_active_panel ) {
        return std::nullopt;
    }
    for( const auto &btn : s_buttons ) {
        if( btn.enabled && point_in_rect( x, y, btn.rect ) ) {
            return btn.action_id;
        }
    }
    return std::nullopt;
}

auto draw_targeting_panel( const draw_targeting_panel_options &options ) -> void
{
    if( !s_active_panel ) {
        return;
    }
    const auto &data = *s_active_panel;
    auto *const renderer = options.renderer;
    const auto view_x = options.view_x;
    const auto view_y = options.view_y;
    const auto view_w = options.view_w;
    const auto view_h = options.view_h;
    auto &texts = options.texts;
    const auto cell_w = std::max( options.cell_w, 6.0f );
    const auto cell_h = std::max( options.cell_h, 14.0f );
    const auto line_h = cell_h;

    const auto panel_w = compute_panel_width( data, cell_w, view_w );
    const auto panel_h = compute_panel_height( data, line_h );
    const auto panel_x = view_x + 16.0f;
    const auto panel_y = view_y + view_h - panel_h - 16.0f;

    s_panel_bounds = { panel_x, panel_y, panel_w, panel_h };
    s_buttons.clear();

    // 1. Draw outer panel background and border
    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SDL_SetRenderDrawColor( renderer, 16, 20, 26, 230 );
    SDL_RenderFillRect( renderer, &s_panel_bounds );

    SDL_SetRenderDrawColor( renderer, 65, 85, 115, 240 );
    SDL_RenderRect( renderer, &s_panel_bounds );

    const auto inner_x = panel_x + 10.0f;
    const auto inner_w = std::max( 40.0f, panel_w - 20.0f );
    auto cur_y = panel_y + 8.0f;

    // 2. Title
    const auto fitted_title = fit_text_to_px( data.title, inner_w, cell_w );
    if( !fitted_title.empty() ) {
        texts.push_back( {
            .x = inner_x,
            .y = cur_y,
            .text = fitted_title,
            .color = 15,
            .align = -1,
            .boxed = false
        } );
    }
    cur_y += line_h + 2.0f;

    // 3. Mode, ammo, and recoil info
    const auto status_line = format_status_line( data );
    if( !status_line.empty() ) {
        const auto fitted_status = fit_text_to_px( status_line, inner_w, cell_w );
        if( !fitted_status.empty() ) {
            texts.push_back( {
                .x = inner_x,
                .y = cur_y,
                .text = fitted_status,
                .color = data.out_of_ammo ? 1 : 6,
                .align = -1,
                .boxed = false
            } );
        }
        cur_y += line_h + 4.0f;
    }

    // Separator line
    SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
    SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
    cur_y += 6.0f;

    // 4. Target information row
    const auto target_line = format_target_line( data );
    const auto fitted_target = fit_text_to_px( target_line, inner_w, cell_w );
    if( !fitted_target.empty() ) {
        texts.push_back( {
            .x = inner_x,
            .y = cur_y,
            .text = fitted_target,
            .color = data.out_of_range ? 1 : 7,
            .align = -1,
            .boxed = false
        } );
    }
    cur_y += line_h + 6.0f;

    // Separator line
    SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
    SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
    cur_y += 6.0f;

    // 5. Inline Aim Tier pills ([1] Immediate, [2] Regular, [3] Careful, [4] Precise)
    const auto btn_h = line_h + 6.0f;
    const auto avail_btn_w = std::max( 20.0f, inner_w - 12.0f );
    if( !data.aim_tiers.empty() ) {
        const auto n_tiers = static_cast<float>( data.aim_tiers.size() );
        const auto gap = 6.0f;
        const auto tier_w = ( inner_w - ( n_tiers - 1.0f ) * gap ) / n_tiers;

        for( size_t i = 0; i < data.aim_tiers.size(); ++i ) {
            const auto &tier = data.aim_tiers[i];
            const auto t_rect = SDL_FRect{
                inner_x + static_cast<float>( i ) *( tier_w + gap ),
                cur_y, tier_w, btn_h
            };
            s_buttons.push_back( { .rect = t_rect, .action_id = tier.action_id, .enabled = tier.enabled } );
            const auto is_hovered = ( s_hovered_action == tier.action_id );

            if( !tier.enabled ) {
                SDL_SetRenderDrawColor( renderer, 20, 24, 30, 150 );
            } else if( tier.is_primary ) {
                SDL_SetRenderDrawColor( renderer, 42, 76, 54, 235 );
            } else if( is_hovered ) {
                SDL_SetRenderDrawColor( renderer, 45, 70, 105, 230 );
            } else {
                SDL_SetRenderDrawColor( renderer, 24, 32, 44, 185 );
            }
            SDL_RenderFillRect( renderer, &t_rect );

            if( tier.is_primary ) {
                SDL_SetRenderDrawColor( renderer, 110, 235, 135, 255 );
            } else if( is_hovered ) {
                SDL_SetRenderDrawColor( renderer, 130, 185, 245, 255 );
            } else {
                SDL_SetRenderDrawColor( renderer, 55, 75, 100, 190 );
            }
            SDL_RenderRect( renderer, &t_rect );

            const auto max_tier_px = std::max( 0.0f, t_rect.w - 6.0f );
            auto tier_label = string_format( "[%s] %s", tier.shortcut_text, tier.label );
            if( text_px( tier_label, cell_w ) > max_tier_px ) {
                tier_label = fit_text_to_px( tier_label, max_tier_px, cell_w );
            }
            if( !tier_label.empty() ) {
                texts.push_back( {
                    .x = t_rect.x + t_rect.w * 0.5f,
                    .y = cur_y + 3.0f,
                    .text = tier_label,
                    .color = tier.enabled ? ( tier.is_primary ? 10 : 15 ) : 8,
                                   .align = 0,
                                   .boxed = false
                } );
            }
        }

        cur_y += btn_h + 4.0f;
        SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
        SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
        cur_y += 6.0f;
    }

    // 6. Presets list (Clickable buttons)
    for( const auto &preset : data.presets ) {
        const auto btn_rect = SDL_FRect{ inner_x, cur_y, inner_w, btn_h };
        s_buttons.push_back( { .rect = btn_rect, .action_id = preset.action_id, .enabled = preset.enabled } );
        const auto is_hovered = ( s_hovered_action == preset.action_id );

        // Button background
        if( !preset.enabled ) {
            SDL_SetRenderDrawColor( renderer, 20, 24, 30, 150 );
        } else if( is_hovered ) {
            SDL_SetRenderDrawColor( renderer, 45, 70, 105, 230 );
        } else if( preset.is_primary ) {
            SDL_SetRenderDrawColor( renderer, 32, 48, 68, 200 );
        } else {
            SDL_SetRenderDrawColor( renderer, 24, 32, 44, 180 );
        }
        SDL_RenderFillRect( renderer, &btn_rect );

        // Button border
        if( is_hovered ) {
            SDL_SetRenderDrawColor( renderer, 130, 185, 245, 255 );
        } else if( preset.is_primary ) {
            SDL_SetRenderDrawColor( renderer, 80, 125, 175, 220 );
        } else {
            SDL_SetRenderDrawColor( renderer, 50, 70, 95, 180 );
        }
        SDL_RenderRect( renderer, &btn_rect );

        auto acc_text = std::string{};
        auto acc_w = 0.0f;
        if( preset.has_accuracy ) {
            const auto raw_acc = string_format( "%d%% / %d%% / %d%%",
                                                preset.chance_great, preset.chance_normal, preset.chance_graze );
            acc_text = fit_text_to_px( raw_acc, avail_btn_w * 0.48f, cell_w );
            acc_w = text_px( acc_text, cell_w );
        }

        auto cost_text = std::string{};
        auto cost_w = 0.0f;
        if( preset.moves_cost >= 0 ) {
            const auto raw_cost = string_format( _( "%d moves" ), preset.moves_cost );
            cost_text = fit_text_to_px( raw_cost, avail_btn_w * 0.28f, cell_w );
            cost_w = text_px( cost_text, cell_w );
        }

        const auto right_reserved = ( acc_w > 0.0f ? ( acc_w + 10.0f ) : 0.0f );
        const auto cost_reserved = ( cost_w > 0.0f ? ( cost_w + 10.0f ) : 0.0f );
        const auto max_left_w = std::max( 0.0f, avail_btn_w - right_reserved - cost_reserved );
        const auto raw_left = string_format( "[%s] %s", preset.shortcut_text, preset.label );
        const auto left_text = fit_text_to_px( raw_left, max_left_w, cell_w );
        const auto left_w = text_px( left_text, cell_w );

        if( !left_text.empty() ) {
            texts.push_back( {
                .x = inner_x + 6.0f,
                .y = cur_y + 3.0f,
                .text = left_text,
                .color = preset.enabled ? ( preset.is_primary ? 15 : 7 ) : 8,
                               .align = -1,
                               .boxed = false
            } );
        }

        if( !cost_text.empty() ) {
            const auto cost_min_x = inner_x + 6.0f + left_w + ( left_w > 0.0f ? 10.0f : 0.0f );
            const auto cost_max_x = std::max( cost_min_x,
                                              inner_x + inner_w - 6.0f - right_reserved - cost_w );
            const auto cost_x = std::clamp( inner_x + inner_w * 0.44f, cost_min_x, cost_max_x );
            texts.push_back( {
                .x = cost_x,
                .y = cur_y + 3.0f,
                .text = cost_text,
                .color = preset.enabled ? 14 : 8,
                .align = -1,
                .boxed = false
            } );
        }

        if( !acc_text.empty() ) {
            texts.push_back( {
                .x = inner_x + inner_w - 6.0f,
                .y = cur_y + 3.0f,
                .text = acc_text,
                .color = preset.enabled ? 10 : 8,
                .align = 1,
                .boxed = false
            } );
        }

        cur_y += btn_h + 4.0f;
    }

    // 6. Aim-Only Action (clearly separated section)
    if( data.aim_only.has_value() ) {
        const auto &aim = *data.aim_only;
        SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
        SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
        cur_y += 6.0f;

        const auto aim_rect = SDL_FRect{ inner_x, cur_y, inner_w, btn_h };
        s_buttons.push_back( { .rect = aim_rect, .action_id = aim.action_id, .enabled = aim.enabled } );
        const auto is_hovered = ( s_hovered_action == aim.action_id );

        if( !aim.enabled ) {
            SDL_SetRenderDrawColor( renderer, 18, 26, 32, 150 );
        } else if( is_hovered ) {
            SDL_SetRenderDrawColor( renderer, 35, 80, 105, 230 );
        } else {
            SDL_SetRenderDrawColor( renderer, 22, 48, 64, 190 );
        }
        SDL_RenderFillRect( renderer, &aim_rect );

        if( is_hovered ) {
            SDL_SetRenderDrawColor( renderer, 90, 210, 245, 255 );
        } else {
            SDL_SetRenderDrawColor( renderer, 55, 145, 180, 220 );
        }
        SDL_RenderRect( renderer, &aim_rect );

        auto cost_text = std::string{};
        auto cost_w = 0.0f;
        if( aim.moves_cost >= 0 ) {
            const auto raw_cost = string_format( _( "%d moves" ), aim.moves_cost );
            cost_text = fit_text_to_px( raw_cost, avail_btn_w * 0.40f, cell_w );
            cost_w = text_px( cost_text, cell_w );
        }

        const auto max_aim_w = std::max( 0.0f,
                                         avail_btn_w - ( cost_w > 0.0f ? ( cost_w + 10.0f ) : 0.0f ) );
        const auto raw_aim = string_format( "[%s] %s", aim.shortcut_text, _( "Aim (steady aim)" ) );
        const auto aim_label = fit_text_to_px( raw_aim, max_aim_w, cell_w );
        if( !aim_label.empty() ) {
            texts.push_back( {
                .x = inner_x + 6.0f,
                .y = cur_y + 3.0f,
                .text = aim_label,
                .color = aim.enabled ? 14 : 8,
                .align = -1,
                .boxed = false
            } );
        }

        if( !cost_text.empty() ) {
            texts.push_back( {
                .x = inner_x + inner_w - 6.0f,
                .y = cur_y + 3.0f,
                .text = cost_text,
                .color = aim.enabled ? 14 : 8,
                .align = 1,
                .boxed = false
            } );
        }

        cur_y += btn_h + 4.0f;
    }

    // 7. Auxiliary controls row (Mode, Ammo, Prev, Next, Cancel)
    if( !data.controls.empty() ) {
        SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
        SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
        cur_y += 6.0f;

        const auto n_ctrl = static_cast<float>( data.controls.size() );
        const auto gap = 6.0f;
        const auto ctrl_w = ( inner_w - ( n_ctrl - 1.0f ) * gap ) / n_ctrl;

        for( size_t i = 0; i < data.controls.size(); ++i ) {
            const auto &ctrl = data.controls[i];
            const auto c_rect = SDL_FRect{ inner_x + static_cast<float>( i ) *( ctrl_w + gap ), cur_y, ctrl_w, btn_h };
            s_buttons.push_back( { .rect = c_rect, .action_id = ctrl.action_id, .enabled = ctrl.enabled } );
            const auto is_hovered = ( s_hovered_action == ctrl.action_id );

            if( !ctrl.enabled ) {
                SDL_SetRenderDrawColor( renderer, 20, 24, 30, 150 );
            } else if( is_hovered ) {
                SDL_SetRenderDrawColor( renderer, 52, 68, 90, 230 );
            } else {
                SDL_SetRenderDrawColor( renderer, 28, 36, 48, 180 );
            }
            SDL_RenderFillRect( renderer, &c_rect );

            if( is_hovered ) {
                SDL_SetRenderDrawColor( renderer, 140, 180, 220, 255 );
            } else {
                SDL_SetRenderDrawColor( renderer, 60, 80, 105, 180 );
            }
            SDL_RenderRect( renderer, &c_rect );

            const auto max_ctrl_px = std::max( 0.0f, c_rect.w - 6.0f );
            auto ctrl_label = string_format( "[%s] %s", ctrl.shortcut_text, ctrl.label );
            if( text_px( ctrl_label, cell_w ) > max_ctrl_px ) {
                if( !ctrl.label.empty() && text_px( ctrl.label, cell_w ) <= max_ctrl_px ) {
                    ctrl_label = ctrl.label;
                } else {
                    ctrl_label = fit_text_to_px( ctrl_label, max_ctrl_px, cell_w );
                }
            }
            if( !ctrl_label.empty() ) {
                texts.push_back( {
                    .x = c_rect.x + c_rect.w * 0.5f,
                    .y = cur_y + 3.0f,
                    .text = ctrl_label,
                    .color = ctrl.enabled ? 15 : 8,
                    .align = 0,
                    .boxed = false
                } );
            }
        }
    }

    // 8. Draw Fallout 4 VATS-inspired hit chance overlay on the targeted enemy
    draw_target_vats_overlay( options, data );
}

} // namespace r3d

#endif // defined(TILES)
