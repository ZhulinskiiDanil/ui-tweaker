#include "Session.hpp"

#include <charconv>
#include <cstdio>
#include <locale>
#include <sstream>

namespace
{
  constexpr std::string_view HEADER = "ui-tweaker session 1";

  // A string on one line: backslashes and line breaks escaped
  std::string escape(std::string const& text)
  {
    std::string out;
    for (char c : text)
    {
      if (c == '\\')
        out += "\\\\";
      else if (c == '\n')
        out += "\\n";
      else if (c == '\r')
        out += "\\r";
      else
        out += c;
    }
    return out;
  }

  std::string unescape(std::string_view text)
  {
    std::string out;
    for (size_t i = 0; i < text.size(); i++)
    {
      if (text[i] == '\\' && i + 1 < text.size())
      {
        char next = text[++i];
        out += next == 'n' ? '\n' : next == 'r' ? '\r' : next;
      }
      else
        out += text[i];
    }
    return out;
  }

  // Numbers with a point whatever locale another mod set: "1.5", never "1,5"
  std::string number(float value)
  {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(9);
    out << value;
    return out.str();
  }

  float toFloat(std::string_view text)
  {
    std::istringstream in{std::string(text)};
    in.imbue(std::locale::classic());
    float value = 0;
    in >> value;
    return value;
  }

  int toInt(std::string_view text)
  {
    int value = 0;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
  }

  // "a b": {"a", "b"}, split at the first space
  std::pair<std::string_view, std::string_view> split(std::string_view line)
  {
    auto space = line.find(' ');
    if (space == std::string_view::npos)
      return {line, {}};
    return {line.substr(0, space), line.substr(space + 1)};
  }

  // Two numbers: "1.5 2"
  std::pair<float, float> pair(std::string_view text)
  {
    auto [a, b] = split(text);
    return {toFloat(a), toFloat(b)};
  }
} // namespace

std::string session::write(Groups const& groups)
{
  std::ostringstream out;
  out << HEADER << "\n";
  for (auto const& [layer, nodes] : groups)
  {
    out << "layer " << escape(layer) << "\n";
    for (auto const& node : nodes)
    {
      out << "node\n";
      out << "id " << escape(node.id) << "\n";
      out << "class " << escape(node.className) << "\n";
      out << "parentId " << escape(node.parentId) << "\n";
      out << "parentClass " << escape(node.parentClass) << "\n";
      out << "hook " << escape(node.layer) << "\n";
      out << "hookIsPopup " << node.layerIsPopup << "\n";
      if (!node.made.kind.empty())
      {
        out << "madeKind " << escape(node.made.kind) << "\n";
        out << "madeFrame " << escape(node.made.frame) << "\n";
        out << "madeText " << escape(node.made.text) << "\n";
        out << "madeFont " << escape(node.made.font) << "\n";
        out << "madeFontSize " << number(node.made.fontSize) << "\n";
        out << "madeLike " << escape(node.made.like) << "\n";
        for (auto const& condition : node.made.parent)
          out << "madeParent " << escape(condition) << "\n";
      }
      out << "typeIndex " << node.typeIndex << "\n";
      out << "byIndex " << node.byIndex << "\n";
      out << "idUnique " << node.idUnique << "\n";
      out << "pathRoot " << escape(node.pathRoot) << "\n";
      out << "pathFromLayer " << node.pathFromLayer << "\n";
      for (auto const& step : node.path)
      {
        // Each step on its lines, after "step"
        out << "step\n";
        out << "stepId " << escape(step.id) << "\n";
        out << "stepClass " << escape(step.className) << "\n";
        out << "stepIndex " << step.typeIndex << "\n";
        out << "stepByIndex " << step.byIndex << "\n";
        out << "stepFrame " << escape(step.frameName) << "\n";
      }
      out << "position " << number(node.position.x) << " " << number(node.position.y) << "\n";
      out << "scale " << number(node.scaleX) << " " << number(node.scaleY) << "\n";
      out << "anchor " << number(node.anchor.x) << " " << number(node.anchor.y) << "\n";
      out << "zOrder " << node.zOrder << "\n";
      out << "size " << number(node.size.width) << " " << number(node.size.height) << "\n";
      out << "hasLayout " << node.hasLayout << "\n";
      out << "frame " << (node.frame.kind == codegen::FrameKind::Screen ? "screen" : "parent") << "\n";
      out << "extent " << number(node.frame.extent.width) << " " << number(node.frame.extent.height) << "\n";
      out << "origin " << number(node.frame.origin.x) << " " << number(node.frame.origin.y) << "\n";
      out << "layoutKind " << static_cast<int>(node.layout.kind) << "\n";
      out << "layoutName " << escape(node.layout.name) << "\n";
      out << "anchorOptions " << node.layout.hasAnchorOptions << "\n";
      if (node.layout.anchorNow)
        out << "anchorNow " << number(node.layout.anchorNow->x) << " " << number(node.layout.anchorNow->y) << "\n";
      if (node.layout.prevGap)
        out << "prevGap " << number(*node.layout.prevGap) << "\n";
      out << "scaleChanged " << node.scaleChanged << "\n";
      out << "anchorChanged " << node.anchorChanged << "\n";
      out << "zOrderChanged " << node.zOrderChanged << "\n";
      out << "sizeChanged " << node.sizeChanged << "\n";
      out << "visible " << node.visible << "\n";
      out << "visibleChanged " << node.visibleChanged << "\n";
      out << "rotation " << number(node.rotation) << "\n";
      out << "rotationChanged " << node.rotationChanged << "\n";
      out << "opacity " << node.opacity << "\n";
      out << "opacityChanged " << node.opacityChanged << "\n";
      out << "end\n";
    }
  }
  return out.str();
}

