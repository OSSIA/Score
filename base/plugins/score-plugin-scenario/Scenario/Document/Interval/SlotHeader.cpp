#include "SlotHeader.hpp"

#include <Automation/AutomationPresenter.hpp>
#include <Curve/CurveView.hpp>
#include <Process/LayerView.hpp>
#include <Process/ProcessMimeSerialization.hpp>
#include <Process/Style/ScenarioStyle.hpp>
#include <QDrag>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QWidget>
#include <QJsonDocument>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Interval/IntervalPresenter.hpp>
#include <Scenario/Document/Interval/IntervalHeader.hpp>
#include <Scenario/Document/Interval/IntervalView.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <Scenario/Document/Interval/Temporal/TemporalIntervalPresenter.hpp>
#include <wobjectimpl.h>
#include <Scenario/Application/Menus/ScenarioCopy.hpp>
namespace Scenario
{
SlotHeader::SlotHeader(
    const IntervalPresenter& slotView, int slotIndex, QGraphicsItem* parent)
    : QGraphicsItem{parent}
    , m_presenter{slotView}
    , m_width{slotView.view()->boundingRect().width()}
    , m_slotIndex{slotIndex}
{
  this->setCacheMode(QGraphicsItem::NoCache);
  this->setAcceptHoverEvents(true);
  this->setFlag(ItemClipsToShape);
  this->setFlag(ItemClipsChildrenToShape);
  this->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

int SlotHeader::slotIndex() const
{
  return m_slotIndex;
}

void SlotHeader::setSlotIndex(int v)
{
  m_slotIndex = v;
}

QRectF SlotHeader::boundingRect() const
{
  return {0., 0., m_width, headerHeight()};
}

void SlotHeader::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  painter->setRenderHint(QPainter::Antialiasing, false);
  const auto& style = ScenarioStyle::instance();
  painter->setPen(style.SlotHeaderPen);
  painter->setBrush(style.NoBrush);

  if (m_width > 20)
  {

    // Grip
    static const std::array<QRectF, 6> rects{[] {
      std::array<QRectF, 6> rects;
      for (int i = 0; i < 3; i++)
        rects[i] = {6., 4 + 3. * i, 1, 1};
      return rects;
    }()};
    painter->drawRects(rects.data(), 6);

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Frame
    painter->drawRect(QRectF{0., 0., m_width, headerHeight() - 1});

    // Menu
    {
      auto view = getView(*this);
      if (!view)
        return;

      double r = 4.5;

      const double leftXinView
          = view->mapFromScene(mapToScene(QPointF{(m_width - 8.) - r, 0.}))
                .x();
      const double rightXinView = leftXinView + 2. * r;

      double centerX = m_width - 8.;
      const double centerY = 7.5;

      const constexpr double min_x = 10.;
      const double max_x = view->width() - 30.;
      if (leftXinView <= min_x)
      {
        centerX += (-leftXinView + min_x);
      }
      else if (rightXinView >= max_x)
      {
        centerX += (-rightXinView + max_x);
      }
      centerX = std::max(centerX, 5.);

      painter->setRenderHint(QPainter::Antialiasing, false);
      painter->drawLine(centerX - r, centerY + 0, centerX + r, centerY + 0);
      painter->drawLine(centerX - r, centerY - 3, centerX + r, centerY - 3);
      painter->drawLine(centerX - r, centerY + 3, centerX + r, centerY + 3);
      m_menupos = centerX;
    }
  }
  else
  {
    painter->drawRect(QRectF{0., 0., m_width, headerHeight() - 1});
  }
}

void SlotHeader::setWidth(qreal width)
{
  prepareGeometryChange();
  m_width = width;
  update();
}

static std::unique_ptr<QDrag> slot_header_drag = nullptr;
static bool slot_drag_moving = false;
void SlotHeader::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  slot_header_drag.reset();
  slot_drag_moving = false;
  m_presenter.selectedSlot(m_slotIndex);

