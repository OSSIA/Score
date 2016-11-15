#include <Process/Style/ScenarioStyle.hpp>
#include <QPainter>

#include "LayerView.hpp"

class QStyleOptionGraphicsItem;
class QWidget;
namespace Process
{
LayerView::~LayerView() = default;


LayerView::LayerView(QGraphicsItem* parent):
    QGraphicsItem{parent}
{
    this->setCacheMode(QGraphicsItem::ItemCoordinateCache);

}

QRectF LayerView::boundingRect() const
{
    return {0, 0, m_width, m_height};
}

void LayerView::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* option,
                      QWidget* widget)
{
    paint_impl(painter);
}


void LayerView::setHeight(qreal height)
{
    prepareGeometryChange();
    m_height = height;
    emit heightChanged();
}


qreal LayerView::height() const
{
    return m_height;
}


void LayerView::setWidth(qreal width)
{
    prepareGeometryChange();
    m_width = width;
    emit widthChanged();
}


qreal LayerView::width() const
{
    return m_width;
}
}
