// "Copy all" from an edit in the game, pasted as it is: its hooks put the nodes where they were dragged. A main menu
// later, each node's place on the screen and scale is compared with where the edit left it, and the result logged
#include E2E_CODE

#include <Geode/modify/MenuLayer.hpp>

#include <cmath>
#include <fstream>
#include <sstream>

namespace
{
  bool g_checked = false;

  void check(MenuLayer* layer)
  {
    std::ifstream in(E2E_EXPECTED);
    int checked = 0, failed = 0;
    for (std::string line; std::getline(in, line);)
    {
      std::istringstream fields(line);
      std::string id;
      float x = 0, y = 0, scale = 0;
      if (!(fields >> id >> x >> y >> scale))
        continue;
      // A node made in the edit mode has this mod's prefix here, as "new-N"_spr gives it
      std::string const made = "zhulis.ui-tweaker/";
      if (id.starts_with(made + "new-"))
        id = std::string(Mod::get()->getID()) + "/" + id.substr(made.size());
      checked++;
      auto node = layer->getChildByIDRecursive(id);
      if (!node || !node->getParent())
      {
        failed++;
        log::error("E2E: {} isn't there", id);
        continue;
      }
      auto world = node->getParent()->convertToWorldSpace(node->getPosition());
      if (std::fabs(world.x - x) > 0.5f || std::fabs(world.y - y) > 0.5f || std::fabs(node->getScaleX() - scale) > 0.001f)
      {
        failed++;
        log::error("E2E: {} is at ({}, {}) scale {}, the edit left it at ({}, {}) scale {}", id, world.x, world.y,
                   node->getScaleX(), x, y, scale);
      }
      else
        log::info("E2E: {} is where the edit left it", id);
    }
    if (checked == 0)
      log::error("E2E FAIL: nothing to check in {}", E2E_EXPECTED);
    else if (failed == 0)
      log::info("E2E PASS: {} nodes where the edit left them", checked);
    else
      log::error("E2E FAIL: {} of {} nodes", failed, checked);
  }
} // namespace

class $modify(E2ECheck, MenuLayer)
{
  bool init()
  {
    if (!MenuLayer::init())
      return false;
    // Once, a frame later: every hook of init has run then
    if (!g_checked)
    {
      g_checked = true;
      Ref<MenuLayer> self = this;
      Loader::get()->queueInMainThread([self] { check(self); });
    }
    return true;
  }
};
