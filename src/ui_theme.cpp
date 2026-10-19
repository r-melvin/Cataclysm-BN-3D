#include "ui_theme.h"
#include "options.h"

auto get_ui_theme() -> const ui_theme & // *NOPAD*
{
    static const auto classic = ui_theme{};
    static const auto modern = ui_theme{ .modern_borders = true, .border = c_dark_gray, .title = c_light_cyan };
    return get_options().has_option( "MODERN_MENUS" ) &&
           get_option<bool>( "MODERN_MENUS" ) ? modern : classic;
}
