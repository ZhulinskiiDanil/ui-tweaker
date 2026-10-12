#pragma once

// The node tree of the running scene at the right of the screen: a search by ID and class, a click on a row
// selects its node. Part of the edit mode's overlay
#include "core/Tree.hpp"

class UiLabel;
struct TreeRow : tree::Row
{
  bool hidden = false;
};

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

class TreePanel : public cocos2d::CCNode
{
public:
  static TreePanel* create();

  bool init() override;

  // The rows again, from the scene as it is now. The selected node's row is kept in view when it changes
  // `edited`: the nodes with edits, marked in the tree (compared only)
  void refresh(cocos2d::CCNode* scene, cocos2d::CCNode* selected, cocos2d::CCNode* hovered,
               std::vector<cocos2d::CCNode*> const& edited);

  // Lets go of the nodes while it's hidden
  void forget();

  bool contains(cocos2d::CCPoint world);
  // The node of the row under the point, if it's still in `scene`
  cocos2d::CCNode* rowAt(cocos2d::CCPoint world, cocos2d::CCNode* scene);
  bool searchAt(cocos2d::CCPoint world);
  void scroll(float notches);

  // Typing goes to the search while it's focused
  bool typing() const { return m_typing; }
  void setTyping(bool on);
  void type(char c);
  void erase();
  // The first match still in `scene`: Enter selects it
  cocos2d::CCNode* firstMatch(cocos2d::CCNode* scene);

private:
  cocos2d::CCLayerColor* m_background = nullptr;
  cocos2d::CCLayerColor* m_searchBackground = nullptr;
  UiLabel* m_title = nullptr;
  UiLabel* m_search = nullptr;
  std::vector<UiLabel*> m_labels;

  // The nodes of the last refresh, not held (holding a scene's nodes would make them die late, on the next
  // refresh, with whatever their destructors do to the game). Compared, never followed, until checked again
  std::vector<cocos2d::CCNode*> m_nodes;
  std::vector<struct TreeRow> m_rows;
  int m_framesSinceRefresh = 1000;
  std::string m_refreshedQuery;
  size_t m_refreshedFirst = static_cast<size_t>(-1);
  std::vector<size_t> m_shown;
  size_t m_first = 0;
  std::string m_query;
  bool m_typing = false;
  cocos2d::CCNode* m_lastSelected = nullptr;
};
