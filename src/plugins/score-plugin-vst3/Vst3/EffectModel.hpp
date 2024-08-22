#pragma once
#include <Process/Dataflow/PortFactory.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/Process.hpp>

#include <Control/DefaultEffectItem.hpp>
#include <Effect/EffectFactory.hpp>
#include <Vst3/Plugin.hpp>

#include <score/tools/std/Invoke.hpp>

#include <ossia/detail/hash_map.hpp>

#include <verdigris>

#include <public.sdk/source/vst/utility/uid.h>
#define VST_DEFAULT_PARAM_NUMBER_CUTOFF 10

namespace vst3
{
class Model;
class ControlInlet;
}
PROCESS_METADATA(
    , vst3::Model, "4cc30435-3237-4e94-a79e-1c2bd6b724a1", "VST 3", "VST 3",
    Process::ProcessCategory::Other, "Plugins", "VST 3 plug-in",
    "VST is a trademark of Steinberg Media Technologies GmbH", {}, {}, {},
    QUrl("https://ossia.io/score-docs/processes/audio-plugins.html#common-formats-vst-vst3-lv2-jsfx"),
    Process::ProcessFlags::ExternalEffect)
UUID_METADATA(
    , Process::Port, vst3::ControlInlet, "82b24dd8-fbc0-43a6-adfa-7bb29ca48660")
DESCRIPTION_METADATA(, vst3::Model, "VST")
namespace vst3
{

class CreateVSTControl;
class ControlInlet;
struct PortCreationVisitor;

class Model final : public Process::ProcessModel
{
  W_OBJECT(Model)
  SCORE_SERIALIZE_FRIENDS
  friend class vst3::CreateVSTControl;

public:
  PROCESS_METADATA_IMPL(Model)
  Model(
      TimeVal t, const QString& name, const Id<Process::ProcessModel>&, QObject* parent);

  ~Model() override;
  template <typename Impl>
  Model(Impl& vis, QObject* parent)
      : ProcessModel{vis, parent}
  {
    init();
    vis.writeTo(*this);
  }

  ControlInlet* getControl(const Id<Process::Port>& p) const;
  QString effect() const noexcept override;
  QString prettyName() const noexcept override;
  bool hasExternalUI() const noexcept;

  Plugin fx{};

  ossia::hash_map<Steinberg::Vst::ParamID, ControlInlet*> controls;

  void removeControl(const Id<Process::Port>&);
  void removeControl(Steinberg::Vst::ParamID fxnum);

  void addControlFromEditor(Steinberg::Vst::ParamID id);
  void on_addControl(const Steinberg::Vst::ParameterInfo& v);
  void on_addControl_impl(ControlInlet* inl);
  void initControl(ControlInlet* inl);

  void reloadControls();
  Steinberg::tresult restartComponent(int32_t flags);

private:
  void loadPreset(const Process::Preset& preset) override;
  Process::Preset savePreset() const noexcept override;
  std::vector<Process::Preset> builtinPresets() const noexcept override;

  void init();
  void create();
  void load();
  void mapAllControls(int numParams);
  QByteArray readProcessorState() const;
  QByteArray readControllerState() const;
  void writeState();

  QString m_vstPath;
  VST3::UID m_uid{};

  QByteArray m_savedProcessorState;
  QByteArray m_savedControllerState;

  ossia::hash_map<Steinberg::Vst::ParamID, int32_t> m_paramToIndex;

  void closePlugin();
  void initFx();

  friend struct PortCreationVisitor;
};
}

namespace Process
{
template <>
QString EffectProcessFactory_T<vst3::Model>::customConstructionData() const noexcept;

template <>
Process::Descriptor
EffectProcessFactory_T<vst3::Model>::descriptor(QString d) const noexcept;
}

namespace vst3
{
using VSTEffectFactory = Process::EffectProcessFactory_T<Model>;
}
