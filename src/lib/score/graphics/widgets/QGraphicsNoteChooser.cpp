#include <score/graphics/widgets/QGraphicsNoteChooser.hpp>
#include <score/model/Skin.hpp>
#include <score/tools/Cursor.hpp>

#include <ossia/detail/math.hpp>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <wobjectimpl.h>
namespace score
{

QRectF QGraphicsNoteChooser::boundingRect() const
{
  return {0, 0, m_width, m_height};
}

QGraphicsNoteChooser::QGraphicsNoteChooser(QGraphicsItem* parent)
    : QGraphicsItem{parent}
{
  auto& skin = score::Skin::instance();
  setCursor(skin.CursorScaleV);
  this->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

void QGraphicsNoteChooser::setValue(int v)
{
  switch(v)
  {
    case 255:
      m_value = -1;
      break;
    case 254:
      m_value = -2;
      break;
    default:
      m_value = ossia::clamp(v, m_min, m_max);
      break;
  }

  update();
}

int QGraphicsNoteChooser::value() const
{
  return m_value;
}

void QGraphicsNoteChooser::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  const auto srect = boundingRect();
  if(srect.contains(event->pos()))
  {
    m_grab = true;
  }

  m_startPos = event->screenPos();
  m_curValue = m_value;
  score::hideCursor(true);

  event->accept();
}

void QGraphicsNoteChooser::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  if(m_grab)
  {
    auto pos = event->screenPos();
    auto dy = ossia::clamp((m_startPos.y() - pos.y()) / 10., -1., 1.);
    if(dy != 0)
    {
      m_curValue = ossia::clamp(m_curValue + dy, (double)m_min, (double)m_max);

      if(int res = std::round(m_curValue); res != m_value)
      {
        m_value = res;
        m_curValue = m_value;
        sliderMoved();
        update();
      }
    }

    score::moveCursorPos(m_startPos);
    score::hideCursor(true);
  }
  event->accept();
}

void QGraphicsNoteChooser::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  mouseMoveEvent(event);

  if(m_grab)
  {
    m_grab = false;
    sliderReleased();
  }
  score::showCursor();
}

static QString noteText(int n)
{
  static constexpr QStringView lit[12]{u"C",  u"C#", u"D",  u"D#", u"E",  u"F",
                                       u"F#", u"G",  u"G#", u"A",  u"A#", u"B"};
  switch(n)
  {
    case -1:
      return "AC";
    case -2:
      return "SL";
    default:
      return QString{"%1%2"}.arg(lit[n % 12]).arg(n / 12 - 1);
  }
}

void QGraphicsNoteChooser::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto& style = score::Skin::instance();
  //const QPen& text = style.Gray.main.pen1;
  const QFont& textFont = style.MonoFontSmall;
  const QPen& currentText = style.Base4.lighter180.pen1;
  // const QBrush& bg = style.SliderBrush;
  // const QPen& noPen = style.NoPen;

  painter->setFont(textFont);
  //painter->setPen(noPen);
  //painter->setBrush(bg);

  //painter->drawRect(boundingRect());
  painter->setPen(currentText);
  painter->drawText(
      boundingRect(), QString{"%1\n%2"}.arg(noteText(m_value)).arg(m_value),
      QTextOption(Qt::AlignLeft));
}
}

W_OBJECT_IMPL(score::QGraphicsNoteChooser)
