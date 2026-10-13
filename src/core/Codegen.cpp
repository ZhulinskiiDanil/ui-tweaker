#include "Codegen.hpp"

#include "Text.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace
{
  constexpr size_t COMMENT_COLUMNS = 100;

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
  std::string literal(std::string const& text)
  {
    std::string out = "\"";
    for (char c : text)
    {
      // A label's text can have line breaks: a raw one in a literal wouldn't compile
      if (c == '\n')
        out += "\\n";
      else if (c == '\r')
        out += "\\r";
      else if (c == '\t')
        out += "\\t";
      else if (static_cast<unsigned char>(c) < 0x20)
      {
        char escaped[8];
        std::snprintf(escaped, sizeof(escaped), "\\x%02x\"\"", static_cast<unsigned char>(c));
        out += escaped;
      }
      else
      {
        if (c == '"' || c == '\\')
          out += '\\';
        out += c;
      }
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
    // A scale an axis layout gave the node is the layout's: written only if it was changed
    bool scaleWanted = node.scaleChanged || ((node.scaleX != 1 || node.scaleY != 1) && !isAxis(node.layout.kind));

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
        // A SimpleAxisLayout has no gap per node: an AxisGap node before it is one, instead of the layout's gap
        // there. The whole line is laid out again with it (a centered row moves), so it's a suggestion
        out << indent << "// The gap before it instead of the layout's, the line laid out again around it:\n"
            << indent << "// node->getParent()->insertBefore(AxisGap::create(" << expr::number(*node.layout.prevGap)
            << "), node); node->getParent()->updateLayout();\n";
      }
      // The position itself, for a node taken out of the layout
      if (usesExtent && !screen)
        out << indent << "// Without the layout: auto parentSize = node->getParent()->getContentSize();\n"
            << indent << "// node->setPosition({" << x << ", " << y << "});\n";
      else
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
    if (node.visibleChanged)
      out << indent << "node->setVisible(" << (node.visible ? "true" : "false") << ");\n";
    if (node.rotationChanged)
      out << indent << "node->setRotation(" << expr::number(node.rotation) << ");\n";
    // Labels have an opacity too without being CCNodeRGBA
    if (node.opacityChanged)
      out << indent << "if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node))\n"
          << indent << "  rgba->setOpacity(" << node.opacity << ");\n";
    if (node.sizeChanged)
    {
      out << indent << "node->setContentSize({" << expr::number(node.size.width) << ", " << expr::number(node.size.height)
          << "});\n";
      if (node.hasLayout)
        out << indent << "node->updateLayout();\n";
    }
    result.lines = out.str();
    return result;
  }

  // The expression that makes a button's image; empty for a node that isn't a button
  std::string imageMaking(codegen::NodeDesc::Made const& made)
  {
    if (made.kind == "button")
      return "CCSprite::createWithSpriteFrameName(" + literal(made.frame) + ")";
    if (made.kind == "text button" && (made.texture.empty() || made.font.empty()))
      return "ButtonSprite::create(" + literal(made.text) + ")";
    if (made.kind == "text button")
      return "ButtonSprite::create(" + literal(made.text) + ", " + std::to_string(made.width) + ", " +
             std::to_string(made.minWidth) + ", " + expr::number(made.scale) + ", " + (made.absolute ? "true" : "false") +
             ", " + literal(made.font) + ", " + literal(made.texture) + ", " + expr::number(made.height) + ")";
    if (made.kind == "circle button")
      return "CircleButtonSprite::createWithSpriteFrameName(" + literal(made.frame) + ", 1.f, CircleBaseColor::" +
             made.baseColor + ", CircleBaseSize::" + made.baseSize + ")";
    return "";
  }

  // An image scaled by itself is made first, then the button around it
  bool imageScaled(codegen::NodeDesc::Made const& made)
  {
    return !imageMaking(made).empty() && std::fabs(made.imageScale - 1) > 0.001f;
  }

  // The expression that makes a node like the one described
  std::string making(codegen::NodeDesc::Made const& made)
  {
    if (made.kind == "sprite")
      return "CCSprite::createWithSpriteFrameName(" + literal(made.frame) + ")";
    if (auto image = imageMaking(made); !image.empty())
      return "CCMenuItemSpriteExtra::create(" + (imageScaled(made) ? std::string("image") : image) + ", this, nullptr)";
    if (made.kind == "label")
      return "CCLabelBMFont::create(" + literal(made.text) + ", " + literal(made.font) + ")";
    if (made.kind == "ttf")
      return "CCLabelTTF::create(" + literal(made.text) + ", " + literal(made.font) + ", " + expr::number(made.fontSize) + ")";
    return "";
  }

  // A node made in the edit mode: made, given an ID, added to its parent, then placed as any edited node is
  std::string madeBlock(codegen::NodeDesc const& node, bool declareWinSize)
  {
    std::ostringstream out;
    auto const& made = node.made;
    auto id = node.id.substr(node.id.find('/') == std::string::npos ? 0 : node.id.find('/') + 1);
    // Where it comes from, wrapped as the other comments are
    bool button = made.kind == "button" || made.kind == "text button" || made.kind == "circle button";
    std::string about = "A new " + node.className + " like " + made.like + ", in " +
                        describe(node.parentId, node.parentClass) + ": give it an ID of your own";
    if (button)
      about += ", and its callback instead of nullptr: menu_selector(YourLayer::onYourButton)";
    for (auto const& line : text::wrap(about, COMMENT_COLUMNS))
      out << "// " << line << "\n";
    // Its parent: found as a node is, or the layer itself
    std::vector<std::string> conditions = made.parent;
    if (conditions.empty())
      conditions.push_back("auto parent = static_cast<CCNode*>(this)");
    std::string indent;
    for (size_t i = 0; i < conditions.size(); i++)
    {
      out << indent << "if (" << conditions[i] << ")\n";
      if (i + 1 < conditions.size())
        indent += "  ";
    }
    auto inside = indent + "  ";
    auto inner = body(node, inside);
    out << indent << "{\n";
    if (inner.usesWinSize && declareWinSize)
      out << inside << "auto winSize = CCDirector::get()->getWinSize();\n";
    if (imageScaled(made))
    {
      out << inside << "auto image = " << imageMaking(made) << ";\n";
      out << inside << "image->setScale(" << expr::number(made.imageScale) << ");\n";
    }
    out << inside << "auto node = " << making(made) << ";\n";
    out << inside << "node->setID(" << literal(id) << "_spr);\n";
    out << inside << "parent->addChild(node, " << node.zOrder << ");\n";
    out << inner.lines;
    if (node.layout.kind != codegen::LayoutKind::None)
      out << inside << "parent->updateLayout();\n";
    out << indent << "}\n";
    return out.str();
  }

  std::string block(codegen::NodeDesc const& node, bool declareWinSize)
  {
    if (!node.made.kind.empty())
      return madeBlock(node, declareWinSize);
    std::ostringstream out;
    out << "// " << describe(node.id, node.className) << " in " << describe(node.parentId, node.parentClass) << "\n";
    // Wrapped as code is: a hundred columns or so
    auto warning = codegen::layoutWarning(node);
    if (!warning.empty())
      for (auto const& line : text::wrap(warning, COMMENT_COLUMNS))
        out << "// " << line << "\n";

    auto conditions = codegen::lookup(node);
    if (conditions.empty())
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

    // Why it's found the way it is, wrapped as the layout's warning is
    std::string why;
    if (!node.id.empty() && !node.idUnique && !node.path.empty())
      why = "Another node in the layer has the ID " + literal(node.id) +
            " and getChildByIDRecursive finds that one first: found down from a parent instead";
    else if (!node.id.empty() && !node.idUnique)
      why = "Another node in the layer has the ID " + literal(node.id) +
            " and getChildByIDRecursive finds that one first: give this one an ID of its own";
    else if (conditions.size() > 1)
      why = "No ID on it or its parent: found " +
            (node.pathFromLayer ? std::string("from the layer") : "from " + literal(node.pathRoot)) +
            " down through its parents, each by its frame or by how many of its class come before it, which a node "
            "the game or another mod adds can change";
    if (!why.empty())
      for (auto const& line : text::wrap(why, COMMENT_COLUMNS))
        out << "// " << line << "\n";
    // One `if` a step, nested; the node's lines in the last
    std::string indent;
    for (size_t i = 0; i < conditions.size(); i++)
    {
      out << indent << "if (" << conditions[i] << ")\n";
      if (i + 1 < conditions.size())
        indent += "  ";
    }
    auto inside = indent + "  ";
    auto inner = body(node, inside);
    out << indent << "{\n";
    if (inner.usesWinSize && declareWinSize)
      out << inside << "auto winSize = CCDirector::get()->getWinSize();\n";
    out << inner.lines << indent << "}\n";
    return out.str();
  }
} // namespace

