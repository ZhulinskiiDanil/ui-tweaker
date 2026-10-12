#include "Snap.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace
{
  // Closer than this after the correction counts as on the guide
  constexpr float ON_GUIDE = 0.01f;

  std::optional<float> best(std::vector<float> const& edges, std::vector<snap::Guide> const& guides, bool vertical,
                            float threshold)
  {
    std::optional<float> correction;
    for (auto const& guide : guides)
    {
      if (guide.vertical != vertical)
        continue;
      for (float edge : edges)
      {
        float delta = guide.at - edge;
        if (std::fabs(delta) <= threshold && (!correction || std::fabs(delta) < std::fabs(*correction)))
          correction = delta;
      }
    }
    return correction;
  }
} // namespace

void snap::addBox(std::vector<Guide>& guides, geom::Rect const& box)
{
  for (float x : {box.minX(), box.midX(), box.maxX()})
    guides.push_back({true, x, box.minY(), box.maxY()});
  for (float y : {box.minY(), box.midY(), box.maxY()})
    guides.push_back({false, y, box.minX(), box.maxX()});
}

snap::Result snap::snap(geom::Rect const& box, std::vector<Guide> const& guides, float threshold)
{
  Result result;
  auto dx = best({box.minX(), box.midX(), box.maxX()}, guides, true, threshold);
  auto dy = best({box.minY(), box.midY(), box.maxY()}, guides, false, threshold);
  result.correction = {dx.value_or(0), dy.value_or(0)};

  geom::Rect moved{box.x + result.correction.x, box.y + result.correction.y, box.width, box.height};
  for (auto const& guide : guides)
  {
    bool snapped = guide.vertical ? dx.has_value() : dy.has_value();
    if (!snapped)
      continue;
    auto edges = guide.vertical ? std::vector<float>{moved.minX(), moved.midX(), moved.maxX()}
                                : std::vector<float>{moved.minY(), moved.midY(), moved.maxY()};
    bool on = std::any_of(edges.begin(), edges.end(), [&](float edge) { return std::fabs(edge - guide.at) <= ON_GUIDE; });
    if (!on)
      continue;
    // From the guide's own span to the box, so the line shows what lines up with what
    Guide line = guide;
    line.from = std::min(guide.from, guide.vertical ? moved.minY() : moved.minX());
    line.to = std::max(guide.to, guide.vertical ? moved.maxY() : moved.maxX());
    // One line per place: the longest
    auto same = std::find_if(result.lines.begin(), result.lines.end(), [&](Guide const& other)
                             { return other.vertical == line.vertical && std::fabs(other.at - line.at) <= ON_GUIDE; });
    if (same == result.lines.end())
      result.lines.push_back(line);
    else
    {
      same->from = std::min(same->from, line.from);
      same->to = std::max(same->to, line.to);
    }
  }
  return result;
}
