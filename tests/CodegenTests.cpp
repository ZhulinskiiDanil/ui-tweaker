#include "../src/core/Codegen.hpp"
#include "Check.hpp"

#include <string>

using namespace codegen;

namespace
{
  constexpr geom::Size WIN{569, 320};

  bool has(std::string const& text, std::string const& part)
  {
    return text.find(part) != std::string::npos;
  }

  NodeDesc onScreen(geom::Vec2 position, geom::Vec2 parentOrigin = {})
  {
    NodeDesc node;
    node.id = "play-button";
    node.className = "CCMenuItemSpriteExtra";
    node.parentId = "main-menu";
    node.parentClass = "CCMenu";
    node.position = position;
    node.frame = {FrameKind::Screen, WIN, parentOrigin};
    return node;
  }

  NodeDesc inParent(geom::Vec2 position, geom::Size parent)
  {
    auto node = onScreen(position);
    node.frame = {FrameKind::Parent, parent, {}};
    return node;
  }

  // Reads the generated setPosition back, the way the self test does in the game
  std::optional<geom::Vec2> readBack(std::string const& code, geom::Size parent = {})
  {
    auto args = expr::callArguments(code, "setPosition");
    if (args.size() != 2)
      return std::nullopt;
    auto lookup = [&](std::string_view name) -> std::optional<float>
    {
      if (name == "winSize.width")
        return WIN.width;
      if (name == "winSize.height")
        return WIN.height;
      if (name == "parentSize.width")
        return parent.width;
      if (name == "parentSize.height")
        return parent.height;
      return std::nullopt;
    };
    auto x = expr::evaluate(args[0], lookup);
    auto y = expr::evaluate(args[1], lookup);
    if (!x || !y)
      return std::nullopt;
    return geom::Vec2{*x, *y};
  }

  void screenEdges()
  {
    auto code = generate(onScreen({324.5f, 295}));
    CHECK(has(code, "// play-button (CCMenuItemSpriteExtra) in main-menu (CCMenu)"));
    CHECK(has(code, "if (auto node = this->getChildByIDRecursive(\"play-button\"))"));
    CHECK(has(code, "auto winSize = CCDirector::get()->getWinSize();"));
    CHECK(has(code, "node->setPosition({winSize.width / 2 + 40.f, winSize.height - 25.f});"));
    // The scale isn't written when it's 1 and unchanged
    CHECK(!has(code, "setScale"));

    // Close to the left edge and the bottom: plain numbers, no winSize
    code = generate(onScreen({25, 30}));
    CHECK(has(code, "node->setPosition({25.f, 30.f});"));
    CHECK(!has(code, "winSize"));
  }

  void centeredMenu()
  {
    // A CCMenu as made: its origin at the middle of the screen. The node is 40 right of the middle, 25 above
    // the bottom
    auto node = onScreen({40, -135}, {284.5f, 160});
    auto code = generate(node);
    CHECK(has(code, "node->setPosition({40.f, -winSize.height / 2 + 25.f});"));
    auto back = readBack(code);
    CHECK(back && near(back->x, 40) && near(back->y, -135));
  }

  void parentEdges()
  {
    auto code = generate(inParent({190, 50}, {200, 100}));
    CHECK(has(code, "auto parentSize = node->getParent()->getContentSize();"));
    CHECK(has(code, "node->setPosition({parentSize.width - 10.f, parentSize.height / 2});"));
    CHECK(!has(code, "winSize"));
    auto back = readBack(code, {200, 100});
    CHECK(back && near(back->x, 190) && near(back->y, 50));
  }

