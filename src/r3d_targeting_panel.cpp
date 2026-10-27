#include "r3d_targeting_panel.h"

#if defined(TILES)

#include <algorithm>
#include <cmath>

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

} // namespace

auto set_targeting_panel_data( const targeting_panel_data &data ) -> void
{
    s_active_panel = data;
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
    const auto cell_h = std::max( options.cell_h, 14.0f );
    const auto line_h = cell_h;

    const auto panel_w = std::clamp( 380.0f, 280.0f, view_w - 32.0f );
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
    const auto inner_w = panel_w - 20.0f;
    auto cur_y = panel_y + 8.0f;

    // 2. Title
    texts.push_back( {
        .x = inner_x,
        .y = cur_y,
        .text = data.title,
        .color = 15,
        .align = -1,
        .boxed = false
    } );
    cur_y += line_h + 2.0f;

    // 3. Mode, ammo, and recoil info
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
    if( !status_line.empty() ) {
        texts.push_back( {
            .x = inner_x,
            .y = cur_y,
            .text = status_line,
            .color = data.out_of_ammo ? 1 : 6,
            .align = -1,
            .boxed = false
        } );
        cur_y += line_h + 4.0f;
    }

    // Separator line
    SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
    SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
    cur_y += 6.0f;

    // 4. Target information row
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
    texts.push_back( {
        .x = inner_x,
        .y = cur_y,
        .text = target_line,
        .color = data.out_of_range ? 1 : 7,
        .align = -1,
        .boxed = false
    } );
    cur_y += line_h + 6.0f;

    // Separator line
    SDL_SetRenderDrawColor( renderer, 45, 60, 80, 180 );
    SDL_RenderLine( renderer, inner_x, cur_y, inner_x + inner_w, cur_y );
    cur_y += 6.0f;

    // 5. Presets list (Clickable buttons)
    const auto btn_h = line_h + 6.0f;
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

        // Button texts
        const auto left_label = string_format( "[%s] %s", preset.shortcut_text, preset.label );
        texts.push_back( {
            .x = inner_x + 6.0f,
            .y = cur_y + 3.0f,
            .text = left_label,
            .color = preset.enabled ? ( preset.is_primary ? 15 : 7 ) : 8,
                           .align = -1,
                           .boxed = false
        } );

        if( preset.moves_cost >= 0 ) {
            const auto cost_label = string_format( _( "%d moves" ), preset.moves_cost );
            texts.push_back( {
                .x = inner_x + inner_w * 0.46f,
                .y = cur_y + 3.0f,
                .text = cost_label,
                .color = preset.enabled ? 14 : 8,
                .align = -1,
                .boxed = false
            } );
        }

        if( preset.has_accuracy ) {
            const auto acc_label = string_format( "%d%% / %d%% / %d%%",
                                                  preset.chance_great, preset.chance_normal, preset.chance_graze );
            texts.push_back( {
                .x = inner_x + inner_w - 6.0f,
                .y = cur_y + 3.0f,
                .text = acc_label,
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

        const auto aim_label = string_format( "[%s] %s", aim.shortcut_text, _( "Aim (steady aim)" ) );
        texts.push_back( {
            .x = inner_x + 6.0f,
            .y = cur_y + 3.0f,
            .text = aim_label,
            .color = aim.enabled ? 14 : 8,
            .align = -1,
            .boxed = false
        } );

        if( aim.moves_cost >= 0 ) {
            const auto cost_label = string_format( _( "%d moves" ), aim.moves_cost );
            texts.push_back( {
                .x = inner_x + inner_w - 6.0f,
                .y = cur_y + 3.0f,
                .text = cost_label,
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

            const auto ctrl_label = string_format( "[%s] %s", ctrl.shortcut_text, ctrl.label );
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

} // namespace r3d

#endif // defined(TILES)
