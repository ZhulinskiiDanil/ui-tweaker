#include "Codegen.hpp"

#include "Text.hpp"

#include <sstream>

namespace
{
  bool isAxis(codegen::LayoutKind kind)
  {
    return kind == codegen::LayoutKind::Axis || kind == codegen::LayoutKind::SimpleAxis;
  }

  // Positioned by an AnchorLayout's options instead of setPosition
  bool anchored(codegen::NodeDesc const& node)
  {
    return node.layout.kind == codegen::LayoutKind::Anchor && node.layout.hasAnchorOptions &&
           node.frame.kind == codegen::FrameKind::Parent;
  }

  // A C++ string literal of the text
  std::string quoted(std::string const& text)
  {
    std::string out = "\"";
    for (char c : text)
    {
      if (c == '"' || c == '\\')
        out += '\\';
      out += c;
    }
    return out + "\"";
  }

  std::string describe(std::string const& id, std::string const& className)
  {
    if (id.empty())
      return className + " (no ID)";
    return id + " (" + className + ")";
  }

  std::string scaleLines(codegen::NodeDesc const& node, std::string const& indent)
  {
    // A button's press animation scales it back to its base scale
    if (node.className == "CCMenuItemSpriteExtra" && node.scaleX == node.scaleY)
      return indent + "node->setScale(" + expr::number(node.scaleX) + ");\n" + indent +
             "static_cast<CCMenuItemSpriteExtra*>(node)->m_baseScale = " + expr::number(node.scaleX) + ";\n";
    if (node.scaleX == node.scaleY)
      return indent + "node->setScale(" + expr::number(node.scaleX) + ");\n";
    return indent + "node->setScaleX(" + expr::number(node.scaleX) + ");\n" + indent + "node->setScaleY(" +
           expr::number(node.scaleY) + ");\n";
  }

  struct Body
  {
    std::string lines;
    bool usesWinSize = false;
  };

  // The lines inside the block, without the winSize declaration
  Body body(codegen::NodeDesc const& node, std::string const& indent)
  {
    Body result;
    std::ostringstream out;
    auto position = codegen::positionExpressions(node);
    bool screen = node.frame.kind == codegen::FrameKind::Screen;
    std::string width = screen ? "winSize.width" : "parentSize.width";
    std::string height = screen ? "winSize.height" : "parentSize.height";
    auto x = expr::format(position.x, width);
    auto y = expr::format(position.y, height);
    bool usesExtent = position.x.k != 0 || position.y.k != 0;
    bool scaleWanted = node.scaleChanged || node.scaleX != 1 || node.scaleY != 1;

    if (anchored(node))
    {
      // AnchorLayout's anchors are the same 3x3 as the expressions'
      out << indent << "node->setLayoutOptions(AnchorLayoutOptions::create()\n"
          << indent << "  ->setAnchor(Anchor::" << codegen::anchorName(position.x.k, position.y.k) << ")\n"
          << indent << "  ->setOffset({" << expr::number(position.x.offset) << ", "
          << expr::number(position.y.offset) << "}));\n";
      if (scaleWanted)
        out << scaleLines(node, indent);
    }
    else if (isAxis(node.layout.kind))
    {
      // The layout decides the position; what it can be told is the gap and whether to scale the node. The
      // scale first: setLayoutOptions updates the layout right away
      if (scaleWanted)
        out << scaleLines(node, indent);
      if (node.layout.kind == codegen::LayoutKind::Axis)
      {
        std::string options;
        if (node.layout.prevGap)
          options += "->setPrevGap(" + expr::number(*node.layout.prevGap) + ")";
        if (node.scaleChanged)
          options += "->setAutoScale(false)";
        // The node's own options kept (a line break, scale limits), only these set on them
        if (!options.empty())
          out << indent << "auto options = typeinfo_cast<AxisLayoutOptions*>(node->getLayoutOptions());\n"
              << indent << "node->setLayoutOptions((options ? options : AxisLayoutOptions::create())" << options
              << ");\n";
      }
      else if (node.layout.prevGap)
      {
        // A SimpleAxisLayout has no gap per node: an AxisGap node before it is one
        out << indent << "// A gap before it, on top of the layout's own: node->getParent()->insertBefore(AxisGap::create("
            << expr::number(*node.layout.prevGap) << "), node);\n";
      }
      // The position itself, for a node taken out of the layout
      out << indent << "// Without the layout: node->setPosition({" << x << ", " << y << "});\n";
      // The anchor and the z-order below still count: the z-order is the order in the layout
    }
    else
    {
      if (usesExtent && !screen)
        out << indent << "auto parentSize = node->getParent()->getContentSize();\n";
      out << indent << "node->setPosition({" << x << ", " << y << "});\n";
      if (scaleWanted)
        out << scaleLines(node, indent);
      result.usesWinSize = usesExtent && screen;
    }

    if (node.anchorChanged)
      out << indent << "node->setAnchorPoint({" << expr::number(node.anchor.x) << ", " << expr::number(node.anchor.y)
          << "});\n";
    if (node.zOrderChanged)
      out << indent << "node->setZOrder(" << node.zOrder << ");\n";
    result.lines = out.str();
    return result;
  }