  void anchorLayout()
  {
    auto node = inParent({190, 90}, {200, 100});
    node.layout = {LayoutKind::Anchor, "AnchorLayout", true, std::nullopt};
    auto code = generate(node);
    CHECK(has(code, "setAnchor(Anchor::TopRight)"));
    CHECK(has(code, "setOffset({-10.f, -10.f})"));
    CHECK(!has(code, "setPosition"));
    CHECK(has(layoutWarning(node), "AnchorLayout"));

    CHECK(anchorName(0.5f, 0.5f) == "Center");
    CHECK(anchorName(0, 0.5f) == "Left");
    CHECK(anchorName(0.5f, 0) == "Bottom");
    CHECK(anchorName(0, 1) == "TopLeft");
    CHECK(anchorName(1, 0) == "BottomRight");

    // An anchor the node has stays, even where another one is closer: only the offset changes
    node.layout.anchorNow = geom::Vec2{1, 0};
    code = generate(node);
    CHECK(has(code, "setAnchor(Anchor::BottomRight)"));
    CHECK(has(code, "setOffset({-10.f, 90.f})"));
    node.layout.anchorNow.reset();

    // Without options the layout leaves the node alone
    node.layout.hasAnchorOptions = false;
    CHECK(layoutWarning(node).empty());
    CHECK(has(generate(node), "setPosition"));
  }

  void axisLayout()
  {
    auto node = onScreen({324.5f, 295});
    node.layout = {LayoutKind::Axis, "RowLayout", false, std::nullopt, 12.f};
    node.scaleX = node.scaleY = 1.2f;
    node.scaleChanged = true;
    auto code = generate(node);
    CHECK(has(layoutWarning(node), "main-menu has a RowLayout"));
    CHECK(has(code, "// main-menu has a RowLayout"));
    // The warning wrapped as code is
    for (size_t start = 0, end; start < code.size(); start = end + 1)
    {
      end = code.find(char(10), start);
      if (end == std::string::npos)
        end = code.size();
      auto line = code.substr(start, end - start);
      if (line.rfind("// main", 0) == 0 || line.rfind("// ", 0) == 0)
        CHECK(line.size() <= 104);
    }
    // The node's own options kept, the gap and the auto scale set on them
    CHECK(has(code, "auto options = typeinfo_cast<AxisLayoutOptions*>(node->getLayoutOptions());"));
    CHECK(has(code, "node->setLayoutOptions((options ? options : AxisLayoutOptions::create())->setPrevGap(12.f)"
                    "->setAutoScale(false));"));
    CHECK(has(code, "node->setScale(1.2f);"));
    // The scale before the options: setting them updates the layout
    CHECK(code.find("node->setScale(1.2f);") < code.find("setLayoutOptions"));
    // A button's press animation goes back to its base scale: that too
    CHECK(has(code, "static_cast<CCMenuItemSpriteExtra*>(node)->m_baseScale = 1.2f;"));
    // The position only as a comment
    CHECK(has(code, "// Without the layout: node->setPosition({"));
    CHECK(!has(code, "\n  node->setPosition"));

    // A SimpleAxisLayout has no AxisLayoutOptions: the gap is an AxisGap, as a comment
    node.layout = {LayoutKind::SimpleAxis, "SimpleRowLayout", false, std::nullopt, 8.f};
    code = generate(node);
    CHECK(!has(code, "AxisLayoutOptions"));
    CHECK(has(code, "insertBefore(AxisGap::create(8.f), node)"));
    CHECK(has(code, "\n  node->setScale(1.2f);"));
    CHECK(has(layoutWarning(node), "SimpleRowLayout"));

    // In a layout the z-order is the order: still written
    node.zOrder = 3;
    node.zOrderChanged = true;
    CHECK(has(generate(node), "node->setZOrder(3);"));
    node.zOrderChanged = false;

    // A scale the layout gave it, not changed: not written
    auto laidOut = onScreen({324.5f, 295});
    laidOut.layout = {LayoutKind::SimpleAxis, "SimpleRowLayout", false, std::nullopt, std::nullopt};
    laidOut.scaleX = laidOut.scaleY = 0.23f;
    CHECK(!has(generate(laidOut), "setScale"));

    node.layout = {LayoutKind::Other, "GridLayout", false, std::nullopt};
    CHECK(has(layoutWarning(node), "GridLayout"));
    CHECK(has(generate(node), "node->setPosition"));
  }

