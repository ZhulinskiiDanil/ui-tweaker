#pragma once

// A grid for the code's numbers: a position whose offset from the closest edge or center (what the code writes,
// `winSize.width / 2 + 40.f`) is a whole number of steps, and the lines of such positions to draw
#include "Expr.hpp"

#include <vector>

namespace grid
{
  // The value as an expression from the closest of 0, extent / 2 and extent, its offset rounded to the step
  expr::Linear round(expr::Linear value, float step);

  // The values in [0, extent] that are a whole number of steps from the closest of 0, extent / 2 and extent:
  // every line of the grid on one axis, the edges and the middle among them
  std::vector<float> lines(float extent, float step);
} // namespace grid
