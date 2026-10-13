#include "SelfTest.hpp"

#include "Editor.hpp"
#include "Inspect.hpp"
#include "Retained.hpp"
#include "core/Expr.hpp"
#include "core/Measure.hpp"
#include "core/Session.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CreatorLayer.hpp>
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJGarageLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/LevelSearchLayer.hpp>
#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/MoreOptionsLayer.hpp>
#include <Geode/binding/OptionsLayer.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <Geode/binding/LevelSelectLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include <cmath>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <set>

using namespace geode::prelude;

namespace
{
  constexpr char NEWLINE = 10;

  // Frames in the main menu before the test, for it to settle, and as long as they'd last at 60 fps
  constexpr int SETTLE_FRAMES = 90;
  constexpr float DRAG_X = 30.f;
  constexpr float DRAG_Y = -20.f;

  int g_checks = 0;
  int g_failed = 0;
  int g_frames = 0;
  bool g_lockable = false;

  void check(bool ok, std::string const& what)
  {
    g_checks++;
    if (ok)
      return;
    g_failed++;
    log::error("Self test FAIL: {}", what);
  }

  bool close(float a, float b, float epsilon = 0.01f)
  {
    return std::fabs(a - b) <= epsilon;
  }

  CCPoint worldPositionOf(CCNode* node)
  {
    return node->getParent()->convertToWorldSpace(node->getPosition());
  }

  bool visibleInTree(CCNode* node)
  {
    for (; node; node = node->getParent())
      if (!node->isVisible())
        return false;
    return true;
  }

  // The scene in the log, for when the test can't find what it looks for
  void dumpTree(CCNode* node, int depth)
  {
    if (depth > 6)
      return;
    auto size = node->getContentSize();
    log::info("Self test tree: {}{} visible {} size {}x{}", std::string(depth * 2, ' '), inspect::name(node),
              node->isVisible(), size.width, size.height);
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      dumpTree(child, depth + 1);
  }

  // Whether the mouse can get to it: under its own middle, with Alt+click if it's inside a button
  bool reachable(CCNode* node)
  {
    auto center = inspect::worldQuad(node).center();
    auto hits = inspect::hitsAt(CCDirector::get()->getRunningScene(), {center.x, center.y}, true);
    return std::find(hits.begin(), hits.end(), node) != hits.end();
  }

  // A label (bitmap or TTF) on the screen: one with an ID outside layouts if there is one, then one with an ID, then any
  CCNode* findLabel(CCNode* root)
  {
    CCNode* inLayout = nullptr;
    CCNode* withoutId = nullptr;
    std::vector<CCNode*> queue{root};
    for (size_t i = 0; i < queue.size(); i++)
    {
      auto node = queue[i];
      if (typeinfo_cast<CCLabelProtocol*>(node))
      {
        if (!reachable(node))
          continue;
        if (node->getID().empty())
          withoutId = withoutId ? withoutId : node;
        else if (!node->getParent()->getLayout())
          return node;
        else
          inLayout = inLayout ? inLayout : node;
        // Its letters aren't labels
        continue;
      }
      for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
        if (child->isVisible())
          queue.push_back(child);
    }
    return inLayout ? inLayout : withoutId;
  }

  struct Case
  {
    Retained<CCNode> node;
    inspect::NodeState before;
    inspect::NodeState after;
  };

  // Press in the middle, drag in steps, let go, two notches of the wheel, an arrow and a Shift+arrow
  void edit(Case& item)
  {
    auto node = item.node.get();
    auto name = inspect::name(node);
    item.before = inspect::stateOf(node);
    auto center = inspect::worldQuad(node).center();
    CCPoint press{center.x, center.y};
    auto worldBefore = worldPositionOf(node);

    // What's under the middle of it, topmost first: other mods may cover or hide the game's nodes
    auto scene = CCDirector::get()->getRunningScene();
    auto hits = inspect::hitsAt(scene, press);
    std::string under;
    for (size_t i = 0; i < hits.size() && i < 5; i++)
      under += (i ? ", " : "") + inspect::name(hits[i]);
    log::info("Self test: under the middle of {}: {}", name, under.empty() ? "nothing" : under);

    editor::pinMouse(press);
    auto at = std::find(hits.begin(), hits.end(), node);
    // Inside a button it's the button that's picked; Alt+click goes inside
    bool deep = false;
    if (at == hits.end())
    {
      auto inside = inspect::hitsAt(scene, press, true);
      if (std::find(inside.begin(), inside.end(), node) != inside.end())
      {
        log::info("Self test: {} is part of {}, Alt+click", name, hits.empty() ? "nothing" : inspect::name(hits[0]));
        hits = inside;
        at = std::find(hits.begin(), hits.end(), node);
        deep = true;
      }
    }
    if (!visibleInTree(node))
    {
      // Hidden by another mod: nothing to pick, edited all the same
      log::info("Self test: {} is hidden (another mod?), grabbed without the mouse", name);
      editor::grab(node, press);
    }
    else
    {
      check(at != hits.end(), fmt::format("{} is under the mouse in its middle", name));
      // Covered by something: Tab until it's the one, as a user would
      for (auto i = at - hits.begin(); at != hits.end() && i > 0; i--)
        editor::nextUnderMouse();
      auto picked = editor::pressAt(press, deep);
      check(picked == node, fmt::format("the mouse in the middle of {} picks it, after {} Tab (picked {})", name,
                                        at - hits.begin(), picked ? inspect::name(picked) : "nothing"));
      if (picked != node)
      {
        editor::release();
        editor::grab(node, press);
      }
    }
    for (int step = 1; step <= 5; step++)
    {
      auto at = press + ccp(DRAG_X, DRAG_Y) * (step / 5.f);
      editor::pinMouse(at);
      editor::dragTo(at);
    }
    editor::release();
    auto moved = worldPositionOf(node) - worldBefore;
    check(close(moved.x, DRAG_X, 0.05f) && close(moved.y, DRAG_Y, 0.05f),
          fmt::format("the drag moved {} by ({}, {}) on the screen, not ({}, {})", name, moved.x, moved.y, DRAG_X,
                      DRAG_Y));

    auto dragged = inspect::stateOf(node);
    editor::wheel(2, false);
    auto scaled = inspect::stateOf(node);
    check(scaled.scaleX > dragged.scaleX && scaled.scaleY > dragged.scaleY,
          fmt::format("the wheel made {} bigger ({} to {})", name, dragged.scaleX, scaled.scaleX));
    check(scaled.position == dragged.position, fmt::format("the wheel didn't move {}", name));

    editor::nudge(1, 0);
    editor::nudge(10, 0);
    item.after = inspect::stateOf(node);
    check(close(item.after.position.x, dragged.position.x + 11) && close(item.after.position.y, dragged.position.y),
          fmt::format("the arrows moved {} by 11 in its parent", name));

    // Why it would act oddly: a hidden node has to say what hides it
    auto facts = inspect::factsOf(node);
    for (auto const& reason : explain::reasons(facts))
      log::info("Self test: {}: {}", name, reason);
    check(visibleInTree(node) == facts.hiddenBy.empty(),
          fmt::format("{} is said to be hidden exactly when it is ({})", name, facts.hiddenBy));

    auto code = editor::codeFor(node);
    log::info("Self test: the code for {}:\n{}", name, code);
    auto problem = inspect::checkCode(node, code);
    check(problem.empty(), fmt::format("the code for {} puts it where it is: {}", name, problem));
  }

  // A drag that ends with the node's center a point off the screen's middle ends right on it; without snapping
  // it doesn't. One step to undo either way
  void finish(bool measure);
  void edgeCases();

  void snapping(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto win = CCDirector::get()->getWinSize();
    for (bool on : {true, false})
    {
      editor::setSnapping(on);
      auto bounds = inspect::worldQuad(node).bounds();
      CCPoint from{bounds.midX(), bounds.midY()};
      // Halfway up the screen too, so it snaps on both axes. On the side where the screen's middle is the closest
      // guide: its parent or a sibling centered a point off the middle is snapped to instead, rightly
      CCPoint middle{win.width / 2, win.height / 2};
      CCPoint to = middle + ccp(1, -1);
      for (auto off : {ccp(1, -1), ccp(1, 1), ccp(-1, -1), ccp(-1, 1)})
      {
        auto snapped = editor::snappedCenter(node, middle + off);
        if (close(snapped.x, middle.x) && close(snapped.y, middle.y))
        {
          to = middle + off;
          break;
        }
      }
      editor::pinMouse(from);
      editor::grab(node, from);
      for (int step = 1; step <= 4; step++)
        editor::dragTo(from + (to - from) * (step / 4.f));
      editor::release();
      auto after = inspect::worldQuad(node).bounds();
      if (on)
        check(close(after.midX(), win.width / 2) && close(after.midY(), win.height / 2),
              fmt::format("{} snapped to the middle of the screen: its center at ({}, {})", name, after.midX(),
                          after.midY()));
      else
        check(close(after.midX(), to.x, 0.05f) && close(after.midY(), to.y, 0.05f),
              fmt::format("{} didn't snap with snapping off: its center at ({}, {})", name, after.midX(),
                          after.midY()));
      check(editor::undo(), "the snapped drag is one step to undo");
      check(inspect::stateOf(node) == before, fmt::format("undo put {} back after the snapped drag", name));
    }
    editor::setSnapping(std::nullopt);
  }

  // Ctrl+arrows move the anchor point and keep the node where it is; [ and ] change the z-order. Both undo
  void anchorAndOrder(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto quad = inspect::worldQuad(node);
    editor::select(node);
    int direction = before.anchor.x > 0.25f ? -1 : 1;
    editor::stepAnchor(direction, direction);
    auto moved = inspect::stateOf(node);
    check(!(moved.anchor == before.anchor), fmt::format("Ctrl+arrows moved the anchor point of {}", name));
    auto after = inspect::worldQuad(node);
    bool still = true;
    for (size_t i = 0; i < 4; i++)
      still = still && close(after.corners[i].x, quad.corners[i].x) && close(after.corners[i].y, quad.corners[i].y);
    check(still, fmt::format("{} stayed where it was with its new anchor point", name));
    auto code = editor::codeFor(node);
    check(code.find("setAnchorPoint") != std::string::npos, fmt::format("the code for {} sets the anchor point", name));
    auto problem = inspect::checkCode(node, code);
    check(problem.empty(), fmt::format("the code with the new anchor point puts {} where it is: {}", name, problem));

    editor::stepZOrder(1);
    check(node->getZOrder() == before.zOrder + 1, fmt::format("] raised the z-order of {}", name));
    check(editor::codeFor(node).find("setZOrder") != std::string::npos, "the code sets the z-order");
    check(editor::undo() && editor::undo(), "the anchor and the z-order are two steps to undo");
    check(inspect::stateOf(node) == before, fmt::format("undo put back the anchor and the z-order of {}", name));
  }

