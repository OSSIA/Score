#pragma once
#include <Device/Protocol/DeviceSettings.hpp>
#include <Device/Protocol/ProtocolSettingsWidget.hpp>

#include <ossia/protocols/midi/midi.hpp>

#include <verdigris>

namespace State
{
class AddressFragmentLineEdit;
}
namespace score
{
class ComboBox;
}
class QCheckBox;
class QRadioButton;
class QWidget;
class QLineEdit;

namespace Protocols
{
class MIDIInputSettingsWidget final : public Device::ProtocolSettingsWidget
{
  W_OBJECT(MIDIInputSettingsWidget)

public:
  MIDIInputSettingsWidget(QWidget* parent = nullptr);

private:
  Device::DeviceSettings getSettings() const override;

  void setSettings(const Device::DeviceSettings& settings) override;

  State::AddressFragmentLineEdit* m_name{};
  QCheckBox* m_createWhole{};
  QCheckBox* m_virtualPort{};
  Device::DeviceSettings m_current;
};
class MIDIOutputSettingsWidget final : public Device::ProtocolSettingsWidget
{
  W_OBJECT(MIDIOutputSettingsWidget)

public:
  MIDIOutputSettingsWidget(QWidget* parent = nullptr);

private:
  Device::DeviceSettings getSettings() const override;

  void setSettings(const Device::DeviceSettings& settings) override;

  State::AddressFragmentLineEdit* m_name{};
  QCheckBox* m_createWhole{};
  QCheckBox* m_virtualPort{};
  Device::DeviceSettings m_current;
};
}