  const auto xpos = event->pos().x();
  if (xpos >= 0 && xpos < 16)
  {
    slot_header_drag.reset(new QDrag(event->widget()));
  }
  else if (xpos >= m_menupos - 4 && xpos < m_menupos + 4)
  {
    // menu
    m_presenter.requestSlotMenu(m_slotIndex, event->screenPos(), event->scenePos());
  }
  else if(boundingRect().contains(event->pos()) && m_presenter.getSlots()[m_slotIndex].processes.size() > 1)
  {
    if (const auto tip
        = dynamic_cast<const TemporalIntervalPresenter*>(&m_presenter))
      tip->requestProcessSelectorMenu(
          m_slotIndex, event->screenPos(), event->scenePos());
  }
  event->accept();
}

void SlotHeader::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();

  const auto xpos = event->pos().x();
  if (xpos >= 0 && xpos < 16 && slot_header_drag)
  {
    auto min_dist = (event->screenPos() - event->buttonDownScreenPos(Qt::LeftButton))
                    .manhattanLength() >= QApplication::startDragDistance();
    if (min_dist)
    {
      slot_drag_moving = true;
    }
    if (!slot_drag_moving)
    {
      return;
    }

    bool temporal = dynamic_cast<const TemporalIntervalPresenter*>(&m_presenter);
    QMimeData* mime = new QMimeData;
    auto proc_id =
        temporal
        ? *m_presenter.model().smallView()[m_slotIndex].frontProcess
        : m_presenter.model().fullView()[m_slotIndex].process;

    auto& proc = m_presenter.model().processes.at(proc_id);
    auto json = copyProcess(proc);
    json["Path"] = toJsonObject(score::IDocument::path(proc));
    json["Duration"] = m_presenter.model().duration.defaultDuration().msec();
    json["SlotIndex"] = m_slotIndex;
    json["View"] = temporal ? QStringLiteral("Small") : QStringLiteral("Full");
    mime->setData(score::mime::layerdata(), QJsonDocument{json}.toJson());
    slot_header_drag->setMimeData(mime);


    auto view = m_presenter.getSlots()[m_slotIndex].processes.front().view;
    slot_header_drag->setPixmap(view->pixmap().scaledToWidth(50));
    slot_header_drag->setHotSpot(QPoint(5, 5));

    QObject::connect(slot_header_drag.get(), &QDrag::destroyed,
                     &m_presenter, [p=&m_presenter] { p->stopSlotDrag(); });

    m_presenter.startSlotDrag(m_slotIndex, mapToParent(event->pos()));
    slot_header_drag->exec();
    auto ptr = slot_header_drag.release();
    ptr->deleteLater();
  }
}

void SlotHeader::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  slot_header_drag.reset();
  slot_drag_moving = false;
  event->accept();
}

void SlotHeader::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
  const auto xpos = event->pos().x();
  if (xpos >= 0 && xpos < 16)
  {
    if (this->cursor().shape() != Qt::CrossCursor)
      setCursor(Qt::CrossCursor);
  }
  else
  {
    if (this->cursor().shape() == Qt::CrossCursor)
      unsetCursor();
  }
}

void SlotHeader::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
  const auto xpos = event->pos().x();
  if (xpos >= 0 && xpos < 16)
  {
    if (this->cursor().shape() != Qt::CrossCursor)
      setCursor(Qt::CrossCursor);
  }
  else
  {
    if (this->cursor().shape() == Qt::CrossCursor)
      unsetCursor();
  }
}

void SlotHeader::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
  if (this->cursor().shape() == Qt::CrossCursor)
    unsetCursor();
}

SlotDragOverlay::SlotDragOverlay(const IntervalPresenter& c, Slot::RackView v)
  : interval{c}
  , view{v}
{
  this->setAcceptDrops(true);
  this->setZValue(9999);
}


