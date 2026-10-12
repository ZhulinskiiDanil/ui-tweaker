// The generated code, compiled against FakeGeode.hpp and run on fake nodes: each node has to end up where its
// sample says, with its scale, anchor point, z-order and layout options
#include "FakeGeode.hpp"

#include "../Check.hpp"
#include "Samples.hpp"

#include <cstdio>

namespace
{
  std::vector<CCNode*> g_nodes;
  std::vector<CCNode*> g_parents;

  CCNode* make(std::string const& className)
  {
    if (className == "CCMenuItemSpriteExtra")
      return new CCMenuItemSpriteExtra();
    if (className == "CCSprite")
      return new CCSprite();
    return new CCNode();
  }

  geom::Vec2 anchorK(Anchor anchor)
  {
    switch (anchor)
    {
    case Anchor::Center: return {0.5f, 0.5f};
    case Anchor::TopLeft: return {0, 1};
    case Anchor::Top: return {0.5f, 1};
    case Anchor::TopRight: return {1, 1};
    case Anchor::Right: return {1, 0.5f};
    case Anchor::BottomRight: return {1, 0};
    case Anchor::Bottom: return {0.5f, 0};
    case Anchor::BottomLeft: return {0, 0};
    case Anchor::Left: return {0, 0.5f};
    }
    return {};
  }

  bool close(float a, float b)
  {
    return near(a, b, 0.006f);
  }
} // namespace

// Every sample's parent and node in the layer, with the siblings the lookups count
void buildScene(CCNode& layer)
{
  g_nodes.clear();
  g_parents.clear();
  for (auto const& desc : samples())
  {
    auto parent = new CCNode();
    parent->id = desc.parentId;
    parent->size = desc.frame.kind == codegen::FrameKind::Parent ? CCSize{desc.frame.extent.width, desc.frame.extent.height}
                                                                  : CCSize{WIN.width, WIN.height};
    layer.addChild(parent);
    // Siblings before it: of its class for getChildByType (and a button in between, which isn't counted), any for
    // getChildByIndex
    for (int i = 0; i < desc.typeIndex; i++)
    {
      if (!desc.byIndex)
        parent->addChild(new CCMenuItemSpriteExtra());
      parent->addChild(desc.byIndex ? new CCNode() : make(desc.className));
    }
    if (!desc.byIndex)
      while (static_cast<int>(parent->children.size()) > desc.typeIndex * 2)
        parent->children.pop_back();
    auto node = make(desc.className);
    node->id = desc.id;
    if (desc.layout.kind == codegen::LayoutKind::Anchor)
    {
      auto options = AnchorLayoutOptions::create();
      options->anchor = Anchor::TopLeft;
      if (desc.layout.anchorNow && desc.layout.anchorNow->x == 1 && desc.layout.anchorNow->y == 0)
        options->anchor = Anchor::BottomRight;
      node->options = options;
    }
    if (desc.layout.kind == codegen::LayoutKind::Axis)
    {
      auto options = AxisLayoutOptions::create();
      options->breakLine = true;
      node->options = options;
    }
    parent->addChild(node);
    g_nodes.push_back(node);
    g_parents.push_back(parent);
  }
}

CCNode* nodeOf(int index)
{
  return g_nodes[index];
}

void checkSample(int index)
{
  // A copy: samples() makes a new vector, a reference into it would dangle
  auto const all = samples();
  auto const& desc = all[index];
  auto node = g_nodes[index];
  auto layout = desc.layout.kind;
  std::printf("sample %d: (%.2f, %.2f) scale %.2f x %.2f\n", index, node->position.x, node->position.y, node->scaleX,
              node->scaleY);

  if (layout == codegen::LayoutKind::Axis)
  {
    // The position is the layout's: untouched. Its own options kept, the gap and the auto scale set on them
    CHECK(close(node->position.x, 0) && close(node->position.y, 0));
    auto options = dynamic_cast<AxisLayoutOptions*>(node->options);
    CHECK(options && options->breakLine);
    CHECK(options && options->prevGap && close(*options->prevGap, *desc.layout.prevGap));
    CHECK(options && options->autoScale == std::optional<bool>(false));
  }
  else if (layout == codegen::LayoutKind::SimpleAxis)
  {
    CHECK(close(node->position.x, 0) && close(node->position.y, 0));
    CHECK(node->options == nullptr);
  }
  else if (layout == codegen::LayoutKind::Anchor)
  {
    // Where AnchorLayout would put it: the anchor on the parent's box plus the offset
    auto options = dynamic_cast<AnchorLayoutOptions*>(node->options);
    CHECK(options != nullptr);
    if (options)
    {
      auto k = anchorK(options->anchor);
      auto size = node->parent->size;
      CHECK(close(k.x * size.width + options->offset.x, desc.position.x));
      CHECK(close(k.y * size.height + options->offset.y, desc.position.y));
      if (desc.layout.anchorNow)
        CHECK(close(k.x, desc.layout.anchorNow->x) && close(k.y, desc.layout.anchorNow->y));
    }
  }
  else
  {
    CHECK(close(node->position.x, desc.position.x));
    CHECK(close(node->position.y, desc.position.y));
  }

  if (desc.scaleChanged || desc.scaleX != 1 || desc.scaleY != 1)
  {
    CHECK(close(node->scaleX, desc.scaleX));
    CHECK(close(node->scaleY, desc.scaleY));
    if (auto button = dynamic_cast<CCMenuItemSpriteExtra*>(node))
      CHECK(close(button->m_baseScale, desc.scaleX));
  }
  if (desc.anchorChanged)
    CHECK(close(node->anchor.x, desc.anchor.x) && close(node->anchor.y, desc.anchor.y));
  if (desc.zOrderChanged)
    CHECK(node->zOrder == desc.zOrder);
  if (desc.sizeChanged)
  {
    CHECK(close(node->size.width, desc.size.width) && close(node->size.height, desc.size.height));
    CHECK(node->layoutUpdates == (desc.hasLayout ? 1 : 0));
  }
}

#include "samples.inc"

int main()
{
  runSamples();
  return finish("compiled code");
}
