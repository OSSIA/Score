// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "OSCQueryProtocolSettingsWidget.hpp"

#include "OSCQueryProtocolFactory.hpp"
#include "OSCQuerySpecificSettings.hpp"

#include <State/Widgets/AddressFragmentLineEdit.hpp>

#include <Device/Protocol/ProtocolSettingsWidget.hpp>

#include <Protocols/RateWidget.hpp>

#include <score/widgets/HelpInteraction.hpp>
#include <score/widgets/Pixmap.hpp>

#include <QAction>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPushButton>
#include <QString>
#include <QVariant>
namespace Protocols
{
OSCQueryProtocolSettingsWidget::OSCQueryProtocolSettingsWidget(QWidget* parent)
    : ProtocolSettingsWidget(parent)
{
  m_deviceNameEdit = new State::AddressFragmentLineEdit{this};
  checkForChanges(m_deviceNameEdit);

  m_localHostEdit = new QLineEdit(this);
  score::setHelp(
      m_localHostEdit, tr("The OSCQuery host. Example: ws://127.0.0.1:5678, "
                          "http://my.oscquery.host, ..."));
  m_rate = new RateWidget{this};
  score::setHelp(m_rate, tr("Rate limiting for outgoing messages"));
  m_localPort = new QSpinBox(this);
  m_localPort->setRange(0, 65535);
  m_localPort->setValue(0);
  score::setHelp(
      m_localPort,
      tr("Choose an explicit port for OSC listening, useful for getting automatic "
         "feedback from an external software. If 0, a random port will be chosen."));
  checkForChanges(m_localPort);

  m_dense = new QCheckBox("Dense message packing", this);
  m_dense->setChecked(false);
  m_dense->setEnabled(false);
  score::setHelp(
      m_localPort,
      tr("Indicated whether the remote API supports dense packing of values ; useful "
         "mainly for low-power embedded devices."));

  QFormLayout* layout = new QFormLayout;

  layout->addRow(tr("Name"), m_deviceNameEdit);
  layout->addRow(tr("Host"), m_localHostEdit);
  layout->addRow(tr("Local port"), m_localPort);
  layout->addRow(tr("Rate"), m_rate);
  layout->addRow(new QLabel("Supported extensions: "));
  layout->addRow(m_dense);

  layout->addRow(
      "",
      new QLabel(
          "Host examples:\n   ws://127.0.0.1:5678\n   http://my.oscquery.host\n\nLeave "
          "Local Port at 0 unless you need to\naccomodate non-OSCQuery-compliant "
          "software."));

  setLayout(layout);

  m_deviceNameEdit->setText("newDevice");
  m_localHostEdit->setText("ws://127.0.0.1:5678");
  m_rate->setRate({});
  m_localPort->setValue(0);
  m_dense->setChecked(false);
}

Device::DeviceSettings OSCQueryProtocolSettingsWidget::getSettings() const
{
  Device::DeviceSettings s;
  s.name = m_deviceNameEdit->text();
  s.protocol = OSCQueryProtocolFactory::static_concreteKey();

  OSCQuerySpecificSettings OSCQuery;
  OSCQuery.host = m_localHostEdit->text();
  OSCQuery.rate = m_rate->rate();
  OSCQuery.localPort = m_localPort->value();
  OSCQuery.dense = m_dense->isChecked();

  s.deviceSpecificSettings = QVariant::fromValue(OSCQuery);
  return s;
}

void OSCQueryProtocolSettingsWidget::setSettings(const Device::DeviceSettings& settings)
{
  m_deviceNameEdit->setText(settings.name);
  OSCQuerySpecificSettings OSCQuery;
  if(settings.deviceSpecificSettings.canConvert<OSCQuerySpecificSettings>())
  {
    OSCQuery = settings.deviceSpecificSettings.value<OSCQuerySpecificSettings>();
    m_localHostEdit->setText(OSCQuery.host);
    m_localPort->setValue(OSCQuery.localPort);
    m_rate->setRate(OSCQuery.rate);
    m_dense->setChecked(OSCQuery.dense);
  }
}
}
