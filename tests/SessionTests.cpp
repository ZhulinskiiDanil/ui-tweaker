#include "../src/core/Session.hpp"
#include "Check.hpp"

using namespace codegen;

namespace
{
  NodeDesc full()
  {
    NodeDesc node;
    node.id = "say \"hi\" \\o/\nnext";
    node.className = "CCMenuItemSpriteExtra";
    node.parentId = "bottom-menu";
    node.parentClass = "CCMenu";
    node.layer = "MenuLayer";
    node.typeIndex = 3;
    node.byIndex = true;
    node.idUnique = false;
    node.pathRoot = "root";
    node.pathFromLayer = true;
    node.path = {{"inner", "", 0, false, ""}, {"", "CCSprite", 2, true, "GJ_playBtn_001.png"}};
    node.position = {-12.345678f, 1e-3f};
    node.scaleX = 1.25f;
    node.scaleY = 0.5f;
    node.anchor = {0, 1};
    node.zOrder = -7;
    node.size = {109.5f, 40};
    node.hasLayout = true;
    node.frame = {FrameKind::Parent, {200, 100}, {3, 4}};
    node.layout = {LayoutKind::Anchor, "AnchorLayout", true, geom::Vec2{1, 0}, 7.5f};
    node.scaleChanged = true;
    node.anchorChanged = true;
    node.zOrderChanged = true;
    node.sizeChanged = true;
    node.visible = false;
    node.visibleChanged = true;
    return node;
  }

  bool same(NodeDesc const& a, NodeDesc const& b)
  {
    return a.id == b.id && a.className == b.className && a.parentId == b.parentId && a.parentClass == b.parentClass &&
           a.layer == b.layer && a.typeIndex == b.typeIndex && a.byIndex == b.byIndex && a.idUnique == b.idUnique &&
           a.pathRoot == b.pathRoot && a.pathFromLayer == b.pathFromLayer && a.path == b.path &&
           a.position == b.position && a.scaleX == b.scaleX && a.scaleY == b.scaleY && a.anchor == b.anchor &&
           a.zOrder == b.zOrder && a.size == b.size && a.hasLayout == b.hasLayout && a.frame.kind == b.frame.kind &&
           a.frame.extent == b.frame.extent && a.frame.origin == b.frame.origin && a.layout.kind == b.layout.kind &&
           a.layout.name == b.layout.name && a.layout.hasAnchorOptions == b.layout.hasAnchorOptions &&
           a.layout.anchorNow == b.layout.anchorNow && a.layout.prevGap == b.layout.prevGap &&
           a.scaleChanged == b.scaleChanged && a.anchorChanged == b.anchorChanged && a.zOrderChanged == b.zOrderChanged &&
           a.sizeChanged == b.sizeChanged && a.visible == b.visible && a.visibleChanged == b.visibleChanged;
  }
} // namespace

int main()
{
  // Every field there and back, the strings with quotes, backslashes and a line break in them
  NodeDesc plain;
  plain.id = "play-button";
  plain.layer = "MenuLayer";
  session::Groups groups{{"MenuLayer", {full(), plain}}, {"Creator\nLayer", {plain}}};
  auto text = session::write(groups);
  auto back = session::read(text);
  CHECK(back.has_value());
  if (back)
  {
    CHECK(back->size() == 2);
    CHECK((*back)[0].first == "MenuLayer" && (*back)[1].first == "Creator\nLayer");
    CHECK((*back)[0].second.size() == 2 && (*back)[1].second.size() == 1);
    if ((*back)[0].second.size() == 2)
    {
      CHECK(same((*back)[0].second[0], full()));
      CHECK(same((*back)[0].second[1], plain));
    }
  }
  // One line a field: no raw line break inside a value
  CHECK(text.find("say \"hi\" \\\\o/\\nnext") != std::string::npos);

  // Not a session, an empty one, unknown keys and Windows line ends
  CHECK(!session::read("something else").has_value());
  CHECK(!session::read("").has_value());
  auto empty = session::read("ui-tweaker session 1\n");
  CHECK(empty && empty->empty());
  auto odd = session::read("ui-tweaker session 1\r\nlayer L\r\nnode\r\nid x\r\nnewer-field 1 2 3\r\nzOrder 4\r\nend\r\n");
  CHECK(odd && odd->size() == 1 && (*odd)[0].second.size() == 1 && (*odd)[0].second[0].id == "x" &&
        (*odd)[0].second[0].zOrder == 4);
  // A node cut off before its end is left out
  auto cut = session::read("ui-tweaker session 1\nlayer L\nnode\nid x\n");
  CHECK(cut && cut->empty());

  // The same node: by an ID the layer finds, else by its way down
  NodeDesc a = plain, b = plain;
  CHECK(session::sameNode(a, b));
  b.position = {5, 5};
  CHECK(session::sameNode(a, b));
  b.id = "other";
  CHECK(!session::sameNode(a, b));
  b = a;
  b.layer = "CreatorLayer";
  CHECK(!session::sameNode(a, b));
  NodeDesc p = full(), q = full();
  q.id = "";
  p.id = "";
  CHECK(session::sameNode(p, q));
  q.path[1].typeIndex = 1;
  CHECK(!session::sameNode(p, q));
  // Through a parent's ID and an index
  NodeDesc r, t;
  r.layer = t.layer = "L";
  r.parentId = t.parentId = "menu";
  r.className = t.className = "CCSprite";
  r.typeIndex = t.typeIndex = 1;
  CHECK(session::sameNode(r, t));
  t.typeIndex = 2;
  CHECK(!session::sameNode(r, t));
  return finish("session");
}