codegen::Position codegen::positionExpressions(NodeDesc const& node)
{
  // An anchor the node has already stays
  if (anchored(node) && node.layout.anchorNow)
  {
    auto k = *node.layout.anchorNow;
    return {{k.x, node.position.x - k.x * node.frame.extent.width},
            {k.y, node.position.y - k.y * node.frame.extent.height}};
  }
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

namespace
{
  // A node made in the edit mode has the user's mod's prefix where the code runs: "new-N"_spr
  std::string idLiteral(std::string const& id)
  {
    std::string_view const prefix = "zhulis.ui-tweaker/new-";
    if (id.starts_with(prefix))
      return literal(id.substr(id.find('/') + 1)) + "_spr";
    return literal(id);
  }
} // namespace

std::vector<std::string> codegen::lookup(NodeDesc const& node, std::string const& name)
{
  // The variable the path starts from, not the one it ends in
  std::string const start = name == "parent" ? "root" : "parent";
  if (!node.id.empty() && (node.idUnique || node.path.empty()))
    return {"auto " + name + " = this->getChildByIDRecursive(" + idLiteral(node.id) + ")"};

  // How a step gets its child from `from`
  auto stepCall = [](std::string const& from, NodeDesc::PathStep const& step)
  {
    if (!step.id.empty())
      return from + "->getChildByID(" + idLiteral(step.id) + ")";
    if (!step.frameName.empty())
      return "geode::cocos::getChildBySpriteFrameName(" + from + ", " + literal(step.frameName) + ")";
    // A class the code can't name (another mod's, in a namespace): which child it is of all
    if (step.byIndex)
      return from + "->getChildByIndex(" + std::to_string(step.typeIndex) + ")";
    return from + "->getChildByType<" + step.className + ">(" + std::to_string(step.typeIndex) + ")";
  };

  auto path = node.path;
  auto root = node.pathRoot;
  if (path.empty())
  {
    if (node.parentId.empty())
      return {};
    path.push_back({{}, node.className, node.typeIndex, node.byIndex, {}});
    root = node.parentId;
  }
  if (!node.pathFromLayer && root.empty())
    return {};

  // The parent with an ID and the node in one `if`, as most nodes without an ID are
  if (!node.pathFromLayer && path.size() == 1)
    return {"auto " + start + " = this->getChildByIDRecursive(" + idLiteral(root) + "); auto " + name + " = " + start +
            " ? " + stepCall(start, path[0]) + " : nullptr"};

  std::vector<std::string> out;
  std::string from = "this";
  if (!node.pathFromLayer)
  {
    out.push_back("auto " + start + " = this->getChildByIDRecursive(" + idLiteral(root) + ")");
    from = start;
  }
  for (size_t i = 0; i < path.size(); i++)
  {
    auto step = i + 1 == path.size() ? name : "child" + std::to_string(i + 1);
    out.push_back("auto " + step + " = " + stepCall(from, path[i]));
    from = step;
  }
  return out;
}

std::string codegen::findCode(NodeDesc const& node)
{
  auto conditions = lookup(node);
  if (conditions.empty())
    return "";
  std::ostringstream out;
  out << "// " << describe(node.id, node.className) << " in " << describe(node.parentId, node.parentClass) << "\n";
  std::string indent;
  for (size_t i = 0; i < conditions.size(); i++)
  {
    out << indent << "if (" << conditions[i] << ")\n";
    if (i + 1 < conditions.size())
      indent += "  ";
  }
  // Typed, so its own functions are there
  out << indent << "{\n";
  if (node.className.find_first_of(":`<> ") == std::string::npos && node.className != "CCNode")
    out << indent << "  auto it = static_cast<" << node.className << "*>(node);\n";
  out << indent << "  // ...\n" << indent << "}\n";
  return out.str();
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
  // Nodes made in the edit mode last: the others are found before a new sibling is there to count
  auto ordered = nodes;
  std::stable_partition(ordered.begin(), ordered.end(), [](NodeDesc const& node) { return node.made.kind.empty(); });
  for (size_t i = 0; i < ordered.size(); i++)
  {
    if (i > 0)
      out << "\n";
    out << block(ordered[i], false);
  }
  return out.str();
}

namespace
{
  // Lines that find a node and do `what` with it, `node` being it: nested `if`s as a node's block has them
  std::string found(codegen::NodeDesc const& node, std::string const& what)
  {
    auto conditions = codegen::lookup(node);
    if (conditions.empty())
      return "";
    std::string out, indent;
    for (size_t i = 0; i < conditions.size(); i++)
    {
      out += indent + "if (" + conditions[i] + ")\n";
      indent += "  ";
    }
    return out + indent + what + "\n";
  }

  std::string names(std::vector<codegen::NodeDesc> const& nodes, std::vector<size_t> const& order)
  {
    std::string out;
    for (size_t i = 0; i < order.size(); i++)
    {
      auto const& node = nodes[order[i]];
      out += (i == 0 ? "" : i + 1 == order.size() ? " and " : ", ") + describe(node.id, node.className);
    }
    return out;
  }
} // namespace

std::string codegen::layoutCode(LayoutDesc const& layout)
{
  auto const& fit = layout.fit;
  if (fit.order.size() < 2 || layout.nodes.size() != fit.order.size())
    return "";
  std::ostringstream out;
  auto const& first = layout.nodes.front();
  std::string kind = fit.row ? "row" : "column";

  // Why, and what a layout will change
  std::vector<std::string> notes{names(layout.nodes, fit.order) + " as a " + kind + ": a new " +
                                 (layout.menu ? "menu" : "node") + " with an AxisLayout that puts them where they are now"};
  // A number for a comment: 12.5, not 12.5f
  auto plain = [](float value)
  {
    auto text = expr::number(value);
    return text.ends_with('f') ? text.substr(0, text.size() - 1) : text;
  };
  if (fit.gapSpread > 0.5f)
    notes.push_back("Their gaps differ by up to " + plain(fit.gapSpread) + ": the layout makes each " + plain(fit.gap) +
                    " (Alt+Shift+" + (fit.row ? "H" : "V") + " makes them the same first)");
  if (fit.crossSpread > 0.5f)
    notes.push_back(std::string("They don't line up across the ") + kind + ": the layout lines up their " +
                    (fit.cross == rowfit::Align::Center ? "middles" : fit.row ? (fit.cross == rowfit::Align::Start ? "bottoms" : "tops")
                                                                         : (fit.cross == rowfit::Align::Start ? "left edges" : "right edges")));
  for (auto const& note : notes)
    for (auto const& line : text::wrap(note, COMMENT_COLUMNS))
      out << "// " << line << "\n";

  // The new one where the box of them all is, in their parent's frame, from its middle
  NodeDesc box = first;
  box.id = "";
  box.position = {fit.bounds.midX(), fit.bounds.midY()};
  box.layout = {};
  auto position = positionExpressions(box);
  bool screen = box.frame.kind == FrameKind::Screen;
  auto x = expr::format(position.x, screen ? "winSize.width" : "parentSize.width");
  auto y = expr::format(position.y, screen ? "winSize.height" : "parentSize.height");
  bool usesExtent = position.x.k != 0 || position.y.k != 0;

  out << "{\n";
  if (usesExtent && screen)
    out << "  auto winSize = CCDirector::get()->getWinSize();\n";
  out << "  auto " << kind << " = " << (layout.menu ? "CCMenu" : "CCNode") << "::create();\n";
  out << "  " << kind << "->setID(\"" << kind << "\"_spr);\n";
  // Placed by its middle: a plain node's anchor is its corner
  out << "  " << kind << "->ignoreAnchorPointForPosition(false);\n";
  out << "  " << kind << "->setAnchorPoint({0.5f, 0.5f});\n";
  out << "  " << kind << "->setContentSize({" << expr::number(fit.bounds.width) << ", " << expr::number(fit.bounds.height)
      << "});\n";
  out << "  " << kind << "->setLayout(" << (fit.row ? "RowLayout::create()" : "ColumnLayout::create()") << "\n";
  out << "    ->setGap(" << expr::number(fit.gap) << ")\n";
  // As long as they are: the alignment along it is any
  out << "    ->setAxisAlignment(AxisAlignment::Center)\n";
  out << "    ->setCrossAxisLineAlignment(AxisAlignment::" << rowfit::alignName(fit.cross) << ")\n";
  // Top to bottom as they're added
  if (!fit.row)
    out << "    ->setAxisReverse(true)\n";
  // Their own scales kept
  out << "    ->setAutoScale(false));\n";

  // Their parent gets it: the layer, or a parent found like a node is, `node` being it
  if (layout.parentIsLayer)
    out << "  this->addChild(" << kind << ");\n";
  else if (auto add = found(layout.parent, "node->addChild(" + kind + ");"); !add.empty())
  {
    std::istringstream lines(add);
    for (std::string line; std::getline(lines, line);)
      out << "  " << line << "\n";
  }
  else
    out << "  // Add " << kind << " to " << describe(layout.parent.id, layout.parent.className)
        << ", which there's no way to find, where you make it\n";
  if (usesExtent && !screen)
    out << "  auto parentSize = " << kind << "->getParent() ? " << kind
        << "->getParent()->getContentSize() : CCSize{0, 0};\n";
  out << "  " << kind << "->setPosition({" << x << ", " << y << "});\n";

  // All found before any is moved: a node found by its index among its siblings would be another once one goes
  out << "  std::vector<CCNode*> nodes;\n";
  for (auto index : fit.order)
  {
    auto const& node = layout.nodes[index];
    auto lines = found(node, "nodes.push_back(node);");
    if (lines.empty())
    {
      out << "  // " << describe(node.id, node.className) << ": no way to find it, add it to " << kind
          << " where you make it, in this place of the order\n";
      continue;
    }
    std::istringstream stream(lines);
    for (std::string line; std::getline(stream, line);)
      out << "  " << line << "\n";
  }
  out << "  for (auto node : nodes)\n";
  out << "  {\n";
  out << "    node->retain();\n";
  out << "    node->removeFromParent();\n";
  out << "    " << kind << "->addChild(node);\n";
  out << "    node->release();\n";
  out << "  }\n";
  out << "  " << kind << "->updateLayout();\n";
  out << "}\n";
  return out.str();
}

std::string codegen::toKAndR(std::string const& code)
{
  std::vector<std::string> lines;
  size_t start = 0;
  while (start <= code.size())
  {
    auto end = code.find(char(10), start);
    if (end == std::string::npos)
    {
      if (start < code.size())
        lines.push_back(code.substr(start));
      break;
    }
    lines.push_back(code.substr(start, end - start));
    start = end + 1;
  }

  std::string out;
  for (size_t i = 0; i < lines.size(); i++)
  {
    auto const& line = lines[i];
    auto indent = line.find_first_not_of(' ');
    if (indent == std::string::npos)
    {
      out += char(10);
      continue;
    }
    auto body = line.substr(indent);
    // A brace alone goes up to the line before, unless that line is a comment or nothing
    if (body == "{" && !out.empty())
    {
      auto last = out.find_last_not_of(char(10));
      auto lineStart = out.rfind(char(10), last);
      auto previous = out.substr(lineStart == std::string::npos ? 0 : lineStart + 1, last - (lineStart == std::string::npos ? 0 : lineStart + 1) + 1);
      auto previousBody = previous.substr(std::min(previous.find_first_not_of(' '), previous.size()));
      if (!previousBody.empty() && previousBody.find("//") == std::string::npos && out.back() == char(10) &&
          last + 1 == out.size() - 1)
      {
        out.pop_back();
        out += " {";
        out += char(10);
        continue;
      }
    }
    out += std::string(indent * 2, ' ') + body;
    out += char(10);
  }
  if (!code.empty() && code.back() != char(10) && !out.empty())
    out.pop_back();
  return out;
}
