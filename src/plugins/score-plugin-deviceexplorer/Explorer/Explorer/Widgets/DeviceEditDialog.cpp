// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "DeviceEditDialog.hpp"

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>
#include <Device/Protocol/ProtocolSettingsWidget.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>

#include <score/application/GUIApplicationContext.hpp>
#include <score/model/Skin.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/widgets/SignalUtils.hpp>
#include <score/widgets/TextLabel.hpp>

#include <core/document/Document.hpp>

#include <ossia/detail/algorithms.hpp>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVariant>
#include <QWidget>
#include <qnamespace.h>

#include <wobjectimpl.h>

#include <utility>
W_OBJECT_IMPL(Explorer::DeviceEditDialog)
namespace Explorer
{
DeviceEditDialog::DeviceEditDialog(
    const DeviceExplorerModel& model,
    const Device::ProtocolFactoryList& pl,
    Mode mode,
    QWidget* parent)
    : QDialog{parent}
    , m_model{model}
    , m_protocolList{pl}
    , m_mode{mode}
    , m_protocolWidget{nullptr}
    , m_index{-1}
{
  const auto& skin = score::Skin::instance();
  const QColor textHeaderColor = QColor("#D5D5D5");
  auto setHeaderTextFormat = [&](QLabel* label)
  {
    label->setFont(skin.TitleFont);
    auto p = label->palette();
    p.setColor(QPalette::WindowText, textHeaderColor);
    label->setPalette(p);
  };

  setWindowTitle(tr("Add device"));
  auto gridLayout = new QGridLayout{};
  setLayout(gridLayout);
  setModal(true);

  m_invalidLabel = new QLabel{
      tr("Cannot add device.\n Try changing the name to make it unique, \nor "
         "check that the ports aren't already used")};
  m_invalidLabel->setAlignment(Qt::AlignRight);
  m_invalidLabel->setTextFormat(Qt::PlainText);
  m_buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
  m_okButton = m_buttonBox->addButton(tr("Add"), QDialogButtonBox::AcceptRole);
  m_buttonBox->addButton(QDialogButtonBox::Cancel);

  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  m_protocols = new QTreeWidget{this};
  m_protocols->header()->hide();
  m_protocols->setSelectionMode(QAbstractItemView::SingleSelection);
  if (m_mode == Mode::Editing)
  {
    m_protocols->setVisible(false);
  }
  else
  {
    m_protocols->setFixedWidth(150);

    {
      m_protocolsLabel = new QLabel{tr("Protocols"), this};
      setHeaderTextFormat(m_protocolsLabel);
      gridLayout->addWidget(m_protocolsLabel, 0, 0, Qt::AlignHCenter);
    }
    gridLayout->addWidget(m_protocols, 1, 0);
  }

  m_devices = new QListWidget{this};
  m_devices->setFixedWidth(140);
  gridLayout->addWidget(m_devices, 1, 1);
  {
    m_devicesLabel = new QLabel{tr("Devices"), this};
    setHeaderTextFormat(m_devicesLabel);
    gridLayout->addWidget(m_devicesLabel, 0, 1, Qt::AlignHCenter);
  }

  {
    m_protocolNameLabel = new QLabel{tr("Settings"), this};
    setHeaderTextFormat(m_protocolNameLabel);
    gridLayout->addWidget(
        m_protocolNameLabel, 0, 2, -1, -1, Qt::AlignLeft | Qt::AlignTop);
  }
  m_main = new QWidget{this};
  gridLayout->addWidget(m_main, 1, 2, -1, -1);
  m_layout = new QFormLayout;
  m_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  m_main->setLayout(m_layout);
  m_layout->addRow(m_invalidLabel);
  m_layout->addRow(m_buttonBox);

  initAvailableProtocols();

  connect(
      m_protocols,
      &QTreeView::activated,
      this,
      [this] { selectedProtocolChanged(); });
  connect(
      m_devices,
      &QListWidget::currentRowChanged,
      this,
      [this] { selectedDeviceChanged(); });

  if (m_protocols->topLevelItemCount() > 0)
  {
    //  m_protocols->setCurrentItem();
    //  m_index = m_protocols->currentRow();
    selectedProtocolChanged();
  }

  setMinimumWidth(700);
  setMinimumHeight(400);

  setAcceptEnabled(false);
}

DeviceEditDialog::~DeviceEditDialog() { }

void DeviceEditDialog::initAvailableProtocols()
{
  // initialize previous settings
  m_previousSettings.clear();

  std::vector<Device::ProtocolFactory*> sorted;
  for (auto& elt : m_protocolList)
  {
    sorted.push_back(&elt);
  }

  ossia::sort(
      sorted,
      [](Device::ProtocolFactory* lhs, Device::ProtocolFactory* rhs)
      { return lhs->visualPriority() > rhs->visualPriority(); });
  for (const auto& prot_pair : sorted)
  {
    auto& prot = *prot_pair;
    auto cat_list
        = m_protocols->findItems(prot.category(), Qt::MatchFixedString);
    QTreeWidgetItem* categoryItem;
    if (cat_list.size() == 0)
    {
      categoryItem = new QTreeWidgetItem;
      categoryItem->setText(0, prot.category());
      categoryItem->setFlags(Qt::ItemIsEnabled);
      m_protocols->addTopLevelItem(categoryItem);
    }
    else
    {
      categoryItem = cat_list.first();
    }

    auto item = new QTreeWidgetItem{categoryItem};
    item->setText(0, prot.prettyName());
    item->setData(0, Qt::UserRole, QVariant::fromValue(prot.concreteKey()));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_previousSettings.append(prot.defaultSettings());
  }

  for (int i = 0; i < m_protocols->topLevelItemCount(); i++)
  {
    auto catItem = m_protocols->topLevelItem(i);
    catItem->setExpanded(true);
    auto font = catItem->font(0);
    font.setPixelSize(13);
    font.setBold(true);
    catItem->setFont(0, font);
  }

  m_protocols->setRootIsDecorated(false);
  m_protocols->setExpandsOnDoubleClick(false);
  m_index = 0;
}

void DeviceEditDialog::selectedDeviceChanged()
{
  if (!m_devices->isVisible() || m_devices->count() == 0)
    return;
  auto item = m_devices->currentItem();
  if (!item)
    return;

  auto name = item->text();
  auto data = item->data(Qt::UserRole).value<Device::DeviceSettings>();
  if (m_protocolWidget)
    m_protocolWidget->setSettings(data);

  updateValidity();
}

void DeviceEditDialog::selectedProtocolChanged()
{
  auto doc = score::GUIAppContext().currentDocument();
  if (!doc)
    return;

  // Recreate
  if (m_protocols->selectedItems().isEmpty())
  {
    //m_devices->setVisible(false);
    //m_devicesLabel->setVisible(false);
    return;
  }
  auto selected_item = m_protocols->selectedItems().first();
  auto key = selected_item->data(0, Qt::UserRole)
                 .value<UuidKey<Device::ProtocolFactory>>();
  if (key == UuidKey<Device::ProtocolFactory>{})
    return;

  // Clear listener
  if (m_enumerator)
  {
    m_enumerator.reset();
  }

  // Clear devices
  m_devices->clear();

  // Clear protocol widget
  if (m_protocolWidget)
  {
    SCORE_ASSERT(m_index < m_previousSettings.count());

    m_previousSettings[m_index] = m_protocolWidget->getSettings();
    delete m_protocolWidget;
    m_protocolWidget = nullptr;
  }

  auto protocol = m_protocolList.get(key);
  m_enumerator.reset(protocol->getEnumerator(*doc));
  if (m_enumerator)
  {
    m_devices->setVisible(true);
    m_devicesLabel->setVisible(true);

    auto addItem = [&](const Device::DeviceSettings& settings)
    {
      auto item = new QListWidgetItem;
      item->setText(settings.name);
      item->setData(Qt::UserRole, QVariant::fromValue(settings));
      m_devices->addItem(item);
    };
    auto rmItem = [&](const QString& name)
    {
      auto items = m_devices->findItems(name, Qt::MatchExactly);
      for (auto item : items)
        delete m_devices->takeItem(m_devices->row(item));
    };
    connect(
        m_enumerator.get(),
        &Device::DeviceEnumerator::deviceAdded,
        this,
        addItem);
    connect(
        m_enumerator.get(),
        &Device::DeviceEnumerator::deviceRemoved,
        this,
        rmItem);
    m_enumerator->enumerate(addItem);
  }
  else
  {
    m_devices->setVisible(false);
    m_devicesLabel->setVisible(false);
  }
  m_protocolNameLabel->setText(
      tr("Settings (%1)").arg(protocol->prettyName()));
  m_protocolWidget = protocol->makeSettingsWidget();

  if (m_protocolWidget)
  {
    m_protocolWidget->setSettings(protocol->defaultSettings());
    connect(
        m_protocolWidget,
        &Device::ProtocolSettingsWidget::changed,
        this,
        &DeviceEditDialog::updateValidity);

    m_layout->insertRow(0, m_protocolWidget);

    QSizePolicy pol{
        QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding};
    pol.setVerticalStretch(1);
    m_protocolWidget->setSizePolicy(pol);
    m_protocolWidget->setMinimumHeight(200);
    this->setSizePolicy(
        QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    updateGeometry();
  }
  updateValidity();
}

Device::DeviceSettings DeviceEditDialog::getSettings() const
{
  if (m_protocolWidget)
    return m_protocolWidget->getSettings();

  return {};
}

Device::Node DeviceEditDialog::getDevice() const
{
  if (m_protocolWidget)
    return m_protocolWidget->getDevice();

  return {};
}

void DeviceEditDialog::setSettings(const Device::DeviceSettings& settings)
{
  m_originalName = settings.name;

  for (int i = 0; i < m_protocols->topLevelItemCount(); i++)
  {
    auto catItem = m_protocols->topLevelItem(i);
    for (int j = 0; j < catItem->childCount(); j++)
    {
      auto item = catItem->child(j);
      if (item->data(0, Qt::UserRole).value<UuidKey<Device::ProtocolFactory>>()
          == settings.protocol)
      {
        m_protocols->setCurrentItem(item);
        selectedProtocolChanged();
        if (m_protocolWidget)
        {
          m_protocolWidget->setSettings(settings);
        }
        updateValidity();
        return;
      }
    }
  }
}

void DeviceEditDialog::setAcceptEnabled(bool st)
{
  m_okButton->setEnabled(st);
  m_invalidLabel->setVisible(!st);
}

void DeviceEditDialog::setBrowserEnabled(bool st)
{
  if (!st)
  {
    m_enumerator.reset();

    delete m_protocols;
    m_protocols = nullptr;
    delete m_protocolsLabel;
    m_protocolsLabel = nullptr;
    delete m_devices;
    m_devices = nullptr;
    delete m_devicesLabel;
    m_devicesLabel = nullptr;

    auto this_lay = this->layout();
    this_lay->removeWidget(m_main);
    delete this_lay;
    auto lay = m_main->layout();
    this->setLayout(lay);
  }
}

void DeviceEditDialog::updateValidity()
{
  switch (m_mode)
  {
    case Mode::Creating:
      setAcceptEnabled(m_model.checkDeviceInstantiatable(getSettings()));
      break;
    case Mode::Editing:
      setAcceptEnabled(
          m_model.checkDeviceEditable(m_originalName, getSettings()));
      break;
  }
}
}
