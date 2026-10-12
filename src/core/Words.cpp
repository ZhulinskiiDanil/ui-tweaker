#include "Words.hpp"

namespace
{
  words::Words const ENGLISH = {
      .banner = "UI Tweaker: edit mode ({} leaves)",
      .helpHint = "F1: all the keys",
      .help1 = "Drag: move   Wheel: scale (Shift: fine)   Arrows: 1 px (Shift: 10)   Ctrl+Z / Ctrl+Y: undo / redo",
      .help2 = "Enter: type values   Ctrl+C: copy code   Ctrl+Shift+C: copy all   Tab: next under the mouse   Alt: distances   Alt+click: "
               "inside   Shift+click: several   PgUp: parent   Ctrl+arrows: anchor   [ ]: z-order   R: reset   V: hide   A: other screens   C: code   T: tree   H: panel",
      .nothingSelected = "Nothing selected",
      .clickHint = "Click a node to select it, drag to move it",
      .position = "position {}, {}   (screen {}, {})",
      .scale = "scale {}{}",
      .rotation = "rotation {}",
      .anchor = "anchor {}, {}{}",
      .ignored = " (ignored)",
      .size = "size {} x {}",
      .zOrder = "z-order {}   parent {}",
      .none = "none",
      .copy = "Copy",
      .copyAll = "Copy all ({})",
      .undo = "Undo",
      .redo = "Redo",
      .leave = "Leave",
      .offScreen = "Off the screen on {} (A shows them)",
      .selectFirst = "Select a node first",
      .nothingEdited = "Nothing is edited in this scene",
      .copiedFor = "Copied the code for {}",
      .copiedNodes = "Copied the code for {} nodes",
      .treeTitle = "Nodes: {} of {}   T: close",
      .searchHint = "/ to search by ID or class",
      .search = "search: {}",
      .anchorLayout = "{} has an AnchorLayout: updateLayout() puts the node at its anchor, so the code sets the "
                      "anchor and the offset",
      .axisLayout = "{} has a {}: updateLayout() puts the node back in line, so the position is lost. The code sets "
                    "its gap in the layout instead",
      .simpleAxisLayout = "{} has a {}: updateLayout() puts the node back in line, so the position is lost. The code "
                          "has the gap before it as an AxisGap, and the position as a comment",
      .otherLayout = "{} has a {}: updateLayout() may move the node back",
      .layoutMoves = "{} has a {}: its updateLayout() sets this node's position, so a setPosition of yours lasts until "
                     "it runs",
      .action = "1 action is running on it: an animation may set its position or scale every frame",
      .actions = "{} actions are running on it: an animation may set its position or scale every frame",
      .hidden = "Not drawn: {} is invisible (setVisible(false))",
      .itself = "the node itself",
      .transparent = "Not seen: its opacity is 0",
      .zeroScale = "Not seen: its scale is 0",
      .zeroSize = "Its content size is 0 x 0: it has no box of its own, its anchor point does nothing and a layout "
                  "gives it no room. setContentSize, or look at its children",
      .ignoresAnchor = "It ignores its anchor point for its position (a layer or a menu as made): the position is "
                       "where its bottom left corner is",
      .clipped = "Outside {}, which cuts off what's outside its box",
      .parentTurned = "Its parent is turned, flipped or skewed: positions are along the parent's axes, not the "
                      "screen's",
      .outsideScreen = "Off the screen",
      .tag = "{}\nin {}",
      .scaleField = "scale",
      .widthField = "width",
      .heightField = "height",
      .typingHint = "Tab: next   Enter: apply   Esc: cancel   w h: screen, pw ph: parent",
      .typingError = "Can't read {}",
      .selectedCount = "{} selected",
      .look = "shows {}",
  };

