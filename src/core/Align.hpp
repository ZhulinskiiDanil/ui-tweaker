#pragma once

// Lining up several boxes on the screen, as Figma does: their left (or other) edges or centers to the same line,
// or the same gap between each two. Each result is how far a box moves on the screen
#include "Geometry.hpp"

#include <vector>

namespace align
{
  enum class Edge
  {
    Left,
    CenterX,
    Right,
    Bottom,
    CenterY,
    Top,
  };

  // The boxes' `edge` to the same edge of `target`: the box of them all when there are several, the parent's or
  // the screen's for one
  std::vector<geom::Vec2> to(std::vector<geom::Rect> const& boxes, Edge edge, geom::Rect const& target);

  // The box of them all
  geom::Rect bounds(std::vector<geom::Rect> const& boxes);

  // The same gap between each two neighbours along one axis, the first and the last staying where they are.
  // Nothing moves for fewer than three
  std::vector<geom::Vec2> distribute(std::vector<geom::Rect> const& boxes, bool horizontal);
} // namespace align
