#include "Explain.hpp"

std::vector<std::string> explain::reasons(Facts const& facts)
{
  std::vector<std::string> out;
  if (!facts.layoutName.empty() && facts.layoutMovesIt)
    out.push_back(facts.parent + " has a " + facts.layoutName +
                  ": its updateLayout() sets this node's position, so a setPosition of yours lasts until it runs");
  if (facts.runningActions > 0)
    out.push_back(std::to_string(facts.runningActions) +
                  (facts.runningActions == 1 ? " action is" : " actions are") +
                  " running on it: an animation may set its position or scale every frame");
  if (!facts.hiddenBy.empty())
    out.push_back("Not drawn: " + facts.hiddenBy + " is invisible (setVisible(false))");
  if (facts.transparent)
    out.push_back("Not seen: its opacity is 0");
  if (facts.zeroScale)
    out.push_back("Not seen: its scale is 0");
  if (facts.zeroSize)
    out.push_back("Its content size is 0 x 0: it has no box of its own, its anchor point does nothing and a "
                  "layout gives it no room. setContentSize, or look at its children");
  if (facts.ignoresAnchor)
    out.push_back("It ignores its anchor point for its position (a layer or a menu as made): the position is "
                  "where its bottom left corner is");
  if (!facts.clippedBy.empty())
    out.push_back("Outside " + facts.clippedBy + ", which cuts off what's outside its box");
  if (facts.parentTurned)
    out.push_back("Its parent is turned, flipped or skewed: positions are along the parent's axes, not the "
                  "screen's");
  if (facts.outsideScreen)
    out.push_back("Off the screen");
  return out;
}
