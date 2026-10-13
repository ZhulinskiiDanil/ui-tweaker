#pragma once

// Several nodes in a row or a column, as a Geode AxisLayout would put them: which axis, in what order, the gap
// between them and how they line up across it. In the parent's space, boxes as the nodes' bounding boxes
#include "Geometry.hpp"

#include <string>
#include <vector>

namespace rowfit
{
  // As Geode's AxisAlignment: on a row's cross axis Start is the bottom, on a column's the left
  enum class Align
  {
    Start,
    Center,
    End,
  };

  struct Fit
  {
    bool row = true;
    // The boxes' indices along the axis: left to right, or top to bottom
    std::vector<size_t> order;
    // The mean gap between neighbours, and how far the farthest one is from it
    float gap = 0;
    float gapSpread = 0;
    // The line they're closest to sharing across the axis, and how far the farthest is from it
    Align cross = Align::Center;
    float crossSpread = 0;
    // The box of them all
    geom::Rect bounds;
  };

  // Nothing for fewer than two
  Fit fit(std::vector<geom::Rect> const& boxes);

  // Close enough for a layout to put them where they are: gaps and the cross line within `tolerance`
  bool exact(Fit const& fit, float tolerance = 0.5f);

  // "Start", "Center", "End"
  std::string alignName(Align align);
} // namespace rowfit