  // Enter, then x, y and the scale typed with the keys a user would press, expressions included
  void typing(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto win = CCDirector::get()->getWinSize();
    editor::select(node);
    editor::startTyping();
    check(editor::typing(), "Enter starts typing values");
    // w/2+40 Tab h-25 Tab 1.5 Enter
    std::vector<std::pair<int, bool>> keys = {
        {'W', false}, {KEY_Slash, false}, {'2', false}, {KEY_OEMEqual, true}, {'4', false}, {'0', false},
        {KEY_Tab, false}, {'H', false}, {KEY_OEMMinus, false}, {'2', false}, {'5', false}, {KEY_Tab, false},
        {'1', false}, {KEY_OEMPeriod, false}, {'5', false}, {KEY_Enter, false}};
    for (auto [key, shift] : keys)
      editor::typingKey(key, shift);
    auto after = inspect::stateOf(node);
    check(!editor::typing(), "Enter applied the values");
    check(close(after.position.x, win.width / 2 + 40) && close(after.position.y, win.height - 25),
          fmt::format("typed w/2+40, h-25 put {} at ({}, {})", name, after.position.x, after.position.y));
    check(close(after.scaleX, 1.5f), fmt::format("typed 1.5 scaled {} to {}", name, after.scaleX));
    auto problem = inspect::checkCode(node, editor::codeFor(node));
    check(problem.empty(), fmt::format("the code for the typed values puts {} where it is: {}", name, problem));

    // Enter, Enter with nothing typed: the exact values stay, not the rounded ones shown, and there's no step
    auto steps = editor::historySize();
    editor::startTyping();
    editor::typingKey(KEY_Enter, false);
    check(inspect::stateOf(node) == after && editor::historySize() == steps,
          "Enter, Enter without typing changes nothing");

    // Something that can't be read stays to be fixed; Esc lets it go
    editor::startTyping();
    for (auto [key, shift] : std::vector<std::pair<int, bool>>{{'P', false}, {KEY_OEMMinus, false}, {KEY_Enter, false}})
      editor::typingKey(key, shift);
    check(editor::typing(), "a value that can't be read isn't applied");
    editor::typingKey(KEY_Escape, false);
    check(!editor::typing() && inspect::stateOf(node) == after, "Esc lets the values go");

    check(editor::undo(), "the typed values are a step to undo");
    check(inspect::stateOf(node) == before, fmt::format("undo put {} back after the typed values", name));

    // The content size, the fourth and fifth fields: 120 x 40
    editor::startTyping();
    for (auto [key, shift] : std::vector<std::pair<int, bool>>{{KEY_Tab, false}, {KEY_Tab, false}, {KEY_Tab, false},
                                                               {'1', false}, {'2', false}, {'0', false}, {KEY_Tab, false},
                                                               {'4', false}, {'0', false}, {KEY_Enter, false}})
      editor::typingKey(key, shift);
    auto size = node->getContentSize();
    check(close(size.width, 120) && close(size.height, 40),
          fmt::format("typed 120 x 40 made {} {} x {}", name, size.width, size.height));
    check(editor::codeFor(node).find("node->setContentSize({120.f, 40.f});") != std::string::npos,
          "the code sets the content size");
    check(editor::undo() && inspect::stateOf(node) == before, "undo put the content size back");

    // V: hidden, in the code, back with undo
    editor::toggleVisible();
    check(!node->isVisible(), fmt::format("V hid {}", name));
    check(editor::codeFor(node).find("node->setVisible(false);") != std::string::npos, "the code hides it");
    check(editor::undo() && node->isVisible() && inspect::stateOf(node) == before, "undo showed it again");

    // R: back to before the first edit, undone back to the edit
    editor::nudge(5, 0);
    auto nudged = inspect::stateOf(node);
    editor::reset();
    check(inspect::stateOf(node) == before, fmt::format("R reset {}", name));
    check(editor::undo() && inspect::stateOf(node) == nudged, "the reset is a step to undo");
    editor::undo();
  }

  // The right side's handle dragged 10 to the right on the screen: wider, the left side where it was
  void resizing(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto quad = inspect::worldQuad(node);
    editor::select(node);
    auto size = node->getContentSize();
    auto handlePoint = node->convertToWorldSpace({size.width, size.height / 2});
    auto handle = editor::handleAt(handlePoint);
    check(handle && handle->x == 1 && handle->y == 0, fmt::format("the right side's handle of {} is under the mouse", name));
    if (!handle)
      return;
    editor::startResize(*handle, handlePoint);
    editor::resizeTo(handlePoint + ccp(10, 0));
    // Nothing else edits it meanwhile: the wheel would be undone by the next frame of the resize
    auto steps = editor::historySize();
    editor::wheel(1, false);
    editor::nudge(1, 0);
    check(editor::historySize() == steps, "the wheel and the arrows wait for the resize to end");
    editor::release();
    auto after = inspect::worldQuad(node);
    check(close(after.corners[0].x, quad.corners[0].x) && close(after.corners[3].y, quad.corners[3].y),
          fmt::format("the left side of {} stayed", name));
    check(close(after.corners[1].x, quad.corners[1].x + 10, 0.05f),
          fmt::format("the right side of {} went 10 to the right ({})", name, after.corners[1].x - quad.corners[1].x));
    check(editor::codeFor(node).find("setContentSize") != std::string::npos, "the code sets the new size");
    check(editor::undo() && inspect::stateOf(node) == before, fmt::format("undo put the size of {} back", name));

    // The game hiding it after an edit (a tab switched) isn't an edit: not in the code, not shown again by undo
    editor::nudge(2, 0);
    node->setVisible(false);
    check(editor::codeFor(node).find("setVisible") == std::string::npos, "the game hiding a node isn't in the code");
    editor::undo();
    check(!node->isVisible(), "undo leaves the game's hiding alone");
    node->setVisible(true);
  }

  // A button in a menu in a menu: the outer one turned by 30, flipped and scaled, the inner one turned back by 10.
  // The mouse has to pick the button, a drag has to move it as far on the screen, its code has to put it where
  // it is, and undo has to put it back
  void transforms()
  {
    auto win = CCDirector::get()->getWinSize();
    auto outer = CCMenu::create();
    outer->setID("selftest-outer"_spr);
    outer->setPosition({win.width / 2, win.height / 2});
    outer->setContentSize({120, 80});
    outer->ignoreAnchorPointForPosition(false);
    outer->setRotation(30);
    outer->setScaleX(-1.5f);
    outer->setScaleY(1.5f);
    auto inner = CCMenu::create();
    inner->setID("selftest-inner"_spr);
    inner->setContentSize({60, 40});
    inner->ignoreAnchorPointForPosition(false);
    inner->setPosition({60, 40});
    inner->setRotation(-10);
    outer->addChild(inner);
    auto sprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    auto button = CCMenuItemSpriteExtra::create(sprite ? static_cast<CCNode*>(sprite) : CCSprite::create(), nullptr, nullptr);
    button->setID("selftest-turned"_spr);
    button->setPosition({30, 20});
    inner->addChild(button);
    CCDirector::get()->getRunningScene()->addChild(outer, 1000);

    auto before = inspect::stateOf(button);
    auto center = inspect::worldQuad(button).center();
    CCPoint press{center.x, center.y};
    auto hits = inspect::hitsAt(CCDirector::get()->getRunningScene(), press);
    check(!hits.empty() && hits[0] == button, fmt::format("the mouse picks the button in the turned, flipped menus ({})",
                                                          hits.empty() ? "nothing" : inspect::name(hits[0])));
    auto worldBefore = worldPositionOf(button);
    editor::setSnapping(false);
    editor::grab(button, press);
    editor::dragTo(press + ccp(10, -6));
    editor::release();
    editor::setSnapping(std::nullopt);
    auto moved = worldPositionOf(button) - worldBefore;
    check(close(moved.x, 10, 0.05f) && close(moved.y, -6, 0.05f),
          fmt::format("the turned button moved by ({}, {}) on the screen, not (10, -6)", moved.x, moved.y));
    auto code = editor::codeFor(button);
    log::info("Self test: the code for a button in turned, flipped menus:\n{}", code);
    auto problem = inspect::checkCode(button, code);
    check(problem.empty(), fmt::format("the code for the turned button puts it where it is: {}", problem));
    check(code.find("parentSize") != std::string::npos, "a turned parent's frame is its own box, not the screen");
    auto facts = inspect::factsOf(button);
    check(facts.parentTurned, "the panel says the parent is turned");
    check(editor::undo() && inspect::stateOf(button) == before, "undo put the turned button back");
    outer->removeFromParent();
  }

