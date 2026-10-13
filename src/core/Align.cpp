#include "Align.hpp"

#include <algorithm>
#include <numeric>

geom::Rect align::bounds(std::vector<geom::Rect> const& boxes)
{
  if (boxes.empty())
    return {};
  float minX = boxes[0].minX(), minY = boxes[0].minY(), maxX = boxes[0].maxX(), maxY = boxes[0].maxY();
  for (auto const& box : boxes)
  {
    minX = std::min(minX, box.minX());
    minY = std::min(minY, box.minY());
    maxX = std::max(maxX, box.maxX());
    maxY = std::max(maxY, box.maxY());
  }
  return {minX, minY, maxX - minX, maxY - minY};
}

std::vector<geom::Vec2> align::to(std::vector<geom::Rect> const& boxes, Edge edge, geom::Rect const& target)
{
  std::vector<geom::Vec2> out;
  for (auto const& box : boxes)
  {
    switch (edge)
    {
    case Edge::Left:
      out.push_back({target.minX() - box.minX(), 0});
      break;
    case Edge::CenterX:
      out.push_back({target.midX() - box.midX(), 0});
      break;
    case Edge::Right:
      out.push_back({target.maxX() - box.maxX(), 0});
      break;
    case Edge::Bottom:
      out.push_back({0, target.minY() - box.minY()});
      break;
    case Edge::CenterY:
      out.push_back({0, target.midY() - box.midY()});
      break;
    case Edge::Top:
      out.push_back({0, target.maxY() - box.maxY()});
      break;
    }
  }
  return out;
}

std::vector<geom::Vec2> align::distribute(std::vector<geom::Rect> const& boxes, bool horizontal)
{
  std::vector<geom::Vec2> out(boxes.size());
  if (boxes.size() < 3)
    return out;
  auto start = [&](size_t i) { return horizontal ? boxes[i].minX() : boxes[i].minY(); };
  auto length = [&](size_t i) { return horizontal ? boxes[i].width : boxes[i].height; };
  auto middle = [&](size_t i) { return start(i) + length(i) / 2; };

  // In order along the axis, by their middles (the order they're seen in), the selection's order for a tie
  std::vector<size_t> order(boxes.size());
  std::iota(order.begin(), order.end(), size_t{0});
  std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return middle(a) < middle(b); });

  float from = start(order.front());
  float until = start(order.back()) + length(order.back());
  float lengths = 0;
  for (auto i : order)
    lengths += length(i);
  float gap = (until - from - lengths) / static_cast<float>(boxes.size() - 1);

  float at = from;
  for (auto i : order)
  {
    float move = at - start(i);
    out[i] = horizontal ? geom::Vec2{move, 0} : geom::Vec2{0, move};
    at += length(i) + gap;
  }
  // The last one stays where it is, whatever the float sums came to
  out[order.back()] = {};
  return out;
}
