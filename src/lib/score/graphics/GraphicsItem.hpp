#pragma once
#include <QList>
#include <QPainterPath>
#include <QUrl>

#include <score_lib_base_export.h>

#include <memory>
#include <optional>
#include <verdigris>
class QGraphicsObject;
class QGraphicsItem;
class QGraphicsView;

namespace score
{
struct DocumentContext;
}
/**
 * @brief deleteGraphicsItem Properly delete a QGraphicsObject
 * @param item item to delete
 *
 * Simply using deleteLater() is generally not enough, the
 * item has to be removed from the scene else there will be crashes.
 */
SCORE_LIB_BASE_EXPORT void deleteGraphicsObject(QGraphicsObject* item);
SCORE_LIB_BASE_EXPORT void deleteGraphicsItem(QGraphicsItem* item);

SCORE_LIB_BASE_EXPORT
QGraphicsView* getView(const QGraphicsItem& self);
SCORE_LIB_BASE_EXPORT
QGraphicsView* getView(const QPainter& painter);

SCORE_LIB_BASE_EXPORT
QImage newImage(double logical_w, double logical_h);

SCORE_LIB_BASE_EXPORT
std::optional<QPointF> mapPointToItem(QPoint global, QGraphicsItem& item);

SCORE_LIB_BASE_EXPORT
void registerItemHelp(int itemType, QString tooltip, QUrl url) noexcept;
SCORE_LIB_BASE_EXPORT
QUrl getItemHelpUrl(int itemType) noexcept;

template <typename T>
struct graphics_item_ptr
{
  T* impl{};
  graphics_item_ptr() = default;
  graphics_item_ptr(const graphics_item_ptr&) = default;
  graphics_item_ptr(graphics_item_ptr&&) noexcept = default;
  graphics_item_ptr& operator=(const graphics_item_ptr&) = default;
  graphics_item_ptr& operator=(graphics_item_ptr&&) noexcept = default;

  graphics_item_ptr(T* p)
      : impl{p}
  {
  }

  ~graphics_item_ptr() { deleteGraphicsItem(impl); }

  auto operator=(T* other) { impl = other; }

  operator bool() const { return impl; }

  operator T*() const { return impl; }

  T& operator*() const { return *impl; }

  T* operator->() const { return impl; }
};
