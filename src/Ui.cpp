#include "Ui.hpp"

#include <Windows.h>

#include <cstdlib>

using namespace geode::prelude;

namespace
{
  // The bitmap font's height at scale 1, about: a TTF label of this times the scale is as tall
  constexpr float TTF_SIZE = 14.f;
  constexpr char const* TTF_FONT = "Segoe UI";
} // namespace

words::Lang ui::language()
{
  // UI_TWEAKER_LANG=en or ru: for screenshots, whatever the setting says
  char* forced = nullptr;
  size_t size = 0;
  if (_dupenv_s(&forced, &size, "UI_TWEAKER_LANG") == 0 && forced)
  {
    bool russian = std::string(forced) == "ru";
    std::free(forced);
    return russian ? words::Lang::Russian : words::Lang::English;
  }
  auto setting = Mod::get()->getSettingValue<std::string>("language");
  if (setting == "English")
    return words::Lang::English;
  if (setting == "Russian")
    return words::Lang::Russian;
  return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN ? words::Lang::Russian : words::Lang::English;
}

words::Words const& ui::words()
{
  return words::get(language());
}

UiLabel* UiLabel::create(float scale, bool ascii)
{
  auto node = new UiLabel();
  if (node->init(scale, ascii))
  {
    node->autorelease();
    return node;
  }
  delete node;
  return nullptr;
}

bool UiLabel::init(float scale, bool ascii)
{
  if (!CCNode::init())
    return false;
  if (ascii || ui::language() == words::Lang::English)
  {
    auto label = CCLabelBMFont::create("", "chatFont.fnt");
    label->setScale(scale);
    m_label = label;
    m_text = label;
    m_rgba = label;
  }
  else
  {
    // Left-aligned like the bitmap labels: a TTF label centers its lines unless told
    auto label = CCLabelTTF::create(" ", TTF_FONT, TTF_SIZE * scale, CCSizeZero, kCCTextAlignmentLeft);
    m_label = label;
    m_text = label;
    m_rgba = label;
  }
  m_label->setAnchorPoint({0, 0});
  addChild(m_label);
  setContentSize(m_label->getScaledContentSize());
  return true;
}

void UiLabel::setString(std::string const& text)
{
  if (text == m_string)
    return;
  m_string = text;
  m_text->setString(text.empty() ? " " : text.c_str());
  setContentSize(m_label->getScaledContentSize());
}

void UiLabel::setColor(ccColor3B const& color)
{
  m_rgba->setColor(color);
}
