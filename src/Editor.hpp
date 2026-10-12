#pragma once

// The edit mode: the mouse and the keys move and scale the nodes of the running scene, with undo, and the
// result is copied as C++. While it's off nothing of it exists: no listeners, no overlay, no node touched
#include <Geode/Geode.hpp>

#include <optional>
#include <string>
#include <vector>

namespace editor
{
  bool active();
  void setActive(bool on);
  void toggle();

  // ! --- What the mouse and the keys do, for the self test too --- !

  // Selects what's under the point and gets ready to drag it. Returns the selected node
  cocos2d::CCNode* pressAt(cocos2d::CCPoint world, bool deep = false);
  // Selects this node and gets ready to drag it from the point, as if it had been pressed there
  void grab(cocos2d::CCNode* node, cocos2d::CCPoint world);
  // Drags the pressed node to follow the point
  void dragTo(cocos2d::CCPoint world);
  // Ends the drag, one step in the history if the node moved
  void release();
  // Scales the selected node: up for positive notches. `fine` is a hundredth a notch
  void wheel(float notches, bool fine);
  // Moves the selected node by this much in its parent's space
  void nudge(float dx, float dy);
  bool undo();
  bool redo();
  // Tab: the next press or hover takes the next node under the mouse, deeper each time
  void nextUnderMouse();

  void select(cocos2d::CCNode* node);
  cocos2d::CCNode* selected();
  size_t historySize();

  // The code for one node, against its state before the edits
  std::string codeFor(cocos2d::CCNode* node);
  // The code for every node edited in the running scene
  std::string codeForAll();
  std::vector<cocos2d::CCNode*> editedNodes();
  void copySelected();
  void copyAll();

  // For the self test: the mouse is here, not where the real one is, until the real one moves
  void pinMouse(std::optional<cocos2d::CCPoint> world);
  // What the edit mode does to a click before the game sees it: true if it keeps it from the game
  bool swallowsClick();
} // namespace editor
