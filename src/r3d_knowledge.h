#pragma once
#ifndef CATA_SRC_R3D_KNOWLEDGE_H
#define CATA_SRC_R3D_KNOWLEDGE_H

#include <string>
#include "coordinates.h"

class avatar;
class map;

namespace r3d
{
/// Name a currently perceived tile, or its recorded memory, without reading unseen live content.
auto crosshair_description( avatar &you, map &here, const tripoint_bub_ms &p ) -> std::string;
}
#endif
