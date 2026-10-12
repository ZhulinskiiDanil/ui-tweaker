#include "Hooks.hpp"

#include <array>

namespace
{
  struct Known
  {
    char const* className;
    char const* signature;
    char const* arguments;
    bool returnsBool;
  };

  // Geode's bindings for GD 2.2081: the function each layer's interface is made in
  constexpr std::array<Known, 17> KNOWN = {{
      {"MenuLayer", "bool init()", "", true},
      {"CreatorLayer", "bool init()", "", true},
      {"GJGarageLayer", "bool init()", "", true},
      {"SecretLayer2", "bool init()", "", true},
      {"LevelSelectLayer", "bool init(int page)", "page", true},
      {"LevelInfoLayer", "bool init(GJGameLevel* level, bool challenge)", "level, challenge", true},
      {"LevelBrowserLayer", "bool init(GJSearchObject* object)", "object", true},
      {"EditLevelLayer", "bool init(GJGameLevel* level)", "level", true},
      {"PlayLayer", "bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)",
       "level, useReplay, dontCreateObjects", true},
      {"LevelEditorLayer", "bool init(GJGameLevel* level, bool noUI)", "level, noUI", true},
      {"EditorUI", "bool init(LevelEditorLayer* editorLayer)", "editorLayer", true},
      {"GJShopLayer", "bool init(ShopType type)", "type", true},
      {"LevelSearchLayer", "bool init(int type)", "type", true},
      {"ProfilePage", "bool init(int accountID, bool ownProfile)", "accountID, ownProfile", true},
      {"PauseLayer", "void customSetup()", "", false},
      {"EditorPauseLayer", "void customSetup()", "", false},
      {"OptionsLayer", "void customSetup()", "", false},
  }};

  std::string indented(std::string const& code, std::string const& indent)
  {
    std::string out;
    size_t start = 0;
    while (start < code.size())
    {
      auto end = code.find('\n', start);
      auto line = code.substr(start, end == std::string::npos ? std::string::npos : end - start);
      out += (line.empty() ? "" : indent + line) + "\n";
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
    return out;
  }
} // namespace

std::optional<hooks::Hook> hooks::find(std::string_view className)
{
  for (auto const& known : KNOWN)
    if (className == known.className)
      return Hook{known.className, known.signature,
                  std::string(known.className) + "::" + (known.returnsBool ? "init(" : "customSetup(") +
                      known.arguments + ")",
                  known.returnsBool};
  return std::nullopt;
}

std::string hooks::wrap(std::string_view className, std::string const& code)
{
  auto hook = find(className);
  if (!hook)
    return "// In a hook of " + std::string(className) + ", after its interface is made (`this` being it)\n" + code;

  std::string out = "#include <Geode/modify/" + hook->className + ".hpp>\n\n";
  out += "class $modify(" + hook->className + ")\n{\n";
  out += "  " + hook->signature + "\n  {\n";
  if (hook->returnsBool)
    out += "    if (!" + hook->original + ")\n      return false;\n\n";
  else
    out += "    " + hook->original + ";\n\n";
  out += indented(code, "    ");
  if (hook->returnsBool)
    out += "\n    return true;\n";
  out += "  }\n};\n";
  return out;
}
