#pragma once

// The language of the edit mode's interface, and its labels: GD's bitmap fonts have no Cyrillic, so in Russian
// they're TTF labels, rendered again only when their text changes
#include "core/Words.hpp"

#include <Geode/Geode.hpp>

#include <string>

namespace ui
{
  // The setting, or Windows' language for Auto
  words::Lang language();
  words::Words const& words();
} // namespace ui

class UiLabel : public cocos2d::CCNode
{
public:
  // `scale` is the bitmap font's; in Russian a TTF label about as tall. Bitmap in Russian too when `ascii`
  // (node IDs and class names)
  static UiLabel* create(float scale, bool ascii = false);

  void setString(std::string const& text);
  std::string const& getString() const { return m_string; }
  void setColor(cocos2d::ccColor3B const& color);

private:
  cocos2d::CCNode* m_label = nullptr;
  cocos2d::CCLabelProtocol* m_text = nullptr;
  cocos2d::CCRGBAProtocol* m_rgba = nullptr;
  std::string m_string;

  bool init(float scale, bool ascii);
};