  void lookups()
  {
    auto node = onScreen({10, 10});
    node.id = "";
    node.className = "CCSprite";
    node.typeIndex = 2;
    auto code = generate(node);
    CHECK(has(code, "// CCSprite (no ID) in main-menu (CCMenu)"));
    CHECK(has(code, "auto parent = this->getChildByIDRecursive(\"main-menu\"); auto node = parent ? "
                    "parent->getChildByType<CCSprite>(2) : nullptr"));

    // A class the code can't name: by its index among all children
    node.className = "`anonymous namespace'::Thing";
    node.byIndex = true;
    node.typeIndex = 5;
    CHECK(has(generate(node), "parent->getChildByIndex(5) : nullptr"));
    CHECK(!has(generate(node), "anonymous namespace'::Thing>"));
    node.className = "CCSprite";
    node.byIndex = false;

    // IDs with quotes or backslashes stay one string literal
    auto odd = node;
    odd.id = "say \"hi\"";
    CHECK(has(generate(odd), "getChildByIDRecursive(\"say \\\"hi\\\"\")"));

    node.parentId = "";
    code = generate(node);
    CHECK(lookup(node).empty());
    CHECK(has(code, "No ID to find it by"));
    CHECK(has(code, "  node->setPosition({10.f, 10.f});"));

    // Two without IDs in one block of code: each in braces, so parentSize isn't declared twice
    auto first = inParent({190, 50}, {200, 100});
    first.id = first.parentId = "";
    auto all = generateAll({first, first});
    CHECK(has(all, "No ID to find it by: these go where you make it, with `node` being it\n{\n  auto parentSize"));
    // Never at the top level of the block
    CHECK(!has(all, "\nauto parentSize"));
  }

  void properties()
  {
    auto node = onScreen({25, 30});
    node.scaleX = 0.5f;
    node.scaleY = 0.75f;
    node.anchor = {0, 1};
    node.anchorChanged = true;
    node.zOrder = 5;
    node.zOrderChanged = true;
    auto code = generate(node);
    CHECK(has(code, "node->setScaleX(0.5f);"));
    CHECK(has(code, "node->setScaleY(0.75f);"));
    CHECK(has(code, "node->setAnchorPoint({0.f, 1.f});"));
    CHECK(has(code, "node->setZOrder(5);"));
  }

  void everything()
  {
    auto first = onScreen({324.5f, 295});
    auto second = onScreen({25, 30});
    second.id = "geode-button";
    auto code = generateAll({first, second});
    // One winSize for the whole block, at the top
    CHECK(code.find("auto winSize") == 0);
    CHECK(code.find("auto winSize", 1) == std::string::npos);
    CHECK(has(code, "// play-button (CCMenuItemSpriteExtra)"));
    CHECK(has(code, "// geode-button (CCMenuItemSpriteExtra)"));
    CHECK(code.find("play-button") < code.find("geode-button"));

    // Nothing that needs winSize: no winSize
    CHECK(!has(generateAll({second}), "winSize"));
  }

  void roundTrips()
  {
    // Wherever the node and its parent are, the code puts it back exactly there
    int checked = 0;
    for (float x : {-30.f, 0.f, 12.25f, 150.f, 284.5f, 300.f, 450.f, 569.f, 600.f})
      for (float y : {-10.f, 0.f, 25.f, 160.f, 200.f, 295.f, 320.f})
        for (geom::Vec2 origin : {geom::Vec2{0, 0}, geom::Vec2{284.5f, 160}, geom::Vec2{100, -20}, geom::Vec2{569, 320}})
        {
          auto node = onScreen({x - origin.x, y - origin.y}, origin);
          auto back = readBack(generate(node));
          CHECK(back && near(back->x, node.position.x) && near(back->y, node.position.y));

          auto inside = inParent({x, y}, {origin.x, origin.y});
          back = readBack(generate(inside), {origin.x, origin.y});
          CHECK(back && near(back->x, x) && near(back->y, y));
          checked++;
        }
    CHECK(checked == 9 * 7 * 4);
  }
} // namespace

int main()
{
  screenEdges();
  centeredMenu();
  parentEdges();
  anchorLayout();
  axisLayout();
  lookups();
  properties();
  everything();
  roundTrips();
  return finish("codegen");
}
