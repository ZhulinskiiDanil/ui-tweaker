#include "RowFit.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

rowfit::Fit rowfit::fit(std::vector<geom::Rect> const& boxes)
{
  Fit out;
  if (boxes.size() < 2)
    return out;

  // The axis they're spread along the most, by their middles
  auto spread = [&](auto middle)
  {
    float low = middle(boxes[0]), high = low;
    for (auto const& box : boxes)
    {
      low = std::min(low, middle(box));
      high = std::max(high, middle(box));
    }
    return high - low;
  };
  out.row = spread([](geom::Rect const& r) { return r.midX(); }) >= spread([](geom::Rect const& r) { return r.midY(); });

  // Left to right, or top to bottom
  out.order.resize(boxes.size());
  std::iota(out.order.begin(), out.order.end(), size_t{0});
  std::stable_sort(out.order.begin(), out.order.end(),
                   [&](size_t a, size_t b)
                   { return out.row ? boxes[a].midX() < boxes[b].midX() : boxes[a].midY() > boxes[b].midY(); });

  std::vector<float> gaps;
  for (size_t i = 1; i < out.order.size(); i++)
  {
    auto const& before = boxes[out.order[i - 1]];
    auto const& after = boxes[out.order[i]];
    gaps.push_back(out.row ? after.minX() - before.maxX() : before.minY() - after.maxY());
  }
  out.gap = std::accumulate(gaps.begin(), gaps.end(), 0.f) / static_cast<float>(gaps.size());
  for (auto gap : gaps)
    out.gapSpread = std::max(out.gapSpread, std::fabs(gap - out.gap));

  // Across: the start edges, the middles or the end edges, whichever are the closest to one line
  auto across = [&](Align align)
  {
    float low = 0, high = 0;
    bool first = true;
    for (auto const& box : boxes)
    {
      float start = out.row ? box.minY() : box.minX();
      float length = out.row ? box.height : box.width;
      float value = align == Align::Start ? start : align == Align::Center ? start + length / 2 : start + length;
      low = first ? value : std::min(low, value);
      high = first ? value : std::max(high, value);
      first = false;
    }
    return high - low;
  };
  out.cross = Align::Center;
  out.crossSpread = across(Align::Center);
  for (auto align : {Align::Start, Align::End})
    if (across(align) < out.crossSpread - 1e-4f)
    {
      out.cross = align;
      out.crossSpread = across(align);
    }

  float minX = boxes[0].minX(), minY = boxes[0].minY(), maxX = boxes[0].maxX(), maxY = boxes[0].maxY();
  for (auto const& box : boxes)
  {
    minX = std::min(minX, box.minX());
    minY = std::min(minY, box.minY());
    maxX = std::max(maxX, box.maxX());
    maxY = std::max(maxY, box.maxY());
  }
  out.bounds = {minX, minY, maxX - minX, maxY - minY};
  return out;
}

bool rowfit::exact(Fit const& fit, float tolerance)
{
  return fit.order.size() >= 2 && fit.gapSpread <= tolerance && fit.crossSpread <= tolerance;
}

std::string rowfit::alignName(Align align)
{
  switch (align)
  {
  case Align::Start:
    return "Start";
  case Align::Center:
    return "Center";
  case Align::End:
    return "End";
  }
  return "Center";
}