  std::string block(codegen::NodeDesc const& node, bool declareWinSize)
  {
    std::ostringstream out;
    out << "// " << describe(node.id, node.className) << " in " << describe(node.parentId, node.parentClass) << "\n";
    auto warning = codegen::layoutWarning(node);
    if (!warning.empty())
      out << "// " << warning << "\n";

    auto find = codegen::lookup(node);
    if (find.empty())
    {
      // Nothing to find it by: the lines are for wherever the node is made
      // In a block of its own, so two of them in one place don't declare parentSize twice
      out << "// No ID to find it by: these go where you make it, with `node` being it\n{\n";
      auto inner = body(node, "  ");
      if (inner.usesWinSize && declareWinSize)
        out << "  auto winSize = CCDirector::get()->getWinSize();\n";
      out << inner.lines << "}\n";
      return out.str();
    }

    auto inner = body(node, "  ");
    out << "if (" << find << ")\n{\n";
    if (inner.usesWinSize && declareWinSize)
      out << "  auto winSize = CCDirector::get()->getWinSize();\n";
    out << inner.lines << "}\n";
    return out.str();
  }
} // namespace

codegen::Position codegen::positionExpressions(NodeDesc const& node)
{
  if (node.frame.kind == FrameKind::Parent)
    return {expr::nearest(node.position.x, node.frame.extent.width),
            expr::nearest(node.position.y, node.frame.extent.height)};

  // The edge or center of the screen closest to where the node is, minus where its parent is
  auto const& origin = node.frame.origin;
  auto const& extent = node.frame.extent;
  return {
      expr::relativeTo(expr::nearest(origin.x + node.position.x, extent.width), expr::nearest(origin.x, extent.width)),
      expr::relativeTo(expr::nearest(origin.y + node.position.y, extent.height),
                       expr::nearest(origin.y, extent.height)),
  };
}

std::string codegen::anchorName(float kx, float ky)
{
  std::string vertical = ky >= 1 ? "Top" : ky <= 0 ? "Bottom" : "";
  std::string horizontal = kx >= 1 ? "Right" : kx <= 0 ? "Left" : "";
  if (vertical.empty() && horizontal.empty())
    return "Center";
  return vertical + horizontal;
}

std::string codegen::layoutWarning(NodeDesc const& node, words::Lang lang)
{
  auto const& w = words::get(lang);
  auto parent = node.parentId.empty() ? node.parentClass : node.parentId;
  switch (node.layout.kind)
  {
  case LayoutKind::None:
    return "";
  case LayoutKind::Anchor:
    return anchored(node) ? text::format(w.anchorLayout, {parent}) : "";
  case LayoutKind::Axis:
    return text::format(w.axisLayout, {parent, node.layout.name});
  case LayoutKind::SimpleAxis:
    return text::format(w.simpleAxisLayout, {parent, node.layout.name});
  case LayoutKind::Other:
    return text::format(w.otherLayout, {parent, node.layout.name});
  }
  return "";
}

std::string codegen::lookup(NodeDesc const& node)
{
  if (!node.id.empty())
    return "auto node = this->getChildByIDRecursive(" + quoted(node.id) + ")";
  if (node.parentId.empty())
    return "";
  auto parent = "auto parent = this->getChildByIDRecursive(" + quoted(node.parentId) + "); auto node = parent ? ";
  // A class the code can't name (another mod's, in a namespace): which child it is of all
  if (node.byIndex)
    return parent + "parent->getChildByIndex(" + std::to_string(node.typeIndex) + ") : nullptr";
  return parent + "parent->getChildByType<" + node.className + ">(" + std::to_string(node.typeIndex) + ") : nullptr";
}

std::string codegen::generate(NodeDesc const& node)
{
  return block(node, true);
}

std::string codegen::generateAll(std::vector<NodeDesc> const& nodes)
{
  std::ostringstream out;
  bool winSize = false;
  for (auto const& node : nodes)
    winSize = winSize || body(node, "").usesWinSize;
  if (winSize)
    out << "auto winSize = CCDirector::get()->getWinSize();\n\n";
  for (size_t i = 0; i < nodes.size(); i++)
  {
    if (i > 0)
      out << "\n";
    out << block(nodes[i], false);
  }
  return out.str();
}
