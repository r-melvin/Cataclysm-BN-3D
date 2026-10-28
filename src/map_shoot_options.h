#pragma once
#ifndef CATA_SRC_MAP_SHOOT_OPTIONS_H
#define CATA_SRC_MAP_SHOOT_OPTIONS_H

#include "coordinates.h"

struct projectile;

/// Cover at a physical wall edge is processed before creatures on its far side.
struct map_shoot_options {
    tripoint_bub_ms origin;
    tripoint_bub_ms pos;
    projectile &shot;
    bool hit_items = false;
    bool hit_terrain = true;
};

#endif // CATA_SRC_MAP_SHOOT_OPTIONS_H
