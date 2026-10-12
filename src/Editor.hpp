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
  // Snapping on or off whatever the setting and Ctrl say; nullopt: as they say
  void setSnapping(std::optional<bool> on);
  // Ends the drag, one step in the history if the node moved
  void release();
  // Scales the selected node: up for positive notches. `fine` is a hundredth a notch
  void wheel(float notches, bool fine);
  // Moves the selected node by this much in its parent's space
  void nudge(float dx, float dy);
  // Ctrl+arrows: the anchor point to the next of 0, 0.5 and 1 that way, the node staying where it is on the screen
  void stepAnchor(int dx, int dy);
  // [ and ]: the z-order down or up by one
  void stepZOrder(int delta);
  // Enter: the selected node's x, y and scale typed in, as numbers or expressions (w, h: the screen; pw, ph: the
  // parent). The keys of it, from the edit mode's listener and the self test alike
  void startTyping();
  void typingKey(int key, bool shift);
  bool typing();
  bool undo();
  bool redo();
  // Tab: the next press or hover takes the next node under the mouse, deeper each time
  void nextUnderMouse();

  void select(cocos2d::CCNode* node);
  // Shift+click: the node added to the selection, or taken out of it. Drags, arrows and the wheel move all of it
  void toggleSelect(cocos2d::CCNode* node);
  // The selected nodes, the one the panel shows first
  std::vector<cocos2d::CCNode*> selection();
  cocos2d::CCNode* selected();
  size_t historySize();

  // The code for one node, against its state before the edits
  std::string codeFor(cocos2d::CCNode* node);
  // The code for every node edited in the running scene
  std::string codeForAll();
  std::vector<cocos2d::CCNode*> editedNodes();
  // The edited nodes here and in the scenes left, which "Copy all" has
  size_t editedCount();
  void copySelected();
  void copyAll();

  // Alt held: the distances from the selected node to the hovered one. nullopt: as Alt says
  void setMeasuring(std::optional<bool> on);
  // The lengths drawn in the last frame
  std::vector<float> measurements();

  // T: the node tree
  void setTreeVisible(bool on);

  // C: the selected node's code in the panel
  void setCodeView(bool on);

  // A: the other screens' shapes over this one
  void setAspectPreview(bool on);

  // For the self test: the mouse is here, not where the real one is, until the real one moves
  void pinMouse(std::optional<cocos2d::CCPoint> world);
  // What the edit mode does to a click before the game sees it: true if it keeps it from the game
  bool swallowsClick();
} // namespace editor