QRectF SlotDragOverlay::boundingRect() const
{
  auto rect = interval.view()->boundingRect();
  rect.adjust(0, 0, 0, 10);
  return rect;
}


void SlotDragOverlay::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const auto& style = ScenarioStyle::instance();
  auto c = style.IntervalBase.getBrush().color();
  c.setAlphaF(0.2);
  painter->fillRect(interval.view()->boundingRect(), c);
  painter->fillRect(m_drawnRect, QColor::fromRgbF(0.6, 0.6, 1., 0.8));
  painter->setPen(QPen(QColor::fromRgbF(0.6, 0.6, 1., 0.7), 2));
  painter->drawRect(m_drawnRect);
}


void SlotDragOverlay::onDrag(QPointF pos)
{
  const auto y = pos.y();
  const auto& itv = interval.model();
  const auto rect = interval.view()->boundingRect();

  double height = interval.header()->headerHeight();

  if(y <= height)
  {
    m_drawnRect = {0, height - 2.5, rect.width(), 5};
    update();
  }
  else if(y > rect.height() - 5.)
  {
    m_drawnRect = {0, rect.height() - 5., rect.width(), 5};
    update();
  }
  else
  {
    const int N = int(view == Slot::SmallView ? itv.smallView().size() : itv.fullView().size());
    for (int i = 0; i < N; i++)
    {
      const auto next_height = itv.getSlotHeight({i, view}) + SlotHeader::headerHeight() + SlotHandle::handleHeight();
      if(y > height - 2.5 && y < height + 2.5)
      {
        m_drawnRect = {0, height - 2.5, rect.width(), 5};
        update();
        break;
      }
      else if(y < height + next_height - 2.5)
      {
        m_drawnRect = {0, height, rect.width(), next_height};
        update();
        break;
      }

      height += next_height;
    }

    if(y > height - 2.5 && y < height + 2.5)
    {
      m_drawnRect = {0, height - 2.5, rect.width(), 5};
      update();
    }
  }
}


void SlotDragOverlay::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
  m_drawnRect = {};

  onDrag(event->pos());
  event->accept();
}


void SlotDragOverlay::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
  onDrag(event->pos());
  event->accept();
}


void SlotDragOverlay::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
  m_drawnRect = {};
  update();
  event->accept();
}


void SlotDragOverlay::dropEvent(QGraphicsSceneDragDropEvent* event)
{
  event->accept();
  const auto pos = event->pos();
  const auto y = pos.y();
  const auto rect = interval.view()->boundingRect();
  const auto& itv = interval.model();
  double height = interval.header()->headerHeight();
  const int N = int(view == Slot::SmallView ? itv.smallView().size() : itv.fullView().size());

  if(y <= height)
  {
    m_drawnRect = {0, height - 2.5, rect.width(), 5};
    dropBefore(0);
    update();
  }
  else if(y > rect.height() - 5.)
  {
    m_drawnRect = {0, rect.height() - 5., rect.width(), 5};
    dropBefore(N);
    update();
  }
  else
  {
    for (int i = 0; i < N; i++)
    {
      const auto next_height = itv.getSlotHeight({i, view}) + SlotHeader::headerHeight() + SlotHandle::handleHeight();
      if(y > height - 2.5 && y < height + 2.5)
      {
        m_drawnRect = {0, height - 2.5, rect.width(), 5};
        dropBefore(i);
        update();
        return;
      }
      else if(y < height + next_height - 2.5)
      {
        m_drawnRect = {0, height, rect.width(), next_height};
        dropIn(i);
        update();
        return;
      }

      height += next_height;
    }

    if(y > height - 2.5 && y < height + 2.5)
    {
      m_drawnRect = {0, height - 2.5, rect.width(), 5};
      dropBefore(N);
      update();
      return;
    }
  }
}

}

W_OBJECT_IMPL(Scenario::SlotDragOverlay)
