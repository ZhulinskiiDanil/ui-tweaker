#pragma once

// The edit mode: the mouse and the keys move and scale the nodes of the running scene, with undo, and the
// result is copied as C++. While it's off nothing of it exists: no listeners, no overlay, no node touched
#include "core/Align.hpp"

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
  // For the self test: the grid's step whatever the setting says (0: none)
  void setGrid(std::optional<int> step);
  // Where a drag would put the node's center from this point: snapped to the screen, its parent and its siblings
  cocos2d::CCPoint snappedCenter(cocos2d::CCNode* node, cocos2d::CCPoint center);
  // The resize handle of the selected node at a point on the screen: -1, 0 or 1 on each axis
  std::optional<cocos2d::CCPoint> handleAt(cocos2d::CCPoint world);
  // A handle pressed at the point: its side follows the mouse, the opposite side stays
  void startResize(cocos2d::CCPoint handle, cocos2d::CCPoint world);
  void resizeTo(cocos2d::CCPoint world);
  // Ends the drag, one step in the history if the node moved
  void release();
  // Scales the selected node: up for positive notches. `fine` is a hundredth a notch
  void wheel(float notches, bool fine);
  // Moves the selected node by this much in its parent's space
  void nudge(float dx, float dy);
  // Ctrl+wheel: the selected nodes turned by 15 degrees a notch, or 1 if fine
  void turn(float notches, bool fine);
  // Ctrl+arrows: the anchor point to the next of 0, 0.5 and 1 that way, the node staying where it is on the screen
  void stepAnchor(int dx, int dy);
  // V: the selected nodes hidden, or shown again (the tree has hidden nodes too)
  void toggleVisible();
  // Ctrl+D: a node like the selected one (its frame, its button's image, its label's text) made next to it, in the
  // same parent, selected; its code makes it
  void duplicate();
  // The selected nodes' edges or centers lined up on the screen: several to the box of them all, one to its
  // parent's (Alt+A, D, H, W, S, V as in Figma)
  void alignTo(align::Edge edge);
  // The same gap between the selected nodes along one axis, the outermost staying (Alt+Shift+H, V; three or more)
  void distribute(bool horizontal);
  // R: the selected nodes as they were before their first edit, as a step to undo
  void reset();
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
  // Ctrl+A: the selected node and every sibling of it that's shown and has a size (a menu's row of buttons), for
  // aligning them or L
  void selectSiblings();
  // A right drag from `from` to `to`: the nodes wholly inside the box it made (inspect::within), added to the
  // selection if `add` (Shift), else in place of it. A box this small is a right click: nothing selected
  void selectIn(cocos2d::CCPoint from, cocos2d::CCPoint to, bool add);
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
  // Alt+C: only the code that finds the selected node, for code of your own that needs it
  void copyFinding();
  // The edits of every scene, kept in the save folder between runs: read when the mod loads, written as they change
  // and when the game exits. Ctrl+Delete forgets those of other scenes
  void loadSession();
  void saveSessionNow();
  void forgetKept();
  // For the self test: where the session is written, and putting edits back whatever the setting says
  std::string sessionFile();
  void setPutBack(std::optional<bool> on);
  // L: the selected nodes of one parent as a row or a column of a new menu with an AxisLayout. Empty if they
  // aren't two or more of one parent
  std::string layoutCode();
  void copyLayout();
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
  // Whether the panel is at a point of the screen: nothing is hovered under it
  bool overPanel(cocos2d::CCPoint world);
  // What the edit mode does to a click before the game sees it: true if it keeps it from the game
  bool swallowsClick();
} // namespace editor
