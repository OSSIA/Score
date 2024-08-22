#pragma once
#include <Process/Execution/ProcessComponent.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/Process.hpp>

#include <Control/DefaultEffectItem.hpp>
#include <Effect/EffectFactory.hpp>
#include <LV2/Context.hpp>

#include <ossia/dataflow/node_process.hpp>

#include <lilv/lilvmm.hpp>

#include <verdigris>

namespace LV2
{
class Model;
}
PROCESS_METADATA(
    , LV2::Model, "fd5243ba-70b5-4164-b44a-ecb0dcdc0494", "LV2", "LV2",
    Process::ProcessCategory::Other, "Plugins", "LV2 plug-in", "ossia score", {}, {}, {},
    QUrl("https://ossia.io/score-docs/processes/audio-plugins.html#common-formats-vst-vst3-lv2-jsfx"),
    Process::ProcessFlags::ExternalEffect)
DESCRIPTION_METADATA(, LV2::Model, "LV2")
namespace LV2
{
class Model : public Process::ProcessModel
{
  W_OBJECT(Model)
  SCORE_SERIALIZE_FRIENDS
public:
  PROCESS_METADATA_IMPL(Model)
  Model(
      TimeVal t, const QString& name, const Id<Process::ProcessModel>&, QObject* parent);

  ~Model() override;
  template <typename Impl>
  Model(Impl& vis, QObject* parent)
      : ProcessModel{vis, parent}
  {
    vis.writeTo(*this);
  }

  QString effect() const noexcept override { return m_effectPath; }

  void setEffect(const QString& s) { m_effectPath = s; }

  QString prettyName() const noexcept override;

  bool hasExternalUI() const noexcept;

  const LilvPlugin* plugin{};
  mutable LV2::EffectContext effectContext;
  LV2::HostContext* hostContext{};

  std::size_t m_controlInStart{};
  std::size_t m_controlOutStart{};
  mutable moodycamel::ReaderWriterQueue<Message> ui_events;     // from ui to score
  mutable moodycamel::ReaderWriterQueue<Message> plugin_events; // from plug-in
  mutable moodycamel::ReaderWriterQueue<Message>
      to_process_events; // from score to process

  ossia::hash_map<uint32_t, std::pair<Process::ControlInlet*, bool>> control_map;
  ossia::hash_map<uint32_t, Process::ControlOutlet*> control_out_map;

private:
  void reload();
  QString m_effectPath;
  void readPlugin();

  ossia::float_vector fInControls, fOutControls;
};

class LV2EffectComponent final
    : public Execution::ProcessComponent_T<LV2::Model, ossia::node_process>
{
  W_OBJECT(LV2EffectComponent)
  COMPONENT_METADATA("57f50003-a179-424a-80b1-b9394d73a84a")

public:
  static constexpr bool is_unique = true;

  LV2EffectComponent(LV2::Model& proc, const Execution::Context& ctx, QObject* parent);

  void lazy_init() override;

  void
  writeAtomToUi(uint32_t port_index, uint32_t type, uint32_t size, const void* body);
  void writeAtomToUi(uint32_t port_index, LV2_Atom& atom);
};
}

namespace Process
{
template <>
QString EffectProcessFactory_T<LV2::Model>::customConstructionData() const noexcept;
}

namespace LV2
{
using ProcessFactory = Process::EffectProcessFactory_T<Model>;
using ExecutorFactory = Execution::ProcessComponentFactory_T<LV2EffectComponent>;
}