  words::Words const RUSSIAN = {
      .banner = "UI Tweaker: режим редактирования ({} - выйти)",
      .helpHint = "F1: все клавиши",
      .help1 = "Тащить: сдвиг   Колесо: масштаб (Shift: точно)   Стрелки: 1 px (Shift: 10)   Ctrl+Z / Ctrl+Y: "
               "отменить / вернуть",
      .help2 = "Enter: ввести числа   Ctrl+C: код   Ctrl+Shift+C: код всех   Tab: следующая под мышью   Alt: расстояния   Alt+клик: "
               "внутрь   Shift+клик: несколько   PgUp: родитель   Ctrl+стрелки: anchor   [ ]: z-order   R: сбросить   V: скрыть   A: другие экраны   C: код   T: дерево   H: панель",
      .nothingSelected = "Ничего не выбрано",
      .clickHint = "Щёлкните по ноде, чтобы выбрать, тащите, чтобы сдвинуть",
      .position = "позиция {}, {}   (на экране {}, {})",
      .scale = "масштаб {}{}",
      .rotation = "поворот {}",
      .anchor = "anchor {}, {}{}",
      .ignored = " (не учитывается)",
      .size = "размер {} x {}",
      .zOrder = "z-order {}   родитель {}",
      .none = "нет",
      .copy = "Код",
      .copyAll = "Код всех ({})",
      .undo = "Отменить",
      .redo = "Вернуть",
      .leave = "Выйти",
      .offScreen = "За краем экрана на {} (A - показать)",
      .selectFirst = "Сначала выберите ноду",
      .nothingEdited = "В этой сцене ничего не изменено",
      .copiedFor = "Код для {} скопирован",
      .copiedNodes = "Код для {} нод скопирован",
      .treeTitle = "Ноды: {} из {}   T: закрыть",
      .searchHint = "/ - поиск по ID или классу",
      .search = "поиск: {}",
      .anchorLayout = "У {} AnchorLayout: updateLayout() ставит ноду к её якорю, поэтому код задаёт якорь и смещение",
      .axisLayout = "У {} {}: updateLayout() вернёт ноду в ряд, позиция потеряется. Код задаёт вместо неё отступ в "
                    "layout",
      .simpleAxisLayout = "У {} {}: updateLayout() вернёт ноду в ряд, позиция потеряется. В коде отступ перед ней "
                          "как AxisGap, а позиция в комментарии",
      .otherLayout = "У {} {}: updateLayout() может вернуть ноду на место",
      .layoutMoves = "У {} {}: его updateLayout() задаёт позицию этой ноды, ваш setPosition продержится до "
                     "следующего вызова",
      .action = "На ноде идёт 1 action: анимация может каждый кадр менять её позицию или масштаб",
      .actions = "На ноде идут action ({}): анимация может каждый кадр менять её позицию или масштаб",
      .hidden = "Не рисуется: {} невидима (setVisible(false))",
      .itself = "сама нода",
      .transparent = "Не видна: прозрачность 0",
      .zeroScale = "Не видна: масштаб 0",
      .zeroSize = "Размер контента 0 x 0: у ноды нет своей рамки, anchor point ни на что не влияет, а layout не даёт "
                  "ей места. Задайте setContentSize или смотрите детей",
      .ignoresAnchor = "Позиция не учитывает anchor point (как у слоя или меню): позиция - это левый нижний угол",
      .clipped = "За пределами {}, а она обрезает всё, что вне её рамки",
      .parentTurned = "Родитель повёрнут, отражён или скошен: позиция идёт по осям родителя, а не экрана",
      .outsideScreen = "За краем экрана",
      .tag = "{}\nв {}",
      .scaleField = "масштаб",
      .widthField = "ширина",
      .heightField = "высота",
      .typingHint = "Tab: дальше   Enter: применить   Esc: отмена   w h: экран, pw ph: родитель",
      .typingError = "Не читается {}",
      .selectedCount = "Выбрано: {}",
      .look = "показывает {}",
  };
} // namespace

words::Words const& words::get(Lang lang)
{
  return lang == Lang::Russian ? RUSSIAN : ENGLISH;
}
