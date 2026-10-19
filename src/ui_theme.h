#pragma once
#ifndef CATA_SRC_UI_THEME_H
#define CATA_SRC_UI_THEME_H

#include "color.h"

/// Native BN menu colours, selected by the Modern menu borders option.
struct ui_theme {
    // Curses screens: draw_border() boxes drawn in the classic border colour use border instead,
    // and their titles (in the classic title colour) use title, padded by a space on each side.
    bool modern_borders = false;
    nc_color border = c_light_gray;
    nc_color title = c_light_red;
};

auto get_ui_theme() -> const ui_theme &; // *NOPAD*

#endif // CATA_SRC_UI_THEME_H
