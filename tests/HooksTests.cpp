#include "../src/core/Hooks.hpp"
#include "Check.hpp"

#include <string>

using namespace hooks;

namespace
{
  bool has(std::string const& text, std::string const& part)
  {
    return text.find(part) != std::string::npos;
  }
} // namespace

int main()
{
  auto menu = find("MenuLayer");
  CHECK(menu && menu->signature == "bool init()" && menu->original == "MenuLayer::init()" && menu->returnsBool);
  auto select = find("LevelSelectLayer");
  CHECK(select && select->original == "LevelSelectLayer::init(page)");
  auto pause = find("PauseLayer");
  CHECK(pause && pause->original == "PauseLayer::customSetup()" && !pause->returnsBool);
  CHECK(!find("SomeModsLayer"));
  // ProfilePage makes its interface when a profile is loaded
  auto profile = find("ProfilePage");
  CHECK(profile && profile->signature == "void loadPageFromUserInfo(GJUserScore* score)" &&
        profile->original == "ProfilePage::loadPageFromUserInfo(score)" && profile->name == "ProfilePage::loadPageFromUserInfo");
  CHECK(has(preamble(), "#include <Geode/Geode.hpp>") && has(preamble(), "using namespace geode::prelude;"));

  auto code = wrap("MenuLayer", "auto winSize = CCDirector::get()->getWinSize();\n\nif (x)\n{\n  y();\n}\n");
  CHECK(has(code, "#include <Geode/modify/MenuLayer.hpp>"));
  // After node-ids, whose IDs the code looks for
  CHECK(has(code, "class $modify(MenuLayer)\n{\n  static void onModify(auto& self)\n  {\n"
                  "    (void)self.setHookPriorityAfter(\"MenuLayer::init\", \"geode.node-ids\");\n  }\n"));
  CHECK(has(code, "  bool init()\n  {\n    if (!MenuLayer::init())\n      return false;\n"));
  // The code indented into the function, empty lines left empty
  CHECK(has(code, "\n    auto winSize = CCDirector::get()->getWinSize();\n\n    if (x)\n    {\n      y();\n    }\n"));
  CHECK(has(code, "    return true;\n  }\n};\n"));

  code = wrap("PauseLayer", "a();\n");
  CHECK(has(code, "  void customSetup()\n  {\n    PauseLayer::customSetup();\n\n    a();\n  }\n};"));
  CHECK(!has(code, "return"));

  code = wrap("SomeModsLayer", "a();\n");
  CHECK(has(code, "// In a hook of SomeModsLayer, after its interface is made"));
  CHECK(has(code, "\na();\n"));
  // A mod's own popup: its setup()
  CHECK(hooks::wrap("MyPopup", "x();\n", true).find("// In MyPopup's setup(), after its nodes are made") == 0);
  CHECK(hooks::wrap("MyPopup", "x();\n").find("// In a hook of MyPopup") == 0);
  return finish("hooks");
}
