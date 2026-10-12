#pragma once

// The node tree of the running scene at the right of the screen: a search by ID and class, a click on a row
// selects its node. Part of the edit mode's overlay
#include "Retained.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

class TreePanel : public cocos2d::CCNode
{
public:
  static TreePanel* create();

  bool init() override;

  // The rows again, from the scene as it is now. The selected node's row is kept in view when it changes
  void refresh(cocos2d::CCNode* scene, cocos2d::CCNode* selected, cocos2d::CCNode* hovered);

  // Lets go of the nodes while it's hidden
  void forget();

  bool contains(cocos2d::CCPoint world);
  // The node of the row under the point, if any
  cocos2d::CCNode* rowAt(cocos2d::CCPoint world);
  bool searchAt(cocos2d::CCPoint world);
  void scroll(float notches);

  // Typing goes to the search while it's focused
  bool typing() const { return m_typing; }
  void setTyping(bool on);
  void type(char c);
  void erase();
  // The first match: Enter selects it
  cocos2d::CCNode* firstMatch();

private:
  cocos2d::CCLayerColor* m_background = nullptr;
  cocos2d::CCLayerColor* m_searchBackground = nullptr;
  cocos2d::CCLabelBMFont* m_title = nullptr;
  cocos2d::CCLabelBMFont* m_search = nullptr;
  std::vector<cocos2d::CCLabelBMFont*> m_labels;

  // The nodes of the last refresh, held until the next one: a node removed in between is still safe to select
  // (and the edit mode drops a selection that isn't in the scene)
  std::vector<Retained<cocos2d::CCNode>> m_nodes;
  std::vector<size_t> m_shown;
  size_t m_first = 0;
  std::string m_query;
  bool m_typing = false;
  cocos2d::CCNode* m_lastSelected = nullptr;
};
