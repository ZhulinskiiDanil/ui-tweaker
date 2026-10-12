#pragma once

// Other screens' shapes: where a box would be on a wider or narrower screen, and whether it would still fit.
// GD keeps the height of its screen (320 points) and changes the width; a box is taken to stay as far from the
// closest of the left edge, the middle and the right edge as it is now, which is what the generated code does
#include "Geometry.hpp"

#include <string>
#include <vector>

namespace aspect
{
  struct Shape
  {
    std::string name;
    float ratio = 16.f / 9.f;
  };

  // 16:9, 16:10, 4:3, 21:9
  std::vector<Shape> const& shapes();

  float widthFor(Shape const& shape, float height);

  // The box on a screen `newWidth` wide instead of `width`, in that screen's points
  geom::Rect move(geom::Rect const& box, float width, float newWidth);

  // Whether the box is inside a screen of this size, to half a point
  bool fits(geom::Rect const& box, float width, float height);

  // The names of the shapes the box doesn't fit on: "4:3, 16:10". Empty if it fits on all
  std::string offScreenOn(geom::Rect const& box, geom::Size screen);
} // namespace aspect