  // Shift+click: two nodes dragged, nudged and scaled together; each is one step to undo
  // Alt+A and the like on three nodes of their own, in a turned, flipped and scaled parent: lined up on the
  // screen, one step to undo, their code right; distributed; one node centered in its parent's box
  void aligning()
  {
    auto win = CCDirector::get()->getWinSize();
    auto scene = CCDirector::get()->getRunningScene();
    auto parent = CCNode::create();
    parent->setID("selftest-align-parent"_spr);
    parent->setPosition({win.width / 2, win.height / 2});
    parent->setRotation(20);
    parent->setScaleX(-1.25f);
    parent->setScaleY(1.25f);
    scene->addChild(parent, 1000);
    std::vector<CCNode*> boxes;
    CCPoint places[] = {{-60, -20}, {10, 30}, {45, -35}};
    CCSize sizes[] = {{20, 10}, {30, 14}, {12, 24}};
    for (int i = 0; i < 3; i++)
    {
      auto box = CCLayerColor::create({255, 120, 60, 160}, sizes[i].width, sizes[i].height);
      box->setID(Mod::get()->expandSpriteName(fmt::format("selftest-align-{}", i)));
      box->setPosition(places[i]);
      parent->addChild(box);
      boxes.push_back(box);
    }
    std::vector<inspect::NodeState> before;
    for (auto box : boxes)
      before.push_back(inspect::stateOf(box));
    auto bounds = [](CCNode* node) { return inspect::worldQuad(node).bounds(); };
    auto putBack = [&]
    {
      bool all = true;
      for (size_t i = 0; i < boxes.size(); i++)
        all = all && inspect::stateOf(boxes[i]) == before[i];
      return all;
    };

    editor::select(boxes[0]);
    editor::toggleSelect(boxes[1]);
    editor::toggleSelect(boxes[2]);
    auto steps = editor::historySize();

    float left = std::min({bounds(boxes[0]).minX(), bounds(boxes[1]).minX(), bounds(boxes[2]).minX()});
    editor::alignTo(align::Edge::Left);
    check(close(bounds(boxes[0]).minX(), left, 0.05f) && close(bounds(boxes[1]).minX(), left, 0.05f) &&
              close(bounds(boxes[2]).minX(), left, 0.05f),
          fmt::format("Alt+A lined up the left edges at {} on the screen: {}, {}, {}", left, bounds(boxes[0]).minX(),
                      bounds(boxes[1]).minX(), bounds(boxes[2]).minX()));
    // The leftmost one doesn't move: no change of it in the step
    check(editor::historySize() == steps + 2,
          fmt::format("aligning three nodes moved two: {} changes", editor::historySize() - steps));
    for (auto box : boxes)
    {
      auto problem = inspect::checkCode(box, editor::codeFor(box));
      check(problem.empty(), fmt::format("the code for an aligned node puts it where it is: {}", problem));
    }
    check(editor::undo() && putBack(), "one Ctrl+Z put the three aligned nodes back");
    check(editor::selection().size() == 3, "the three are still selected after Ctrl+Z, the one that didn't move too");

    float top = std::max({bounds(boxes[0]).maxY(), bounds(boxes[1]).maxY(), bounds(boxes[2]).maxY()});
    editor::alignTo(align::Edge::Top);
    check(close(bounds(boxes[0]).maxY(), top, 0.05f) && close(bounds(boxes[1]).maxY(), top, 0.05f) &&
              close(bounds(boxes[2]).maxY(), top, 0.05f),
          fmt::format("Alt+W lined up the top edges at {}: {}, {}, {} ({} selected)", top, bounds(boxes[0]).maxY(),
                      bounds(boxes[1]).maxY(), bounds(boxes[2]).maxY(), editor::selection().size()));
    editor::undo();

    editor::distribute(true);
    std::vector<geom::Rect> after;
    for (auto box : boxes)
      after.push_back(bounds(box));
    std::sort(after.begin(), after.end(), [](auto const& a, auto const& b) { return a.midX() < b.midX(); });
    float gap1 = after[1].minX() - after[0].maxX();
    float gap2 = after[2].minX() - after[1].maxX();
    check(close(gap1, gap2, 0.05f), fmt::format("Alt+Shift+H made the gaps the same: {} and {}", gap1, gap2));
    check(editor::undo() && putBack(), "Ctrl+Z put the distributed nodes back");

    // A value typed with three selected: the scale typed goes to all of them, their positions stay their own
    editor::select(boxes[0]);
    editor::toggleSelect(boxes[1]);
    editor::toggleSelect(boxes[2]);
    editor::startTyping();
    editor::typingKey(KEY_Tab, false);
    editor::typingKey(KEY_Tab, false);
    editor::typingKey('2', false);
    editor::typingKey(KEY_Enter, false);
    bool scaled = true, kept = true;
    for (size_t i = 0; i < boxes.size(); i++)
    {
      scaled = scaled && close(boxes[i]->getScaleX(), 2) && close(boxes[i]->getScaleY(), 2);
      kept = kept && boxes[i]->getPosition() == before[i].position;
    }
    check(scaled, "a scale typed with three selected scales all three");
    check(kept, "the positions not typed into stay each node's own");
    check(editor::undo() && putBack(), "one Ctrl+Z put the three back");

    // One node: to its parent's box
    auto frame = CCLayerColor::create({60, 120, 255, 80}, 160, 90);
    frame->setPosition({40, 40});
    scene->addChild(frame, 1000);
    auto inside = CCLayerColor::create({255, 255, 255, 160}, 30, 20);
    inside->setPosition({10, 12});
    frame->addChild(inside);
    editor::select(inside);
    editor::alignTo(align::Edge::CenterX);
    editor::alignTo(align::Edge::CenterY);
    check(close(bounds(inside).midX(), bounds(frame).midX(), 0.05f) &&
              close(bounds(inside).midY(), bounds(frame).midY(), 0.05f),
          "Alt+H and Alt+V centered one node in its parent");
    check(close(inside->getPositionX(), 65, 0.05f) && close(inside->getPositionY(), 35, 0.05f),
          fmt::format("the centered node is at (65, 35) in its parent: ({}, {})", inside->getPositionX(),
                      inside->getPositionY()));
    editor::undo();
    editor::undo();
    check(close(inside->getPositionX(), 10) && close(inside->getPositionY(), 12), "two Ctrl+Z put it back");

    // Keys through Geode's own event, as a key press comes: Alt with an arrow still nudges (Alt shows the
    // distances meanwhile), Alt+H lines up; a menu selected with its child moves alone, the child with it
    auto press = [](enumKeyCodes key, uint8_t modifiers)
    {
      KeyboardInputData data(key, KeyboardInputData::Action::Press, {0, 0}, 0, KeyboardModifier(modifiers));
      KeyboardInputEvent(key).send(data);
      KeyboardInputData up(key, KeyboardInputData::Action::Release, {0, 0}, 0, KeyboardModifier(modifiers));
      KeyboardInputEvent(key).send(up);
    };
    editor::select(inside);
    auto nudgedFrom = inside->getPositionX();
    press(KEY_Right, KeyboardModifier::Alt);
    check(close(inside->getPositionX(), nudgedFrom + 1), "Alt+Right nudges as Right does");
    editor::undo();
    editor::select(frame);
    editor::toggleSelect(inside);
    auto insideBefore = inspect::stateOf(inside);
    press(KEY_H, KeyboardModifier::Alt);
    // The parent alone is lined up, to the screen's middle as one node is; the child goes with it
    check(inspect::stateOf(inside) == insideBefore, "a child selected with its parent isn't aligned on its own");
    check(close(bounds(frame).midX(), win.width / 2, 0.05f), "Alt+H centered the parent on the screen");
    editor::undo();

    editor::select(nullptr);
    frame->removeFromParent();
    parent->removeFromParent();
  }

  // The code L copies, done as it says with Geode's own AxisLayout: a new container with its size, layout and
  // position, the nodes moved into it in the code's order, laid out. Each node has to stay where it was on the screen
  void applyLayout(std::string const& code, CCNode* parent, std::vector<CCNode*> nodes, std::string const& what)
  {
    auto win = CCDirector::get()->getWinSize();
    auto parentSize = parent->getContentSize();
    auto lookup = [&](std::string_view name) -> std::optional<float>
    {
      if (name == "winSize.width")
        return win.width;
      if (name == "winSize.height")
        return win.height;
      if (name == "parentSize.width")
        return parentSize.width;
      if (name == "parentSize.height")
        return parentSize.height;
      return std::nullopt;
    };
    auto size = expr::callArguments(code, "setContentSize");
    auto gap = expr::callArguments(code, "setGap");
    auto position = expr::callArguments(code, "setPosition");
    auto cross = expr::callArguments(code, "setCrossAxisLineAlignment");
    bool row = code.find("RowLayout::create()") != std::string::npos;
    bool reverse = code.find("setAxisReverse(true)") != std::string::npos;
    if (size.size() != 2 || gap.size() != 1 || position.size() != 2 || cross.size() != 1)
    {
      check(false, fmt::format("{}: the layout's code can be read", what));
      return;
    }
    auto width = expr::evaluate(size[0], lookup), height = expr::evaluate(size[1], lookup);
    auto x = expr::evaluate(position[0], lookup), y = expr::evaluate(position[1], lookup);
    auto spacing = expr::evaluate(gap[0], lookup);
    if (!width || !height || !x || !y || !spacing)
    {
      check(false, fmt::format("{}: the layout's numbers can be read", what));
      return;
    }
    auto alignment = cross[0] == "AxisAlignment::Start" ? AxisAlignment::Start
                     : cross[0] == "AxisAlignment::End" ? AxisAlignment::End
                                                         : AxisAlignment::Center;

    std::vector<geom::Rect> before;
    for (auto node : nodes)
      before.push_back(inspect::worldQuad(node).bounds());
    // The code's order: where each node's ID is in it
    auto order = nodes;
    std::sort(order.begin(), order.end(),
              [&](CCNode* a, CCNode* b) { return code.find(std::string(a->getID())) < code.find(std::string(b->getID())); });

    CCNode* container = code.find("CCMenu::create()") != std::string::npos ? CCMenu::create() : CCNode::create();
    container->ignoreAnchorPointForPosition(false);
    if (code.find("setAnchorPoint({0.5f, 0.5f})") != std::string::npos)
      container->setAnchorPoint({0.5f, 0.5f});
    container->setContentSize({*width, *height});
    auto layout = row ? static_cast<AxisLayout*>(RowLayout::create()) : static_cast<AxisLayout*>(ColumnLayout::create());
    layout->setGap(*spacing)
        ->setAxisAlignment(AxisAlignment::Center)
        ->setCrossAxisLineAlignment(alignment)
        ->setAutoScale(false);
    if (reverse)
      layout->setAxisReverse(true);
    container->setLayout(layout);
    parent->addChild(container);
    container->setPosition({*x, *y});
    for (auto node : order)
    {
      node->retain();
      node->removeFromParent();
      container->addChild(node);
      node->release();
    }
    container->updateLayout();

    float worst = 0;
    std::string moves;
    for (size_t i = 0; i < nodes.size(); i++)
    {
      auto after = inspect::worldQuad(nodes[i]).bounds();
      worst = std::max({worst, std::fabs(after.x - before[i].x), std::fabs(after.y - before[i].y)});
      moves += fmt::format(" ({}, {}) -> ({}, {})", before[i].x, before[i].y, after.x, after.y);
    }
    if (worst >= 0.1f)
      log::info("Self test: {} moved:{}", what, moves);
    check(worst < 0.1f, fmt::format("{}: the layout's code puts every node where it was (off by {} at most)", what, worst));
    container->removeFromParent();
  }

