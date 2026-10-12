#pragma once

// Snapping a dragged box to guides, as in Figma: its edges and its center to the screen's edges and center and
// to those of the boxes around it, with the lines to draw. All on the screen, in points
#include "Geometry.hpp"

#include <vector>

namespace snap
{
  // A line something can snap to: x = at (vertical) or y = at (horizontal), drawn from `from` to `to` along it
  struct Guide
  {
    bool vertical = true;
    float at = 0;
    float from = 0;
    float to = 0;
  };

  struct Result
  {
    // What to add to the box's place on the screen
    geom::Vec2 correction;
    // The guides it's on after the correction, stretched to reach the box
    std::vector<Guide> lines;
  };

  // The guides of a box: its two edges and its center on each axis
  void addBox(std::vector<Guide>& guides, geom::Rect const& box);

  // The closest guide within `threshold` on each axis, for any of the box's edges or its center. Ties go to the
  // first guide given
  Result snap(geom::Rect const& box, std::vector<Guide> const& guides, float threshold);
} // namespace snap
