#pragma once
#include <score/graphics/widgets/Constants.hpp>

#include <QGraphicsItem>
#include <QObject>

#include <score_lib_base_export.h>

#include <verdigris>

namespace score
{

class SCORE_LIB_BASE_EXPORT QGraphicsNoteChooser final
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(QGraphicsNoteChooser)
  SCORE_GRAPHICS_ITEM_TYPE(120)

  static constexpr int m_min = -2;
  static constexpr int m_max = 127;
  static constexpr double m_width = 30;
  static constexpr double m_height = 28;
  QPointF m_startPos{};
  double m_curValue{};
  int m_value{};
  bool m_grab{};

public:
  explicit QGraphicsNoteChooser(QGraphicsItem* parent);

  void setValue(int v);
  int value() const;

  bool moving = false;

public:
  void sliderMoved() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderMoved)
  void sliderReleased() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderReleased)

private:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
      override;
  QRectF boundingRect() const override;
};

}
