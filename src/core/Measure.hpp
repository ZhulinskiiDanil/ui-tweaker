#pragma once

// Distances between two boxes on the screen, as Figma shows them with Alt: the gaps between them, or from the
// inner box's edges to the outer box's if one is inside the other
#include "Geometry.hpp"

#include <vector>

namespace measure
{
  struct Segment
  {
    geom::Vec2 from;
    geom::Vec2 to;
    float length = 0;
  };

  // From `selected` to `other`. Nothing if they're the same box
  std::vector<Segment> between(geom::Rect const& selected, geom::Rect const& other);
} // namespace measure
