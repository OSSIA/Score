#include "EffectInspector.hpp"

#include <Media/Commands/InsertEffect.hpp>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <score/document/DocumentContext.hpp>

#if defined(HAS_FAUST)
#  include <Media/Commands/EditFaustEffect.hpp>
#  include <Media/Effect/Faust/FaustEffectModel.hpp>
#endif

#if defined(LILV_SHARED)
#  include <Media/ApplicationPlugin.hpp>
#  include <Media/Effect/LV2/LV2EffectModel.hpp>
#  include <lilv/lilvmm.hpp>
#  include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#endif

#if defined(HAS_VST2)
#  include <Media/Effect/VST/VSTEffectModel.hpp>
#endif

#include <Process/ProcessList.hpp>
#include <Scenario/DialogWidget/AddProcessDialog.hpp>
namespace Media
{
namespace Effect
{

InspectorWidget::InspectorWidget(
    const Effect::ProcessModel& object,
    const score::DocumentContext& doc,
    QWidget* parent)
    : InspectorWidgetDelegate_T{object, parent}
    , m_dispatcher{doc.commandStack}
    , m_ctx{doc}
{
  setObjectName("EffectInspectorWidget");

  auto lay = new QVBoxLayout;
  m_list = new QListWidget;
  lay->addWidget(m_list);
  con(process(), &Effect::ProcessModel::effectsChanged, this,
      &InspectorWidget::recreate);

  connect(
      m_list, &QListWidget::itemDoubleClicked, &object,
      [&](QListWidgetItem* item) {
        Process::setupExternalUI(object.effects().at(item->data(Qt::UserRole).value<Id<Process::ProcessModel>>()), doc, true);
      },
      Qt::QueuedConnection);

  m_list->setContextMenuPolicy(Qt::DefaultContextMenu);
  recreate();

  // Add an effect
  {
    auto add = new QPushButton{tr("Add (Score)")};
    connect(add, &QPushButton::pressed, this, [&] { add_score(cur_pos()); });
    lay->addWidget(add);
  }
  {
#if defined(HAS_FAUST)
    auto add = new QPushButton{tr("Add (Faust)")};
    connect(add, &QPushButton::pressed, this, [&] { add_faust(cur_pos()); });

    lay->addWidget(add);
#endif
  }

  {
#if defined(LILV_SHARED)
    auto add = new QPushButton{tr("Add (LV2)")};
    connect(add, &QPushButton::pressed, this, [&] { add_lv2(cur_pos()); });

    lay->addWidget(add);
#endif
  }

#if defined(HAS_VST2)
  {
    auto add = new QPushButton{tr("Add (VST 2)")};
    connect(add, &QPushButton::pressed, this, [&] { add_vst2(cur_pos()); });

    lay->addWidget(add);
  }
#endif

  // Remove an effect
  // Effects changed
  // Effect list
  // Double-click : open editor window.

  this->setLayout(lay);
}

void InspectorWidget::add_score(std::size_t pos)
{
  auto& base_fxs = m_ctx.app.interfaces<Process::ProcessFactoryList>();
  auto dialog = new Scenario::AddProcessDialog(
      base_fxs, Process::ProcessFlags::SupportsEffectChain, this);

  dialog->on_okPressed = [&](const auto& proc, const QString&) {
    m_dispatcher.submitCommand(
        new InsertEffect{process(), proc, {}, pos});
  };
  dialog->launchWindow();
  dialog->deleteLater();
}

void InspectorWidget::add_lv2(std::size_t pos)
{
#if defined(LILV_SHARED)
  auto txt = LV2::LV2EffectFactory{}.customConstructionData();
  if (!txt.isEmpty())
  {
    m_dispatcher.submitCommand(
        new InsertGenericEffect<LV2::LV2EffectModel>{process(), txt,
                                                               pos});
  }
#endif
}
void InspectorWidget::add_faust(std::size_t pos)
{
#if defined(HAS_FAUST)
  m_dispatcher.submitCommand(
      new InsertGenericEffect<Faust::FaustEffectModel>{
          process(), "process = _;", pos});
#endif
}

void InspectorWidget::add_vst2(std::size_t pos)
{
#if defined(HAS_VST2)
  auto res = VST::VSTEffectFactory{}.customConstructionData();

  if (!res.isEmpty())
  {
    m_dispatcher.submitCommand(
        new InsertGenericEffect<VST::VSTEffectModel>{process(), res,
                                                               pos});
  }
#endif
}

void InspectorWidget::addRequested(int pos)
{
}

int InspectorWidget::cur_pos()
{
  if (m_list->currentRow() >= 0)
    return m_list->currentRow();
  return process().effects().size();
}
struct ListWidgetItem
    : public QObject
    , public QListWidgetItem
{
public:
  using QListWidgetItem::QListWidgetItem;
};

void InspectorWidget::recreate()
{
  m_list->clear();

  for (const auto& fx : process().effects())
  {
    auto item = new ListWidgetItem(fx.prettyName(), m_list);

    con(fx.metadata(), &score::ModelMetadata::LabelChanged, item,
        [=](const auto& txt) { item->setText(txt); });
    item->setData(Qt::UserRole, QVariant::fromValue(fx.id()));
    m_list->addItem(item);
  }
}
}
}
