#include "Explain.hpp"

#include "Text.hpp"

std::vector<std::string> explain::reasons(Facts const& facts, words::Lang lang)
{
  auto const& w = words::get(lang);
  std::vector<std::string> out;
  if (!facts.layoutName.empty() && facts.layoutMovesIt)
    out.push_back(text::format(w.layoutMoves, {facts.parent, facts.layoutName}));
  if (facts.runningActions == 1)
    out.push_back(w.action);
  else if (facts.runningActions > 1)
    out.push_back(text::format(w.actions, {std::to_string(facts.runningActions)}));
  if (!facts.hiddenBy.empty())
    out.push_back(text::format(w.hidden, {facts.hiddenBy}));
  if (facts.transparent)
    out.push_back(w.transparent);
  if (facts.zeroScale)
    out.push_back(w.zeroScale);
  if (facts.zeroSize)
    out.push_back(w.zeroSize);
  if (facts.ignoresAnchor)
    out.push_back(w.ignoresAnchor);
  if (!facts.clippedBy.empty())
    out.push_back(text::format(w.clipped, {facts.clippedBy}));
  if (facts.parentTurned)
    out.push_back(w.parentTurned);
  if (facts.outsideScreen)
    out.push_back(w.outsideScreen);
  return out;
}
