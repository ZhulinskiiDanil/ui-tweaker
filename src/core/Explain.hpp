#pragma once

// Why a node doesn't move, isn't seen or behaves oddly: plain facts about it in, sentences out
#include "Words.hpp"

#include <string>
#include <vector>

namespace explain
{
  struct Facts
  {
    // "main-menu (CCMenu)"
    std::string parent;
    // The parent's layout and what it does: "" if none
    std::string layoutName;
    bool layoutMovesIt = false;
    // The first ancestor (or the node) that's invisible, "" if none
    std::string hiddenBy;
    bool transparent = false;
    bool zeroSize = false;
    bool zeroScale = false;
    bool ignoresAnchor = false;
    // Actions running on the node: something animates it
    unsigned runningActions = 0;
    // The node is outside its parent's box, which clips it (a scroll layer, a clipping node)
    std::string clippedBy;
    // A turned or flipped parent: the arrows move it along the parent's axes, not the screen's
    bool parentTurned = false;
    bool outsideScreen = false;
  };

  std::vector<std::string> reasons(Facts const& facts, words::Lang lang = words::Lang::English);
} // namespace explain