  // L on three buttons lined up by Alt+S and Alt+Shift+H: a row; on three sprites by Alt+D and Alt+Shift+V: a
  // column. Each layout's code done as it says leaves them where they were
  void layouts(CCNode* layer)
  {
    auto win = CCDirector::get()->getWinSize();
    auto menu = CCMenu::create();
    menu->setID("selftest-row-parent"_spr);
    menu->ignoreAnchorPointForPosition(false);
    menu->setContentSize({320, 90});
    menu->setPosition({win.width / 2, 90});
    layer->addChild(menu, 1000);
    std::vector<CCNode*> buttons;
    float scales[] = {1.f, 0.8f, 1.3f};
    CCPoint places[] = {{40, 30}, {150, 50}, {250, 40}};
    for (int i = 0; i < 3; i++)
    {
      auto sprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto button = CCMenuItemSpriteExtra::create(sprite ? static_cast<CCNode*>(sprite) : CCSprite::create(), nullptr, nullptr);
      button->setID(Mod::get()->expandSpriteName(fmt::format("selftest-row-{}", i)));
      button->setScale(scales[i]);
      button->m_baseScale = scales[i];
      button->setPosition(places[i]);
      menu->addChild(button);
      buttons.push_back(button);
    }
    editor::select(buttons[0]);
    editor::toggleSelect(buttons[1]);
    editor::toggleSelect(buttons[2]);
    check(editor::layoutCode().find("Their gaps differ") != std::string::npos,
          "the layout's code says when the gaps differ");
    editor::alignTo(align::Edge::Bottom);
    editor::distribute(true);
    auto code = editor::layoutCode();
    log::info("Self test: the layout for three buttons:\n{}", code);
    check(code.find("RowLayout::create()") != std::string::npos && code.find("CCMenu::create()") != std::string::npos &&
              code.find("AxisAlignment::Start") != std::string::npos && code.find("Their gaps differ") == std::string::npos,
          "three buttons lined up at the bottom are a row of a menu, aligned at the start");
    applyLayout(code, menu, buttons, "the row");

    auto column = CCNode::create();
    column->setID("selftest-column-parent"_spr);
    column->setContentSize({120, 200});
    column->setPosition({60, 60});
    layer->addChild(column, 1000);
    std::vector<CCNode*> sprites;
    CCPoint spots[] = {{40, 170}, {70, 100}, {55, 30}};
    for (int i = 0; i < 3; i++)
    {
      auto sprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      if (!sprite)
        sprite = CCSprite::create();
      sprite->setID(Mod::get()->expandSpriteName(fmt::format("selftest-column-{}", i)));
      sprite->setScale(0.7f + 0.2f * i);
      sprite->setPosition(spots[i]);
      column->addChild(sprite);
      sprites.push_back(sprite);
    }
    editor::select(sprites[0]);
    editor::toggleSelect(sprites[1]);
    editor::toggleSelect(sprites[2]);
    editor::alignTo(align::Edge::Right);
    editor::distribute(false);
    code = editor::layoutCode();
    log::info("Self test: the layout for three sprites:\n{}", code);
    check(code.find("ColumnLayout::create()") != std::string::npos && code.find("CCNode::create()") != std::string::npos &&
              code.find("AxisAlignment::End") != std::string::npos,
          "three sprites lined up at the right are a column of a plain node, aligned at the end");
    applyLayout(code, column, sprites, "the column");

    // Nodes of different parents: no layout
    editor::select(buttons[0]);
    editor::toggleSelect(sprites[0]);
    check(editor::layoutCode().empty(), "nodes of two parents have no layout");
    editor::select(nullptr);
    menu->removeFromParent();
    column->removeFromParent();
  }

  // Ctrl+wheel turns a node, typed values give it an opacity: in its code, one step each to undo. A node the game
  // turns by itself (an action) isn't turned in the code
  void turning()
  {
    auto win = CCDirector::get()->getWinSize();
    auto scene = CCDirector::get()->getRunningScene();
    auto sprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    if (!sprite)
      sprite = CCSprite::create();
    sprite->setID("selftest-turned-sprite"_spr);
    sprite->setPosition({win.width / 2 + 60, win.height / 2});
    scene->addChild(sprite, 1000);
    auto before = inspect::stateOf(sprite);
    editor::select(sprite);
    auto steps = editor::historySize();
    editor::turn(1, false);
    editor::turn(1, true);
    check(close(sprite->getRotation(), -16), fmt::format("Ctrl+wheel up turned it by 15 then 1: {}", sprite->getRotation()));
    check(editor::historySize() == steps + 1, "turns in a row are one step");
    // Opacity: the seventh field
    editor::startTyping();
    for (int i = 0; i < 6; i++)
      editor::typingKey(KEY_Tab, false);
    for (int key : {'1', '2', '8'})
      editor::typingKey(key, false);
    editor::typingKey(KEY_Enter, false);
    check(sprite->getOpacity() == 128, fmt::format("typed 128 into the opacity: {}", static_cast<int>(sprite->getOpacity())));
    auto code = editor::codeFor(sprite);
    check(code.find("node->setRotation(-16.f);") != std::string::npos &&
              code.find("rgba->setOpacity(128);") != std::string::npos,
          "the code turns it and sets its opacity");
    // 256 can't be an opacity: it stays to be fixed
    editor::startTyping();
    for (int i = 0; i < 6; i++)
      editor::typingKey(KEY_Tab, false);
    for (int key : {'2', '5', '6'})
      editor::typingKey(key, false);
    editor::typingKey(KEY_Enter, false);
    check(editor::typing() && sprite->getOpacity() == 128, "an opacity over 255 isn't taken");
    editor::typingKey(KEY_Escape, false);
    editor::undo();
    editor::undo();
    check(inspect::stateOf(sprite) == before, "two Ctrl+Z took the opacity and the turn off");

    // Turned by an action, only dragged here: no rotation in its code, and undo doesn't stop the turn
    sprite->runAction(CCRepeatForever::create(CCRotateBy::create(1.f, 90.f)));
    sprite->setRotation(10);
    auto center = inspect::worldQuad(sprite).center();
    editor::setSnapping(false);
    editor::grab(sprite, {center.x, center.y});
    editor::dragTo(CCPoint{center.x, center.y} + ccp(5, 5));
    editor::release();
    editor::setSnapping(std::nullopt);
    sprite->setRotation(40);
    check(editor::codeFor(sprite).find("setRotation") == std::string::npos, "a node the game turns isn't turned in the code");
    editor::undo();
    check(close(sprite->getRotation(), 40), "undoing its drag leaves the game's turn alone");
    sprite->stopAllActions();
    editor::select(nullptr);
    sprite->removeFromParent();
  }

  // The grid: a node dragged by an odd amount ends where its code's offsets are whole steps, on the screen and in a
  // scaled parent; its code puts it there. Under a turned parent, whose axes aren't the screen's, there's no grid
  void gridding()
  {
    auto win = CCDirector::get()->getWinSize();
    auto scene = CCDirector::get()->getRunningScene();
    auto onScreen = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    if (!onScreen)
      onScreen = CCSprite::create();
    onScreen->setID("selftest-grid-sprite"_spr);
    onScreen->setPosition({win.width / 2 + 31.3f, win.height / 2 - 12.2f});
    scene->addChild(onScreen, 1000);
    auto box = CCLayerColor::create({90, 200, 120, 90}, 160, 100);
    box->setID("selftest-grid-box"_spr);
    box->ignoreAnchorPointForPosition(false);
    box->setPosition({win.width / 2 - 100, win.height / 2 + 40});
    box->setScale(1.3f);
    scene->addChild(box, 1000);
    auto inBox = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    if (!inBox)
      inBox = CCSprite::create();
    inBox->setID("selftest-grid-inner"_spr);
    inBox->setPosition({33.3f, 71.1f});
    box->addChild(inBox);

    editor::setSnapping(false);
    editor::setGrid(5);
    for (auto node : {static_cast<CCNode*>(onScreen), static_cast<CCNode*>(inBox)})
    {
      auto name = inspect::name(node);
      auto center = inspect::worldQuad(node).center();
      editor::grab(node, {center.x, center.y});
      editor::dragTo(CCPoint{center.x, center.y} + ccp(13.3f, -7.7f));
      editor::release();
      auto position = codegen::positionExpressions(inspect::describe(node));
      auto whole = [](float offset) { return std::fabs(offset / 5 - std::round(offset / 5)) < 1e-3f; };
      check(whole(position.x.offset) && whole(position.y.offset),
            fmt::format("{} dragged on a grid of 5: its code's offsets {} and {} are whole steps", name,
                        position.x.offset, position.y.offset));
      auto problem = inspect::checkCode(node, editor::codeFor(node));
      check(problem.empty(), fmt::format("the code for {} on the grid puts it where it is: {}", name, problem));
      editor::undo();
    }
    box->setRotation(20);
    auto worldBefore = worldPositionOf(inBox);
    auto center = inspect::worldQuad(inBox).center();
    editor::grab(inBox, {center.x, center.y});
    editor::dragTo(CCPoint{center.x, center.y} + ccp(13.3f, -7.7f));
    editor::release();
    auto moved = worldPositionOf(inBox) - worldBefore;
    check(close(moved.x, 13.3f, 0.05f) && close(moved.y, -7.7f, 0.05f),
          fmt::format("under a turned parent the grid leaves the drag as it is: ({}, {})", moved.x, moved.y));
    editor::undo();
    editor::setGrid(std::nullopt);
    editor::setSnapping(std::nullopt);
    editor::select(nullptr);
    onScreen->removeFromParent();
    box->removeFromParent();
  }

  void several(CCNode* first, CCNode* second)
  {
    auto a = inspect::stateOf(first);
    auto b = inspect::stateOf(second);
    auto worldA = worldPositionOf(first);
    auto worldB = worldPositionOf(second);
    editor::select(first);
    editor::toggleSelect(second);
    check(editor::selection().size() == 2, "Shift+click adds a second node to the selection");
    auto steps = editor::historySize();

    auto center = inspect::worldQuad(first).center();
    CCPoint from{center.x, center.y};
    editor::setSnapping(false);
    editor::grab(first, from);
    editor::dragTo(from + ccp(12, 8));
    editor::release();
    editor::setSnapping(std::nullopt);
    auto movedA = worldPositionOf(first) - worldA;
    auto movedB = worldPositionOf(second) - worldB;
    check(close(movedA.x, 12, 0.05f) && close(movedA.y, 8, 0.05f) && close(movedB.x, 12, 0.05f) &&
              close(movedB.y, 8, 0.05f),
          fmt::format("both moved by (12, 8) on the screen: ({}, {}) and ({}, {})", movedA.x, movedA.y, movedB.x,
                      movedB.y));
    editor::nudge(1, 0);
    editor::nudge(1, 0);
    editor::wheel(1, false);
    check(editor::historySize() == steps + 6, fmt::format("a drag, two arrows and the wheel on two nodes: {} steps",
                                                          editor::historySize() - steps));
    check(editor::undo() && editor::undo() && editor::undo(), "three steps to undo");
    check(inspect::stateOf(first) == a && inspect::stateOf(second) == b, "undo put both back");
    check(editor::selection().size() == 2, "undo selected both");
    check(editor::redo(), "redo");
    check(close(worldPositionOf(second).x - worldB.x, 12, 0.05f), "redo moved the second one again");
    editor::undo();
    editor::toggleSelect(second);
    check(editor::selection().size() == 1, "Shift+click takes a node out of the selection");
  }

