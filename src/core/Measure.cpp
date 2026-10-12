#include "Measure.hpp"

#include <algorithm>
#include <cmath>

namespace
{
  bool inside(geom::Rect const& inner, geom::Rect const& outer)
  {
    return inner.minX() >= outer.minX() && inner.maxX() <= outer.maxX() && inner.minY() >= outer.minY() &&
           inner.maxY() <= outer.maxY();
  }

  void add(std::vector<measure::Segment>& out, geom::Vec2 from, geom::Vec2 to)
  {
    float length = std::hypot(to.x - from.x, to.y - from.y);
    if (length > 0.01f)
      out.push_back({from, to, length});
  }

  // The middle of where two spans overlap, or of the first span if they don't
  float across(float aMin, float aMax, float bMin, float bMax)
  {
    float low = std::max(aMin, bMin);
    float high = std::min(aMax, bMax);
    return low <= high ? (low + high) / 2 : (aMin + aMax) / 2;
  }
} // namespace

std::vector<measure::Segment> measure::between(geom::Rect const& a, geom::Rect const& b)
{
  std::vector<Segment> out;
  if (a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height)
    return out;

  // One inside the other: the inner box's four edges to the outer box's
  if (inside(a, b) || inside(b, a))
  {
    auto const& inner = inside(a, b) ? a : b;
    auto const& outer = inside(a, b) ? b : a;
    float midY = inner.midY();
    float midX = inner.midX();
    add(out, {outer.minX(), midY}, {inner.minX(), midY});
    add(out, {inner.maxX(), midY}, {outer.maxX(), midY});
    add(out, {midX, outer.minY()}, {midX, inner.minY()});
    add(out, {midX, inner.maxY()}, {midX, outer.maxY()});
    return out;
  }

  // Apart: the gap on each axis they're apart on, drawn where they face each other
  float y = across(a.minY(), a.maxY(), b.minY(), b.maxY());
  if (b.minX() >= a.maxX())
    add(out, {a.maxX(), y}, {b.minX(), y});
  else if (a.minX() >= b.maxX())
    add(out, {b.maxX(), y}, {a.minX(), y});

  float x = across(a.minX(), a.maxX(), b.minX(), b.maxX());
  if (b.minY() >= a.maxY())
    add(out, {x, a.maxY()}, {x, b.minY()});
  else if (a.minY() >= b.maxY())
    add(out, {x, b.maxY()}, {x, a.minY()});
  return out;
}
