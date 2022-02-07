#pragma once

#include <Inspector/InspectorWidgetBase.hpp>
#include <Process/TimeValue.hpp>

#include <score/widgets/MarginLess.hpp>

#include <QHBoxLayout>

#include <vector>

namespace Inspector
{
class InspectorSectionWidget;
}
class QCheckBox;
class QLabel;
class QToolButton;
namespace Scenario
{
class MetadataWidget;
class TriggerInspectorWidget;
class EventModel;
class TimeSyncModel;
/*!
 * \brief The TimeSyncInspectorWidget class
 *      Inherits from InspectorWidgetInterface. Manages an interface for an
 * TimeSync (Timebox) element.
 */
class TimeSyncInspectorWidget final : public Inspector::InspectorWidgetBase
{
public:
  explicit TimeSyncInspectorWidget(
      const TimeSyncModel& object,
      const score::DocumentContext& context,
      QWidget* parent);

private:
  void updateDisplayedValues();
  void on_dateChanged(const TimeVal&);

  const TimeSyncModel& m_model;

  MetadataWidget* m_metadata{};
  QLabel* m_date{};
  QToolButton* m_autotrigger{};
  QToolButton* m_isStart{};
  TriggerInspectorWidget* m_trigwidg{};
  score::MarginLess<QHBoxLayout> m_btnLayout;
};
}
