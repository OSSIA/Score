// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "IntervalInspectorWidget.hpp"

#include <Inspector/InspectorLayout.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/QuantificationWidget.hpp>
#include <score/widgets/SelectionButton.hpp>
#include <score/widgets/Separator.hpp>
#include <score/widgets/SetIcons.hpp>
#include <score/widgets/StyleSheets.hpp>
#include <score/widgets/TextLabel.hpp>

#include <QCheckBox>
#include <QToolBar>

#include <Scenario/Application/ScenarioApplicationPlugin.hpp>
#include <Scenario/Commands/Cohesion/DoForSelectedIntervals.hpp>
#include <Scenario/Commands/Cohesion/InterpolateStates.hpp>
#include <Scenario/Commands/Interval/MakeBus.hpp>
#include <Scenario/Commands/Signature/SignatureCommands.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentPresenter.hpp>
#include <Scenario/Inspector/Interval/SpeedSlider.hpp>
#include <Scenario/Inspector/Interval/Widgets/DurationSectionWidget.hpp>
#include <Scenario/Inspector/MetadataWidget.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

namespace Scenario
{
IntervalInspectorWidget::IntervalInspectorWidget(
    const Inspector::InspectorWidgetList& widg,
    const IntervalModel& object,
    const score::DocumentContext& ctx,
    QWidget* parent)
    : InspectorWidgetBase{object,
                          ctx,
                          parent,
                          (object.graphal() ? tr("Direct transition") : tr("Interval (%1)").arg(object.metadata().getName()))}
    , m_model{object}
{
  using namespace score;
  using namespace score::IDocument;
  setObjectName("Interval");
  if (object.graphal())
    return;

  std::vector<QWidget*> parts;
  ////// HEADER
  // metadata
  auto meta = new MetadataWidget{
      m_model.metadata(), ctx.commandStack, &m_model, this};

  meta->setupConnections(m_model);
  addHeader(meta);

  ////// BODY
  // Separator
  auto w = new QWidget;
  auto lay = new Inspector::Layout{w};
  parts.push_back(w);

  auto btnLayout = new QHBoxLayout;
  btnLayout->setContentsMargins(0, 0, 0, 0);

  // Full View
  {
    auto fullview = new QToolButton;
    fullview->setIcon(makeIcons(
        QStringLiteral(":/icons/fullview_on.png"),
        QStringLiteral(":/icons/fullview_hover.png"),
        QStringLiteral(":/icons/fullview_off.png"),
        QStringLiteral(":/icons/fullview_off.png")));
    fullview->setToolTip(tr("FullView"));
    fullview->setStatusTip(tr("Display the content of the selcted interval in full view\n"
                              "Same effect as double clicking on its name"));
    fullview->setAutoRaise(true);
    fullview->setIconSize(QSize{28, 28});

    connect(fullview, &QToolButton::clicked, this, [this] {
      auto base = get<ScenarioDocumentPresenter>(*documentFromObject(m_model));
      if (base)
        base->setDisplayedInterval(&model());
    });
    btnLayout->addWidget(fullview);
  }

  // Audio
  {
    ScenarioDocumentModel& doc
        = get<ScenarioDocumentModel>(*documentFromObject(m_model));
    auto busWidg = new QToolButton{this};

    busWidg->setIcon(makeIcons(
        QStringLiteral(":/icons/audio_bus_on.png"),
        QStringLiteral(":/icons/audio_bus_hover.png"),
        QStringLiteral(":/icons/audio_bus_off.png"),
        QStringLiteral(":/icons/audio_bus_off.png")));
    busWidg->setToolTip(tr("Audio bus"));
    busWidg->setStatusTip(
          tr("Add audio output controls to the bus section of the audio mixer"));
    busWidg->setCheckable(true);
    busWidg->setChecked(ossia::contains(doc.busIntervals, &m_model));
    busWidg->setAutoRaise(true);
    busWidg->setIconSize(QSize{28, 28});

    connect(busWidg, &QToolButton::toggled, this, [=, &ctx, &doc](bool b) {
      bool is_bus = ossia::contains(doc.busIntervals, &m_model);
      if ((b && !is_bus) || (!b && is_bus))
      {
        CommandDispatcher<> disp{ctx.commandStack};
        disp.submit<Command::SetBus>(doc, m_model, b);
      }
    });

    btnLayout->addWidget(busWidg);
  }

  // Time signature
  {
    auto sigWidg = new QToolButton{this};

    sigWidg->setIcon(makeIcons(
        QStringLiteral(":/icons/time_signature_on.png"),
        QStringLiteral(":/icons/time_signature_hover.png"),
        QStringLiteral(":/icons/time_signature_off.png"),
        QStringLiteral(":/icons/time_signature_off.png")));
    sigWidg->setToolTip(tr("Time signature"));
    sigWidg->setStatusTip(tr("Specifiy a different time signature from the parent interval"));
    sigWidg->setCheckable(true);
    sigWidg->setAutoRaise(true);
    sigWidg->setChecked(this->m_model.hasTimeSignature());
    sigWidg->setIconSize(QSize{28, 28});

    connect(sigWidg, &QToolButton::toggled, this, [=](bool b) {
      if (b != this->m_model.hasTimeSignature())
      {
        this->commandDispatcher()->submit<Command::SetHasTimeSignature>(
            m_model, b);
      }
    });
    btnLayout->addWidget(sigWidg);
  }
  {
    auto interp = new QToolButton{this};
    interp->setToolTip(tr("Interpolate states (Ctrl+K)"));
    interp->setStatusTip(tr("Interpolate states (Ctrl+K)\n"
                            "Create automations between values contained in the start and end states"));
    interp->setShortcut(QKeySequence(tr("Ctrl+K")));
    interp->setIcon(makeIcons(
        QStringLiteral(":/icons/interpolate_on.png"),
        QStringLiteral(":/icons/interpolate_hover.png"),
        QStringLiteral(":/icons/interpolate_off.png"),
        QStringLiteral(":/icons/interpolate_disabled.png")));
    interp->setIconSize(QSize{28, 28});
    interp->setAutoRaise(true);

    connect(interp, &QToolButton::clicked, this, [&] {
      DoForSelectedIntervals(this->context(), Command::InterpolateStates);
    });

    btnLayout->addWidget(interp);
  }
  {
    QWidget* spacerWidget = new QWidget(this);
    spacerWidget->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacerWidget->setVisible(true);
    btnLayout->addWidget(spacerWidget);
  }
  ((QBoxLayout*)meta->layout())->insertLayout(0, btnLayout);

  // Speed
  auto speedWidg = new SpeedWidget{true, true, this};
  speedWidg->setInterval(m_model);
  lay->addRow(tr("Speed"), speedWidg);

  // Quantization
  {
    auto quantiz = new score::QuantificationWidget{this};
    quantiz->setQuantification(m_model.quantizationRate());
    quantiz->setStatusTip(tr("Set quantification to synchronize the interval start"));

    QObject::connect(
        quantiz,
        &score::QuantificationWidget::quantificationChanged,
        this,
        [&ctx, &object](double v) {
          CommandDispatcher<>{ctx.commandStack}
              .submit<Scenario::Command::SetIntervalQuantizationRate>(object, v);
        });

    connect(&m_model, &IntervalModel::quantizationRateChanged,
            quantiz, &score::QuantificationWidget::setQuantification);
    lay->addRow(tr("Quantization"), quantiz);
  }

  // Durations
  auto& ctrl = ctx.app.guiApplicationPlugin<ScenarioApplicationPlugin>();
  auto dur = new DurationWidget{ctrl.editionSettings(), *lay, this};
  dur->setStatusTip(tr("Set the duration of the interval\n"
                       "Min: Define the minimal duration before evaluating the trigger\n"
                       "Max: Define the duration before triggering automatically\n"));
  lay->addWidget(dur);

  // Display data
  updateAreaLayout(parts);
}

IntervalInspectorWidget::~IntervalInspectorWidget() = default;

IntervalModel& IntervalInspectorWidget::model() const
{
  return const_cast<IntervalModel&>(m_model);
}
}