  void run(MenuLayer* layer)
  {
    log::info("Self test: start");
    editor::setActive(true);
    // The plain drags are checked to the point: no snapping in them
    editor::setSnapping(false);
    check(editor::active(), "the edit mode is on");
    check(editor::swallowsClick(), "a click in the edit mode is kept from the game");

    std::vector<Case> cases;
    for (auto id : {"play-button", "geode.loader/geode-button"})
    {
      auto node = layer->getChildByIDRecursive(id);
      check(node != nullptr, fmt::format("{} is in the main menu", id));
      if (node)
        cases.push_back({Retained<CCNode>(node), {}, {}});
    }
    // Another mod's menu may be beside MenuLayer in the scene, not in it
    auto label = findLabel(CCDirector::get()->getRunningScene());
    check(label != nullptr, "a label is on the screen");
    if (!label)
      dumpTree(CCDirector::get()->getRunningScene(), 0);
    if (label)
      cases.push_back({Retained<CCNode>(label), {}, {}});

    for (auto& item : cases)
      edit(item);

    // What a button shows: its sprite's frame name, to make one like it
    for (auto& item : cases)
      if (typeinfo_cast<CCMenuItemSpriteExtra*>(item.node.get()))
      {
        auto look = inspect::lookOf(item.node.get());
        log::info("Self test: {} shows {}", inspect::name(item.node.get()), look);
        check(!look.empty(), fmt::format("the frame of {} is found", inspect::name(item.node.get())));
      }

    auto edited = editor::editedNodes();
    check(edited.size() == cases.size(), fmt::format("{} nodes edited, {} expected", edited.size(), cases.size()));
    auto all = editor::codeForAll();
    log::info("Self test: the code for every edited node:\n{}", all);
    for (auto& item : cases)
    {
      auto id = std::string(item.node->getID());
      check(all.find("// " + inspect::name(item.node.get())) != std::string::npos,
            fmt::format("the code for all has {}", inspect::name(item.node.get())));
    }

    int undone = 0;
    while (editor::undo())
      undone++;
    log::info("Self test: {} steps undone", undone);
    // A drag, the wheel and the two arrows (merged) for each
    check(undone == static_cast<int>(cases.size()) * 3, fmt::format("{} steps undone, {} expected", undone,
                                                                     cases.size() * 3));
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.before,
            fmt::format("undo put {} back", inspect::name(item.node.get())));

