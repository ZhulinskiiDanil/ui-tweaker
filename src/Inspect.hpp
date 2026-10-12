#pragma once

// Reading real nodes: what they are, where they are on the screen, which one is under the mouse, and the
// plain description the code generator takes
#include "core/Codegen.hpp"
#include "core/Explain.hpp"
#include "core/Geometry.hpp"

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

namespace inspect
{
  // What an edit changes, and what undo puts back
  struct NodeState
  {
    cocos2d::CCPoint position;
    float scaleX = 1;
    float scaleY = 1;
    cocos2d::CCPoint anchor;
    int zOrder = 0;
    cocos2d::CCSize size;

    bool operator==(NodeState const& other) const;
  };

  NodeState stateOf(cocos2d::CCNode* node);
  void apply(cocos2d::CCNode* node, NodeState const& state);

  // The class from RTTI without "class " and "cocos2d::": "CCMenuItemSpriteExtra"
  std::string className(cocos2d::CCObject const* object);
  // "play-button (CCMenuItemSpriteExtra)", or the class alone without an ID
  std::string name(cocos2d::CCNode* node);

  geom::Affine toGeom(cocos2d::CCAffineTransform const& transform);
  // What the node keeps for its transform, for the math in core
  geom::NodeTransform transformOf(cocos2d::CCNode* node);
  // The node's content box on the screen
  geom::Quad worldQuad(cocos2d::CCNode* node);

  // Whether `node` is one of the nodes of `root`'s tree, found by walking the tree: `node` itself isn't read, so
  // it may be a pointer to a node that's gone
  bool contains(cocos2d::CCNode* root, cocos2d::CCNode const* node);

  // Whether the node is in `root`'s tree (false once it was removed, or its scene was replaced)
  bool isIn(cocos2d::CCNode* node, cocos2d::CCNode* root);

  // The nodes under a point, topmost first: what's drawn last over it, without plain containers unless there's
  // nothing else. Unless `deep`, a hit inside a button, a label or a 9-slice is that button, label or 9-slice
  std::vector<cocos2d::CCNode*> hitsAt(cocos2d::CCNode* root, cocos2d::CCPoint world, bool deep = false);

  // The visible nodes with an ID that draw something, in the order they're drawn: what the aspect preview checks
  std::vector<cocos2d::CCNode*> shownWithIds(cocos2d::CCNode* root);

  // The layout of the node's parent, as far as the code is concerned
  codegen::LayoutInfo layoutOf(cocos2d::CCNode* node);

  // What could make the node not move, not show or act oddly
  explain::Facts factsOf(cocos2d::CCNode* node);

  // The node for the code generator. `original` is the state before the edits, to write only what changed
  codegen::NodeDesc describe(cocos2d::CCNode* node, NodeState const* original = nullptr);

  // Whether the generated code, read back, puts the node exactly where it is. Empty if it does, what's wrong
  // otherwise. Checks setPosition (also a commented one), or AnchorLayoutOptions' anchor and offset
  std::string checkCode(cocos2d::CCNode* node, std::string const& code);
} // namespace inspect
