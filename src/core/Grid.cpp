#include "Grid.hpp"

#include <algorithm>
#include <cmath>

expr::Linear grid::round(expr::Linear value, float step)
{
  if (step <= 0)
    return value;
  return {value.k, std::round(value.offset / step) * step};
}

std::vector<float> grid::lines(float extent, float step)
{
  std::vector<float> out;
  if (step <= 0 || extent <= 0)
    return out;
  // From each anchor both ways, as far as values stay closest to it: the quarter points are where that changes
  for (float k : {0.f, 0.5f, 1.f})
    for (float direction : {1.f, -1.f})
      for (float value = k * extent; value >= -1e-3f && value <= extent + 1e-3f; value += direction * step)
      {
        if (std::fabs(expr::nearest(value, extent).k - k) > 1e-4f)
          break;
        out.push_back(value);
      }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end(), [](float a, float b) { return std::fabs(a - b) < 1e-3f; }), out.end());
  return out;
}