    int redone = 0;
    while (editor::redo())
      redone++;
    check(redone == undone, fmt::format("{} steps redone, {} undone", redone, undone));
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.after,
            fmt::format("redo put {} where it was edited to", inspect::name(item.node.get())));
    while (editor::undo())
    {
    }
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.before,
            fmt::format("undo after redo put {} back", inspect::name(item.node.get())));

    std::vector<CCNode*> shown;
    for (auto& item : cases)
      if (visibleInTree(item.node.get()))
        shown.push_back(item.node.get());
    if (!shown.empty())
    {
      snapping(shown[0]);
      anchorAndOrder(shown[0]);
      typing(shown[0]);
      transforms();
      aligning();
      layouts(layer);
      turning();
      gridding();
      // A colored box of its own: a node whose size is what it looks like, big enough for every handle
      {
        auto box = CCLayerColor::create({80, 160, 255, 120}, 60, 30);
        box->setID("selftest-box"_spr);
        box->setPosition({100, 100});
        CCDirector::get()->getRunningScene()->addChild(box, 1000);
        resizing(box);
        box->removeFromParent();
      }
      if (shown.size() > 1)
        several(shown[0], shown[1]);
    }

    edgeCases();
  }

  // For the screenshot: the Geode button (or a label) selected
  void prepareScreenshot()
  {
    auto scene = CCDirector::get()->getRunningScene();
    if (auto node = scene->getChildByIDRecursive("geode.loader/geode-button"); node && visibleInTree(node))
      editor::select(node);
    else if (auto label = findLabel(scene))
      editor::select(label);

    // UI_TWEAKER_SELFTEST=aspect: the other screens' shapes on, for a screenshot of them
    if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "aspect")
      editor::setAspectPreview(true);
    // =code: the selected node's code in the panel
    if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "code")
      editor::setCodeView(true);
    // =grid: in the middle of a drag on a grid of 10, the grid drawn
    if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "grid")
    {
      // A node outside layouts: the title, if the main menu has its own
      if (auto title = scene->getChildByIDRecursive("main-title"); title && visibleInTree(title))
        editor::select(title);
      if (auto node = editor::selected())
      {
        // The grid alone, no guides: the code's numbers are whole tens
        editor::setGrid(10);
        editor::setSnapping(false);
        auto center = inspect::worldQuad(node).center();
        editor::grab(node, {center.x, center.y});
        editor::dragTo(CCPoint{center.x, center.y} + ccp(-23, 11));
      }
    }
  }

  // The mouse over a node the edit mode would hover there, not under the panel (so a few frames after the
  // selection, the panel having moved out of its way), and that has distances to the selected one: a sibling of
  // it if there is one, else a label, else anything on the screen. A label filling its button exactly has none to
  // it (another mod's menu has such buttons). Whether there's one
  bool pointAtMeasurable()
  {
    auto scene = CCDirector::get()->getRunningScene();
    auto selected = editor::selected();
    // =grid ends in the middle of a drag: nothing is hovered during one
    if (!selected || std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "grid")
      return false;
    auto measurable = [&](CCNode* node)
    {
      if (node == selected || !visibleInTree(node) || node->getContentWidth() <= 0)
        return false;
      auto center = inspect::worldQuad(node).center();
      if (editor::overPanel({center.x, center.y}))
        return false;
      auto hits = inspect::hitsAt(scene, {center.x, center.y});
      if (hits.empty() || hits[0] != node)
        return false;
      return !measure::between(inspect::worldQuad(selected).bounds(), inspect::worldQuad(node).bounds()).empty();
    };
    std::vector<CCNode*> first;
    for (auto sibling : CCArrayExt<CCNode*>(selected->getParent()->getChildren()))
      first.push_back(sibling);
    if (auto label = findLabel(scene))
      first.push_back(label);
    CCNode* other = nullptr;
    for (auto node : first)
      if (!other && measurable(node))
        other = node;
    // Then the whole scene, breadth first
    std::vector<CCNode*> queue{scene};
    for (size_t i = 0; !other && i < queue.size(); i++)
    {
      if (measurable(queue[i]))
        other = queue[i];
      for (auto child : CCArrayExt<CCNode*>(queue[i]->getChildren()))
        if (child->isVisible())
          queue.push_back(child);
    }
    if (!other)
      return false;
    auto center = inspect::worldQuad(other).center();
    editor::pinMouse(CCPoint{center.x, center.y});
    // The distances as with Alt held: the overlay measures them when it draws, so the result is a few frames away
    editor::setMeasuring(true);
    return true;
  }

  // ! --- Over several frames --- !

  // Steps run one after another, each followed by its number of frames for the game to go on
  constexpr int CROWD = 5000;

  // A popup as a mod makes one: Geode's Popup, a button put in its menu with addChildAtPosition, which gives it
  // AnchorLayoutOptions in the menu's AnchorLayout
  class TestPopup : public Popup
  {
  public:
    static TestPopup* create()
    {
      auto popup = new TestPopup();
      if (popup->setup())
      {
        popup->autorelease();
        return popup;
      }
      delete popup;
      return nullptr;
    }

    void close() { onClose(nullptr); }

  private:
    bool setup()
    {
      if (!Popup::init(260.f, 160.f))
        return false;
      setTitle("UI Tweaker self test");
      auto button = CCMenuItemSpriteExtra::create(ButtonSprite::create("Test"), this, nullptr);
      button->setID("selftest-button"_spr);
      m_buttonMenu->addChildAtPosition(button, Anchor::BottomRight, {-50, 25});
      return true;
    }
  };

  // The anchor and the offset of generated AnchorLayoutOptions, put on the node as the code would
  bool applyAnchorCode(CCNode* node, std::string const& code)
  {
    auto offset = expr::callArguments(code, "setOffset");
    auto anchor = expr::callArguments(code, "setAnchor");
    if (offset.size() != 2 || anchor.size() != 1)
      return false;
    auto none = [](std::string_view) -> std::optional<float> { return std::nullopt; };
    auto x = expr::evaluate(offset[0], none);
    auto y = expr::evaluate(offset[1], none);
    static std::pair<char const*, Anchor> const anchors[] = {
        {"Center", Anchor::Center}, {"TopLeft", Anchor::TopLeft}, {"Top", Anchor::Top},
        {"TopRight", Anchor::TopRight}, {"Right", Anchor::Right}, {"BottomRight", Anchor::BottomRight},
        {"Bottom", Anchor::Bottom}, {"BottomLeft", Anchor::BottomLeft}, {"Left", Anchor::Left},
    };
    for (auto const& [name, value] : anchors)
      if (x && y && anchor[0] == std::string("Anchor::") + name)
      {
        node->setLayoutOptions(AnchorLayoutOptions::create()->setAnchor(value)->setOffset({*x, *y}));
        return true;
      }
    return false;
  }

  double now()
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  struct Step
  {
    int frames;
    std::function<void()> run;
    // What the step waits for before it runs, up to 15 s more
    std::function<bool()> ready = nullptr;
  };

  // The running scene isn't a transition anymore
  bool sceneSettled()
  {
    auto scene = CCDirector::get()->getRunningScene();
    return scene && !typeinfo_cast<CCTransitionScene*>(scene);
  }

  std::vector<Step> g_steps;
  size_t g_next = 0;
  int g_wait = 0;
  double g_until = 0;

  // A step's frames are also as long as they'd last at 60 fps: a game without a frame limit draws hundreds a
  // second, and a scene's transition takes its time whatever the frame rate
  void tick()
  {
    if (g_wait > 0 || now() < g_until)
    {
      if (g_wait > 0)
        g_wait--;
      Loader::get()->queueInMainThread(tick);
      return;
    }
    if (g_next >= g_steps.size())
      return;
    static double waited = 0;
    if (auto& ready = g_steps[g_next].ready; ready && !ready())
    {
      if (waited == 0)
        waited = now();
      if (now() - waited < 15)
      {
        Loader::get()->queueInMainThread(tick);
        return;
      }
    }
    waited = 0;
    auto step = g_steps[g_next++];
    step.run();
    g_wait = step.frames;
    g_until = now() + step.frames / 60.0;
    Loader::get()->queueInMainThread(tick);
  }

  // The first main level, the edit mode on in it: a label of the level's interface dragged, its code checked,
  // undone. The level plays on meanwhile, the player dies and starts again
  void playLevel()
  {
    g_steps.push_back({180,
                       []
                       {
                         auto level = GameLevelManager::get()->getMainLevel(1, false);
                         CCDirector::get()->replaceScene(PlayLayer::scene(level, false, false));
                       }});
    g_steps.push_back({30,
                       []
                       {
                         auto scene = CCDirector::get()->getRunningScene();
                         check(scene && scene->getChildByType<PlayLayer>(0), "the level is playing");
                         check(editor::active(), "the edit mode is on in the level");
                         auto label = findLabel(scene);
                         check(label != nullptr, "a label is in the level");
                         if (!label)
                           return;
                         Case item{Retained<CCNode>(label), {}, {}};
                         edit(item);
                         int undone = 0;
                         while (editor::undo())
                           undone++;
                         check(inspect::stateOf(label) == item.before, "undo put the level's label back");
                         editor::select(label);
                         editor::pinMouse(std::nullopt);
                         editor::setMeasuring(std::nullopt);
                         // The cursor hidden and locked as a level does: shown and let go. It locks only with GD's
                         // lock cursor option on, so that's found out first
                         auto view = CCEGLView::get();
                         editor::setActive(false);
                         view->toggleLockCursor(true);
                         g_lockable = view->getCursorLocked();
                         view->toggleLockCursor(false);
                         editor::setActive(true);
                         log::info("Self test: the cursor {} locked here", g_lockable ? "can be" : "can't be (GD's option is off)");
                         view->showCursor(false);
                         view->toggleLockCursor(true);
                       }});
    g_steps.push_back({3,
                       []
                       {
                         auto view = CCEGLView::get();
                         check(!view->getShouldHideCursor() && !view->getCursorLocked(),
                               "the edit mode shows and lets go of the cursor a level hid and locked");
                         editor::setActive(false);
                         check(view->getShouldHideCursor() && view->getCursorLocked() == g_lockable,
                               fmt::format("leaving the edit mode in the level hides ({}) and locks ({}) the cursor again",
                                           view->getShouldHideCursor(), view->getCursorLocked()));
                         view->showCursor(true);
                         view->toggleLockCursor(false);
                         editor::setActive(true);
                         check(!view->getShouldHideCursor() && !view->getCursorLocked(), "and nothing to put back now");
                       }});
    g_steps.push_back({1,
                       []
                       {
                         g_frames = 0;
                         finish(false);
                       }});
  }

  // Whatever is under a grid of points in the scene: each node dragged, its code read back, undone. The nodes of
  // a real scene, with their layouts, turned parents and nested menus, not the ones picked for the test
  void tourScene(std::string const& name)
  {
    auto scene = CCDirector::get()->getRunningScene();
    auto win = CCDirector::get()->getWinSize();
    std::set<CCNode*> seen;
    int edited = 0;
    int paths = 0;
    int deepPaths = 0;
    editor::setSnapping(false);
    for (int gx = 1; gx <= 7; gx++)
      for (int gy = 1; gy <= 5; gy++)
      {
        CCPoint point{win.width * gx / 8, win.height * gy / 6};
        // The whole thing under the point, and with Alt the part of it (a sprite in a button, its label)
        for (bool deep : {false, true})
        {
          auto hits = inspect::hitsAt(scene, point, deep);
          if (hits.empty() || seen.contains(hits[0]) || !hits[0]->getParent())
            continue;
          auto node = hits[0];
          seen.insert(node);
          auto nodeName = inspect::name(node);
          auto before = inspect::stateOf(node);
          auto worldBefore = worldPositionOf(node);
          editor::grab(node, point);
          editor::dragTo(point + ccp(7, -5));
          editor::release();
          auto moved = worldPositionOf(node) - worldBefore;
          check(close(moved.x, 7, 0.1f) && close(moved.y, -5, 0.1f),
                fmt::format("{}: {} moved by ({}, {}), not (7, -5)", name, nodeName, moved.x, moved.y));
          auto problem = inspect::checkCode(node, editor::codeFor(node));
          check(problem.empty(), fmt::format("{}: the code for {} puts it where it is: {}", name, nodeName, problem));
          // Without an ID: the code's way down to it leads to it
          auto desc = inspect::describe(node);
          if ((desc.id.empty() || !desc.idUnique) && !desc.path.empty())
          {
            auto found = inspect::followPath(node, desc);
            std::string way;
            for (auto const& condition : codegen::lookup(desc))
              way += "\n  if (" + condition + ")";
            check(found == node, fmt::format("{}: the code's path to {} (hook in {}) leads to {}:{}", name, nodeName,
                                             desc.layer, found ? inspect::name(found) : "nothing", way));
            paths++;
            // The first one in a scene through parents without an ID, as an example
            if (desc.path.size() > 1 && deepPaths++ == 0)
              log::info("Self test tour: {}: the code for {}, down through its parents:{}{}", name, nodeName, NEWLINE,
                        editor::codeFor(node));
          }
          editor::undo();
          check(inspect::stateOf(node) == before, fmt::format("{}: undo put {} back", name, nodeName));
          edited++;
        }
      }
    editor::setSnapping(std::nullopt);
    log::info("Self test tour: {}: {} nodes dragged, their code read back, undone; {} without an ID found by "
              "their path ({} through parents without one)",
              name, edited, paths, deepPaths);
    // A level is mostly its background under the grid
    check(edited >= 1, fmt::format("{}: {} nodes under the grid", name, edited));
  }

  // Scene after scene of the game, each looked through by tourScene
  void tour()
  {
    auto visit = [](std::string name, std::function<CCScene*()> make)
    {
      g_steps.push_back({90, [make] { CCDirector::get()->replaceScene(make()); }});
      g_steps.push_back({2, [name] { tourScene(name); }});
    };
    visit("MenuLayer", [] { return MenuLayer::scene(false); });
    visit("CreatorLayer", [] { return CreatorLayer::scene(); });
    visit("GJGarageLayer", [] { return GJGarageLayer::scene(); });
    visit("LevelSelectLayer", [] { return LevelSelectLayer::scene(0); });
    visit("LevelSearchLayer", [] { return LevelSearchLayer::scene(0); });
    // The player's own levels (a list of TableView cells) and one of them, nothing from the servers
    visit("LevelBrowserLayer", [] { return LevelBrowserLayer::scene(GJSearchObject::create(SearchType::MyLevels)); });
    visit("EditLevelLayer", [] { return EditLevelLayer::scene(GJGameLevel::create()); });
    // Popups over the main menu, as a mod's own would be: Geode's quick popup, the settings, the player's profile
    auto popup = [](std::string name, std::function<void()> open)
    {
      g_steps.push_back({90, [] { CCDirector::get()->replaceScene(MenuLayer::scene(false)); }});
      g_steps.push_back({60, [open] { open(); }, sceneSettled});
      g_steps.push_back({2, [name] { tourScene(name); }});
    };
    popup("FLAlertLayer", [] { createQuickPopup("UI Tweaker", "A popup like a mod's own", "OK", nullptr, nullptr); });
    popup("OptionsLayer", [] { OptionsLayer::create()->showLayer(true); });
    popup("MoreOptionsLayer", [] { MoreOptionsLayer::create()->show(); });
    popup("ProfilePage", [] { ProfilePage::create(GJAccountManager::get()->m_accountID, true)->show(); });
    visit("PlayLayer", [] { return PlayLayer::scene(GameLevelManager::get()->getMainLevel(1, false), false, false); });
    // Paused in the level: the pause menu's nodes
    g_steps.push_back({60,
                       []
                       {
                         if (auto play = PlayLayer::get())
                           play->pauseGame(false);
                       }});
    g_steps.push_back({2, [] { tourScene("PauseLayer"); }});
    g_steps.push_back({120, [] { CCDirector::get()->replaceScene(MenuLayer::scene(false)); }});
    g_steps.push_back({1,
                       []
                       {
                         prepareScreenshot();
                         g_frames = 0;
                         finish(true);
                       }});
  }

  // The level editor with a new level and 3000 objects: a button of its interface edited, and what the hover
  // costs there
  bool menuSettled();

  // =session: edits kept and put back. The Geode button and a sprite without an ID (in the play button) moved, the
  // main menu made again with putting back on: both back where they were moved, one step to undo, one edit each.
  // The session's file written, read, forgotten and loaded again as after a restart
  void sessionTest()
  {
    struct Kept
    {
      inspect::NodeState geode;
      inspect::NodeState sprite;
      inspect::NodeState geodeBefore;
      inspect::NodeState spriteBefore;
      // Another mod may put the Geode button back every frame (a custom main menu's layout guard)
      bool geodeStays = true;
    };
    static Kept kept;
    g_steps.push_back({10,
                       []
                       {
                         // Nothing kept from the self test's earlier scenes
                         while (editor::undo())
                           ;
                         editor::forgetKept();
                         auto scene = CCDirector::get()->getRunningScene();
                         auto geode = scene->getChildByIDRecursive("geode.loader/geode-button");
                         auto play = scene->getChildByIDRecursive("play-button");
                         CCNode* sprite = play ? play->getChildByType<CCSprite>(0) : nullptr;
                         check(geode && sprite, "the Geode button and the play button's sprite are there");
                         if (!geode || !sprite)
                           return;
                         kept.geodeBefore = inspect::stateOf(geode);
                         kept.spriteBefore = inspect::stateOf(sprite);
                         editor::setSnapping(false);
                         for (auto [node, by] : {std::pair{geode, ccp(-14, 9)}, std::pair{sprite, ccp(6, -4)}})
                         {
                           auto center = inspect::worldQuad(node).center();
                           editor::grab(node, {center.x, center.y});
                           editor::dragTo(CCPoint{center.x, center.y} + by);
                           editor::release();
                         }
                         editor::setSnapping(std::nullopt);
                         kept.geode = inspect::stateOf(geode);
                         kept.sprite = inspect::stateOf(sprite);
                         check(editor::editedCount() == 2, fmt::format("two nodes edited: {}", editor::editedCount()));
                       }});
    g_steps.push_back({60,
                       []
                       {
                         auto geode = CCDirector::get()->getRunningScene()->getChildByIDRecursive("geode.loader/geode-button");
                         kept.geodeStays = geode && inspect::stateOf(geode).position == kept.geode.position;
                         if (!kept.geodeStays)
                           log::info("Self test: another mod puts the Geode button back: its position isn't checked");
                         editor::setPutBack(true);
                         CCDirector::get()->replaceScene(MenuLayer::scene(false));
                       }});
    g_steps.push_back({10,
                       []
                       {
                         auto scene = CCDirector::get()->getRunningScene();
                         auto geode = scene->getChildByIDRecursive("geode.loader/geode-button");
                         auto play = scene->getChildByIDRecursive("play-button");
                         auto sprite = play ? play->getChildByType<CCSprite>(0) : nullptr;
                         check(geode && sprite, "the new main menu has them");
                         if (!geode || !sprite)
                           return;
                         check(!kept.geodeStays || inspect::stateOf(geode).position == kept.geode.position,
                               "the Geode button of the main menu made again is where it was moved");
                         check(inspect::stateOf(sprite).position == kept.sprite.position,
                               "the sprite without an ID is put back too, found by its path");
                         check(editor::editedCount() == 2,
                               fmt::format("each is one edit, not one kept and one put back: {}", editor::editedCount()));
                         auto all = editor::codeForAll();
                         size_t times = 0;
                         std::string lookup = "getChildByIDRecursive(\"geode.loader/geode-button\")";
                         for (auto at = all.find(lookup); at != std::string::npos; at = all.find(lookup, at + 1))
                           times++;
                         check(times == 1, fmt::format("the Geode button is in the code for all once: {}", times));

                         // Written and read back
                         editor::saveSessionNow();
                         auto text = file::readString(std::filesystem::path(editor::sessionFile()));
                         auto groups = text ? session::read(*text) : std::nullopt;
                         size_t nodes = 0;
                         if (groups)
                           for (auto const& [layer, descs] : *groups)
                             nodes += descs.size();
                         check(nodes == 2, fmt::format("the session's file has the two nodes: {}", nodes));

                         // As after a restart: forgotten, the file put back, loaded
                         editor::undo();
                         check((!kept.geodeStays || inspect::stateOf(geode).position == kept.geodeBefore.position) &&
                                   inspect::stateOf(sprite).position == kept.spriteBefore.position,
                               "one Ctrl+Z took both put back edits off");
                         check(editor::editedCount() == 2,
                               fmt::format("undoing the put back keeps the edits kept: {}", editor::editedCount()));
                         // Another layer's description with the same ID finds nothing here
                         auto other = inspect::describe(geode);
                         other.layer = "CreatorLayer";
                         check(inspect::resolve(scene, other) == nullptr,
                               "a kept edit of another layer isn't put on a node with its ID here");
                         editor::forgetKept();
                         check(editor::editedCount() == 0, "Ctrl+Delete forgot the kept edits");
                         if (text)
                           (void)file::writeString(std::filesystem::path(editor::sessionFile()), *text);
                         editor::loadSession();
                         check(editor::editedCount() == 2,
                               fmt::format("the session loaded again has the two edits: {}", editor::editedCount()));
                         editor::forgetKept();
                         editor::setPutBack(std::nullopt);
                         g_frames = 0;
                         finish(false);
                       },
                       menuSettled});
  }

  bool menuSettled()
  {
    auto scene = CCDirector::get()->getRunningScene();
    return sceneSettled() && scene->getChildByType<MenuLayer>(0);
  }

  // The game reloaded as a change of screen mode, resolution or texture quality does it (GameManager::reloadAll:
  // every texture unloaded, the loading screen, a new main menu) while the edit mode is on with a selection
  void reloadGame()
  {
    g_steps.push_back({30,
                       []
                       {
                         auto scene = CCDirector::get()->getRunningScene();
                         if (auto node = scene->getChildByIDRecursive("geode.loader/geode-button"))
                           editor::select(node);
                         log::info("Self test: reloading the game with the edit mode on");
                         GameManager::get()->reloadAll(false, false, false);
                       }});
    g_steps.push_back({60,
                       []
                       {
                         log::info("Self test: the main menu is back after the reload; the edit mode is {}",
                                   editor::active() ? "on" : "off");
                         // Usable again with F8, as a user would turn it on
                         if (!editor::active())
                           editor::setActive(true);
                         check(editor::active(), "the edit mode can be on after the reload");
                         check(editor::selected() == nullptr, "nothing of the old scene is selected after the reload");
                         auto scene = CCDirector::get()->getRunningScene();
                         auto button = scene->getChildByIDRecursive("geode.loader/geode-button");
                         check(button != nullptr, "the Geode button is in the new main menu");
                         if (!button)
                           return;
                         Case item{Retained<CCNode>(button), {}, {}};
                         editor::setSnapping(false);
                         edit(item);
                         editor::setSnapping(std::nullopt);
                         while (editor::undo())
                           ;
                         check(inspect::stateOf(button) == item.before, "undo works after the reload");
                         // Every kind of label drawn after the textures were unloaded: the tree's rows, the code
                         editor::setTreeVisible(true);
                         editor::setCodeView(true);
                         prepareScreenshot();
                         g_frames = 0;
                         finish(true);
                       },
                       menuSettled});
  }

  void openEditor()
  {
    g_steps.push_back({180,
                       []
                       {
                         CCDirector::get()->replaceScene(LevelEditorLayer::scene(GJGameLevel::create(), false));
                       }});
    g_steps.push_back({30,
                       []
                       {
                         auto scene = CCDirector::get()->getRunningScene();
                         auto editor = scene ? scene->getChildByType<LevelEditorLayer>(0) : nullptr;
                         check(editor != nullptr, "the level editor is open");
                         if (!editor)
                           return;
                         for (int i = 0; i < 3000; i++)
                           editor->createObject(1, {static_cast<float>(i % 60) * 30 + 15, static_cast<float>(i / 60) * 30 + 105}, true);
                         auto win = CCDirector::get()->getWinSize();
                         auto start = now();
                         for (int i = 0; i < 10; i++)
                           inspect::hitsAt(scene, {win.width / 2, win.height / 2});
                         auto millis = (now() - start) * 100;
                         log::info("Self test: the hover in the editor with 3000 objects: {:.2f} ms", millis);
                         {
                           // A node of the editor's interface without an ID, described as the panel does every frame
                           CCNode* unnamed = nullptr;
                           std::vector<CCNode*> nodes{editor->m_editorUI};
                           for (size_t i = 0; i < nodes.size() && !unnamed; i++)
                           {
                             if (nodes[i] != editor->m_editorUI && nodes[i]->getID().empty() &&
                                 typeinfo_cast<CCSprite*>(nodes[i]))
                               unnamed = nodes[i];
                             for (auto child : CCArrayExt<CCNode*>(nodes[i]->getChildren()))
                               nodes.push_back(child);
                           }
                           if (unnamed)
                           {
                             inspect::forgetLookups();
                             auto first = now();
                             inspect::describe(unnamed);
                             auto firstMillis = (now() - first) * 1000;
                             auto again = now();
                             for (int i = 0; i < 10; i++)
                               inspect::describe(unnamed);
                             auto againMillis = (now() - again) * 100;
                             log::info("Self test: describing {} in the editor: {:.3f} ms, then {:.3f} ms a frame",
                                       inspect::name(unnamed), firstMillis, againMillis);
                             check(againMillis < 0.5, fmt::format("describing a node every frame in the editor takes {:.3f} ms",
                                                                  againMillis));
                           }
                         }
                         check(millis < 10, fmt::format("the hover in the editor takes {:.2f} ms", millis));

                         // A button of the editor's interface, with an ID, the mouse can get to
                         CCNode* button = nullptr;
                         std::vector<CCNode*> queue{editor->m_editorUI};
                         for (size_t i = 0; i < queue.size() && !button; i++)
                         {
                           auto node = queue[i];
                           if (typeinfo_cast<CCMenuItemSpriteExtra*>(node) && !node->getID().empty() &&
                               visibleInTree(node) && reachable(node))
                             button = node;
                           for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
                             queue.push_back(child);
                         }
                         check(button != nullptr, "a button of the editor's interface is there");
                         if (!button)
                           return;
                         Case item{Retained<CCNode>(button), {}, {}};
                         edit(item);
                         while (editor::undo())
                         {
                         }
                         check(inspect::stateOf(button) == item.before, "undo put the editor's button back");
                         editor::select(button);
                         editor::pinMouse(std::nullopt);
                         editor::setMeasuring(std::nullopt);
                       }});
    g_steps.push_back({1,
                       []
                       {
                         g_frames = 0;
                         finish(false);
                       }});
  }

  // A node removed while it's selected, hovered, in the history and in the tree; then the scene replaced while
  // a node is selected, an edit in the new scene, and back to the main menu
  void edgeCases()
  {
    static Retained<CCNode> temp;
    static CCPoint middle;
    static Retained<CCNode> crowd;
    static double frameStart = 0;
    static std::vector<double> frameTimes;
    static Retained<TestPopup> popup;
    static Retained<CCNode> button;
    static Retained<LayoutOptions> options;
    static inspect::NodeState buttonBefore;
    g_steps = {
        // A popup: its button dragged, the generated AnchorLayoutOptions put on it, the layout updated: it stays
        // where it was dragged to
        {240,
         []
         {
           auto made = TestPopup::create();
           check(made != nullptr, "the test popup is made");
           if (!made)
             return;
           made->show();
           popup = Retained<TestPopup>(made);
         }},
        {2,
         []
         {
           if (!popup)
             return;
           auto node = popup->getChildByIDRecursive("selftest-button"_spr);
           check(node != nullptr, "the popup's button is there");
           if (!node)
           {
             popup->close();
             popup.reset();
             return;
           }
           button = Retained<CCNode>(node);
           options = Retained<LayoutOptions>(node->getLayoutOptions());
           buttonBefore = inspect::stateOf(node);
           auto layout = inspect::layoutOf(node);
           check(layout.kind == codegen::LayoutKind::Anchor && layout.hasAnchorOptions,
                 fmt::format("the popup's button has AnchorLayoutOptions in an {}", layout.name));
           auto center = inspect::worldQuad(node).center();
           CCPoint from{center.x, center.y};
           editor::setSnapping(false);
           editor::grab(node, from);
           editor::dragTo(from + ccp(-30, 15));
           editor::release();
           editor::setSnapping(std::nullopt);
           auto dragged = node->getPosition();
           auto code = editor::codeFor(node);
           log::info("Self test: the code for the popup's button:\n{}", code);
           check(code.find("AnchorLayoutOptions") != std::string::npos, "the popup's button's code is AnchorLayoutOptions");
           auto problem = inspect::checkCode(node, code);
           check(problem.empty(), fmt::format("its anchor and offset give where it is: {}", problem));
           // As the code would do it: the options set, the menu's layout updated
           check(applyAnchorCode(node, code), "the anchor and the offset can be put on it");
           node->getParent()->updateLayout();
           auto laidOut = node->getPosition();
           check(close(laidOut.x, dragged.x) && close(laidOut.y, dragged.y),
                 fmt::format("after updateLayout() it stays where it was dragged: ({}, {}) for ({}, {})", laidOut.x,
                             laidOut.y, dragged.x, dragged.y));
           // Back as it was: its own options, the drag undone
           node->setLayoutOptions(options.get());
           check(editor::undo(), "the drag in the popup is a step to undo");
           check(inspect::stateOf(node) == buttonBefore, "undo put the popup's button back");
           popup->close();
           popup.reset();
           button.reset();
           options.reset();
         }},
        // A crowded scene, like a level in the editor: 5000 sprites. What the hover and the tree cost then
        {1,
         []
         {
           auto win = CCDirector::get()->getWinSize();
           auto node = CCNode::create();
           node->setID("selftest-crowd"_spr);
           for (int i = 0; i < CROWD; i++)
           {
             auto sprite = CCSprite::createWithSpriteFrameName("square_01_001.png");
             if (!sprite)
               sprite = CCSprite::create();
             sprite->setPosition({static_cast<float>(i % 100) * win.width / 100, static_cast<float>(i / 100) * 5});
             sprite->setScale(0.2f);
             node->addChild(sprite);
           }
           CCDirector::get()->getRunningScene()->addChild(node, -1);
           crowd = Retained<CCNode>(node);

           auto start = now();
           size_t hits = 0;
           for (int i = 0; i < 10; i++)
             hits += inspect::hitsAt(CCDirector::get()->getRunningScene(), {win.width / 2, 20}).size();
           auto millis = (now() - start) * 100;
           log::info("Self test: what's under the mouse among {} more nodes: {:.2f} ms ({} hits)", CROWD, millis, hits / 10);
           check(millis < 30, fmt::format("the hover in a crowded scene takes {:.2f} ms", millis));
           editor::setTreeVisible(true);
           frameTimes.clear();
           frameStart = now();
         }},
        {30,
         []
         {
           // The frames with the tree open over the crowd, the edit mode drawing as it does
           auto millis = (now() - frameStart) * 1000 / 30;
           log::info("Self test: a frame with the tree open over {} more nodes: {:.2f} ms on average", CROWD, millis);
           check(millis < 50, fmt::format("a frame with the tree open in a crowded scene takes {:.2f} ms", millis));
           editor::setTreeVisible(false);
           crowd->removeFromParent();
           crowd.reset();
         }},
        {3,
         []
         {
           auto win = CCDirector::get()->getWinSize();
           middle = ccp(win.width / 2, win.height / 2);
           CCNode* node = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
           if (!node)
             node = CCLayerColor::create({255, 255, 255, 255}, 20, 20);
           node->setID("selftest-node"_spr);
           node->setPosition(middle);
           CCDirector::get()->getRunningScene()->addChild(node, 1000);
           temp = Retained<CCNode>(node);
           editor::setTreeVisible(true);
           editor::grab(node, middle);
           editor::dragTo(middle + ccp(15, 10));
           editor::release();
           editor::wheel(1, false);
           auto center = inspect::worldQuad(node).center();
           editor::pinMouse(CCPoint{center.x, center.y});
           check(editor::selected() == node, "the new node is selected");
         }},
        {3,
         []
         {
           // Gone from the scene while selected, hovered, in the history and in the tree's rows
           temp->removeFromParent();
           temp.reset();
         }},
        {1,
         []
         {
           check(editor::selected() == nullptr, "a removed node is unselected");
           check(!editor::undo(), "undo skips a removed node's steps");
           check(editor::editedNodes().empty(), "a removed node isn't among the edited ones");
           editor::setTreeVisible(false);
           editor::pinMouse(middle);
           log::info("Self test: a node removed while selected: no crash");
           auto button = CCDirector::get()->getRunningScene()->getChildByIDRecursive("geode.loader/geode-button");
           // An edit to be kept when the scene goes
           if (button)
           {
             auto center = inspect::worldQuad(button).center();
             editor::setSnapping(false);
             editor::grab(button, {center.x, center.y});
             editor::dragTo(CCPoint{center.x, center.y} + ccp(-6, 4));
             editor::release();
             editor::setSnapping(std::nullopt);
           }
           check(editor::editedCount() == 1, "the Geode button is edited before the scene changes");
           editor::select(button);
           auto scene = CreatorLayer::scene();
           CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, scene));
         }},
        {5,
         []
         {
           check(editor::active(), "the edit mode is on during the transition");
           check(editor::selected() == nullptr, "the old scene's node is unselected in the transition");
         }},
        {60,
         []
         {
           auto scene = CCDirector::get()->getRunningScene();
           check(scene && scene->getChildByType<CreatorLayer>(0), "the creator layer is there");
           auto picked = editor::pressAt(middle);
           if (picked)
           {
             editor::dragTo(middle + ccp(20, 0));
             editor::release();
           }
           check(picked != nullptr, "something is picked in the new scene");
           check(editor::historySize() >= 1, "an edit in the new scene is in the history");
           log::info("Self test: picked {} in the creator layer", picked ? inspect::name(picked) : "nothing");
           CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, MenuLayer::scene(false)));
         },
         sceneSettled},
        {120,
         []
         {
           check(editor::active(), "the edit mode is on after two scene changes");
           check(editor::historySize() == 0, "the history forgot the creator layer's nodes");
           // The edits of both scenes left are kept for "Copy all", by layer
           auto all = editor::codeForAll();
           log::info("Self test: the code for all, after two scene changes:\n{}", all);
           check(all.find("// ! --- In MenuLayer --- !") != std::string::npos &&
                     all.find("geode.loader/geode-button") != std::string::npos,
                 "the main menu's edit is kept after the scene changed");
           check(all.find("// ! --- In CreatorLayer --- !") != std::string::npos,
                 "the creator layer's edit is kept after the scene changed");
           // Each section is the hook its code goes in, ready to paste
           check(all.find("class $modify(MenuLayer)") != std::string::npos &&
                     all.find("if (!MenuLayer::init())") != std::string::npos &&
                     all.find("class $modify(CreatorLayer)") != std::string::npos,
                 "the code for all is MenuLayer's and CreatorLayer's hooks");
           check(editor::editedCount() == 2, fmt::format("{} edits kept from the scenes left", editor::editedCount()));
           // Back in the main menu, the Geode button edited again: one section for the main menu, its newer code
           if (auto again = CCDirector::get()->getRunningScene()->getChildByIDRecursive("geode.loader/geode-button"))
           {
             auto center = inspect::worldQuad(again).center();
             editor::setSnapping(false);
             editor::grab(again, {center.x, center.y});
             editor::dragTo(CCPoint{center.x, center.y} + ccp(-9, 3));
             editor::release();
             editor::setSnapping(std::nullopt);
             auto merged = editor::codeForAll();
             size_t sections = 0;
             for (auto at = merged.find("// ! --- In MenuLayer --- !"); at != std::string::npos;
                  at = merged.find("// ! --- In MenuLayer --- !", at + 1))
               sections++;
             check(sections == 1, fmt::format("the main menu is one section after coming back ({})", sections));
             check(editor::editedCount() == 2, fmt::format("the button edited again is one edit ({})", editor::editedCount()));
             editor::undo();
           }
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "play")
           {
             playLevel();
             return;
           }
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "editor")
           {
             openEditor();
             return;
           }
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "tour")
           {
             tour();
             return;
           }
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "reload")
           {
             reloadGame();
             return;
           }
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "session")
           {
             sessionTest();
             return;
           }
           // =ime: the edit mode off and a text field with the keyboard, for tools/real-input.sh's F8 in it
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "ime")
           {
             editor::setActive(false);
             auto win = CCDirector::get()->getWinSize();
             auto input = TextInput::create(160, "UI Tweaker");
             input->setID("selftest-input"_spr);
             input->setPosition({win.width / 2, win.height / 2});
             CCDirector::get()->getRunningScene()->addChild(input, 1000);
             input->focus();
             check(CCIMEDispatcher::sharedDispatcher()->hasDelegate(), "a text field has the keyboard");
             g_frames = 0;
             finish(false);
             return;
           }
           prepareScreenshot();
           g_frames = 0;
           finish(true);
         },
         sceneSettled},
    };
    g_next = 0;
    g_wait = 0;
    tick();
  }

  // The end: with `measure`, the distances to a node pointed at once the panel has moved out of the way
  void finish(bool measure)
  {
    static bool measurable = false;
    ++g_frames;
    if (g_frames == 3)
      measurable = measure && pointAtMeasurable();
    if (g_frames < 8)
    {
      Loader::get()->queueInMainThread([measure] { finish(measure); });
      return;
    }
    if (measurable)
    {
      auto lengths = editor::measurements();
      std::string text;
      for (auto length : lengths)
        text += fmt::format("{} ", length);
      log::info("Self test: distances from the selected node to the hovered one: {}", text);
      check(!lengths.empty() && std::all_of(lengths.begin(), lengths.end(), [](float l) { return l > 0; }),
            "Alt shows the distances from the selected node to the hovered one");
    }

    if (g_failed == 0)
      log::info("Self test PASS: {} checks", g_checks);
    else
      log::error("Self test FAIL: {} of {} checks failed", g_failed, g_checks);
  }

  void poll()
  {
    auto scene = CCDirector::get()->getRunningScene();
    auto layer = scene ? scene->getChildByType<MenuLayer>(0) : nullptr;
    static double since = 0;
    if (!layer)
      g_frames = 0;
    else if (++g_frames == 1)
      since = now();
    else if (g_frames >= SETTLE_FRAMES && now() - since >= SETTLE_FRAMES / 60.0)
    {
      run(layer);
      return;
    }
    Loader::get()->queueInMainThread(poll);
  }
} // namespace

bool selftest::requested()
{
  auto value = std::getenv("UI_TWEAKER_SELFTEST");
  return value && *value && std::string(value) != "0";
}

void selftest::start()
{
  log::info("Self test: waiting for the main menu");
  Loader::get()->queueInMainThread(poll);
}