std::optional<session::Groups> session::read(std::string_view text)
{
  std::vector<std::string_view> lines;
  for (size_t start = 0; start < text.size();)
  {
    auto end = text.find('\n', start);
    if (end == std::string_view::npos)
      end = text.size();
    auto line = text.substr(start, end - start);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    lines.push_back(line);
    start = end + 1;
  }
  if (lines.empty() || lines[0] != HEADER)
    return std::nullopt;

  Groups groups;
  codegen::NodeDesc node;
  bool inNode = false;
  for (size_t i = 1; i < lines.size(); i++)
  {
    auto [key, value] = split(lines[i]);
    if (key == "layer")
    {
      groups.emplace_back(unescape(value), std::vector<codegen::NodeDesc>{});
      continue;
    }
    if (groups.empty())
      continue;
    if (key == "node")
    {
      node = {};
      inNode = true;
      continue;
    }
    if (!inNode)
      continue;
    if (key == "end")
    {
      groups.back().second.push_back(node);
      inNode = false;
    }
    else if (key == "id")
      node.id = unescape(value);
    else if (key == "class")
      node.className = unescape(value);
    else if (key == "parentId")
      node.parentId = unescape(value);
    else if (key == "parentClass")
      node.parentClass = unescape(value);
    else if (key == "hook")
      node.layer = unescape(value);
    else if (key == "hookIsPopup")
      node.layerIsPopup = value == "1";
    else if (key == "madeKind")
      node.made.kind = unescape(value);
    else if (key == "madeFrame")
      node.made.frame = unescape(value);
    else if (key == "madeText")
      node.made.text = unescape(value);
    else if (key == "madeFont")
      node.made.font = unescape(value);
    else if (key == "madeFontSize")
      node.made.fontSize = toFloat(value);
    else if (key == "madeLike")
      node.made.like = unescape(value);
    else if (key == "madeParent")
      node.made.parent.push_back(unescape(value));
    else if (key == "typeIndex")
      node.typeIndex = toInt(value);
    else if (key == "byIndex")
      node.byIndex = value == "1";
    else if (key == "idUnique")
      node.idUnique = value == "1";
    else if (key == "pathRoot")
      node.pathRoot = unescape(value);
    else if (key == "pathFromLayer")
      node.pathFromLayer = value == "1";
    else if (key == "step")
      node.path.emplace_back();
    else if (key == "stepId" && !node.path.empty())
      node.path.back().id = unescape(value);
    else if (key == "stepClass" && !node.path.empty())
      node.path.back().className = unescape(value);
    else if (key == "stepIndex" && !node.path.empty())
      node.path.back().typeIndex = toInt(value);
    else if (key == "stepByIndex" && !node.path.empty())
      node.path.back().byIndex = value == "1";
    else if (key == "stepFrame" && !node.path.empty())
      node.path.back().frameName = unescape(value);
    else if (key == "position")
    {
      auto [x, y] = pair(value);
      node.position = {x, y};
    }
    else if (key == "scale")
      std::tie(node.scaleX, node.scaleY) = pair(value);
    else if (key == "anchor")
    {
      auto [x, y] = pair(value);
      node.anchor = {x, y};
    }
    else if (key == "zOrder")
      node.zOrder = toInt(value);
    else if (key == "size")
    {
      auto [w, h] = pair(value);
      node.size = {w, h};
    }
    else if (key == "hasLayout")
      node.hasLayout = value == "1";
    else if (key == "frame")
      node.frame.kind = value == "screen" ? codegen::FrameKind::Screen : codegen::FrameKind::Parent;
    else if (key == "extent")
    {
      auto [w, h] = pair(value);
      node.frame.extent = {w, h};
    }
    else if (key == "origin")
    {
      auto [x, y] = pair(value);
      node.frame.origin = {x, y};
    }
    else if (key == "layoutKind")
    {
      auto kind = toInt(value);
      node.layout.kind = kind >= 0 && kind <= static_cast<int>(codegen::LayoutKind::Other)
                             ? static_cast<codegen::LayoutKind>(kind)
                             : codegen::LayoutKind::Other;
    }
    else if (key == "layoutName")
      node.layout.name = unescape(value);
    else if (key == "anchorOptions")
      node.layout.hasAnchorOptions = value == "1";
    else if (key == "anchorNow")
    {
      auto [x, y] = pair(value);
      node.layout.anchorNow = geom::Vec2{x, y};
    }
    else if (key == "prevGap")
      node.layout.prevGap = toFloat(value);
    else if (key == "scaleChanged")
      node.scaleChanged = value == "1";
    else if (key == "anchorChanged")
      node.anchorChanged = value == "1";
    else if (key == "zOrderChanged")
      node.zOrderChanged = value == "1";
    else if (key == "sizeChanged")
      node.sizeChanged = value == "1";
    else if (key == "visible")
      node.visible = value == "1";
    else if (key == "visibleChanged")
      node.visibleChanged = value == "1";
    else if (key == "rotation")
      node.rotation = toFloat(value);
    else if (key == "rotationChanged")
      node.rotationChanged = value == "1";
    else if (key == "opacity")
      node.opacity = toInt(value);
    else if (key == "opacityChanged")
      node.opacityChanged = value == "1";
  }
  std::erase_if(groups, [](auto const& group) { return group.second.empty(); });
  return groups;
}

bool session::sameNode(codegen::NodeDesc const& a, codegen::NodeDesc const& b)
{
  if (a.layer != b.layer)
    return false;
  bool byIdA = !a.id.empty() && (a.idUnique || a.path.empty());
  bool byIdB = !b.id.empty() && (b.idUnique || b.path.empty());
  if (byIdA || byIdB)
    return byIdA && byIdB && a.id == b.id && a.parentId == b.parentId;
  if (a.path.empty() || b.path.empty())
    return a.path.empty() && b.path.empty() && a.parentId == b.parentId && a.parentClass == b.parentClass &&
           a.className == b.className && a.typeIndex == b.typeIndex && a.byIndex == b.byIndex;
  return a.pathRoot == b.pathRoot && a.pathFromLayer == b.pathFromLayer && a.path == b.path;
}
