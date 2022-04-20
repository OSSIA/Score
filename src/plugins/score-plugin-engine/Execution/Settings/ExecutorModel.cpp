// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ExecutorModel.hpp"

#include <Execution/Clock/DataflowClock.hpp>
#include <Execution/Clock/DefaultClock.hpp>
#include <Execution/Transport/JackTransport.hpp>

#include <Audio/Settings/Model.hpp>

#include <score/application/ApplicationContext.hpp>

#include <Scenario/Execution/score2OSSIA.hpp>
#if defined(OSSIA_AUDIO_JACK)
#include <Audio/JackInterface.hpp>
#endif
namespace Execution
{
namespace Settings
{

namespace Parameters
{
SETTINGS_PARAMETER_IMPL(Clock){
    QStringLiteral("score_plugin_engine/Clock"),
    Dataflow::ClockFactory::static_concreteKey()};
SETTINGS_PARAMETER_IMPL(Rate){QStringLiteral("score_plugin_engine/Rate"), 50};
SETTINGS_PARAMETER_IMPL(Scheduling){
    QStringLiteral("score_plugin_engine/Scheduling"),
    SchedulingPolicies{}.StaticTC};
SETTINGS_PARAMETER_IMPL(Ordering){
    QStringLiteral("score_plugin_engine/Ordering"),
    OrderingPolicies{}.CreationOrder};
SETTINGS_PARAMETER_IMPL(Merging){
    QStringLiteral("score_plugin_engine/Merging"),
    MergingPolicies{}.Merge};
SETTINGS_PARAMETER_IMPL(Commit){
    QStringLiteral("score_plugin_engine/Commit"),
    CommitPolicies{}.Merged};
SETTINGS_PARAMETER_IMPL(Tick){
    QStringLiteral("score_plugin_engine/Tick"),
    TickPolicies{}.Buffer};
SETTINGS_PARAMETER_IMPL(Parallel){
    QStringLiteral("score_plugin_engine/Parallel"),
    false};
SETTINGS_PARAMETER_IMPL(ExecutionListening){
    QStringLiteral("score_plugin_engine/ExecListening"),
    true};
SETTINGS_PARAMETER_IMPL(Logging){
    QStringLiteral("score_plugin_engine/Logging"),
    false};
SETTINGS_PARAMETER_IMPL(Bench){
    QStringLiteral("score_plugin_engine/Bench"),
    false};
SETTINGS_PARAMETER_IMPL(ScoreOrder){
    QStringLiteral("score_plugin_engine/ScoreOrder"),
    false};
SETTINGS_PARAMETER_IMPL(ValueCompilation){
    QStringLiteral("score_plugin_engine/ValueCompilation"),
    true};
SETTINGS_PARAMETER_IMPL(TransportValueCompilation){
    QStringLiteral("score_plugin_engine/TransportValueCompilation"),
    false};

static auto list()
{
  return std::tie(
      Clock,
      Rate,
      Scheduling,
      Ordering,
      Merging,
      Commit,
      Tick,
      Parallel,
      ExecutionListening,
      Logging,
      Bench,
      ScoreOrder,
      ValueCompilation,
      TransportValueCompilation);
}
}

Model::Model(QSettings& set, const score::ApplicationContext& ctx)
    : m_clockFactories{ctx.interfaces<ClockFactoryList>()}
    , m_transportInterfaces{ctx.interfaces<Transport::TransportInterfaceList>()}
{
  score::setupDefaultSettings(set, Parameters::list(), *this);
}

std::unique_ptr<Clock> Model::makeClock(const Execution::Context& ctx) const
{
  auto it = m_clockFactories.get(m_Clock);
  return it ? it->make(ctx) : std::make_unique<Dataflow::Clock>(ctx);
}

time_function Model::makeTimeFunction(const score::DocumentContext& ctx) const
{
  auto it = m_clockFactories.get(m_Clock);
  return it ? it->makeTimeFunction(ctx)
            : Dataflow::ClockFactory{}.makeTimeFunction(ctx);
}

reverse_time_function
Model::makeReverseTimeFunction(const score::DocumentContext& ctx) const
{
  auto it = m_clockFactories.get(m_Clock);
  return it ? it->makeReverseTimeFunction(ctx)
            : Dataflow::ClockFactory{}.makeReverseTimeFunction(ctx);
}

Transport::TransportInterface* Model::getTransport() const
{
#if defined(OSSIA_AUDIO_JACK)
  auto& audioSettings = score::AppContext().settings<Audio::Settings::Model>();
  if (audioSettings.getDriver() == Audio::JackFactory::static_concreteKey())
  {
    if (audioSettings.getJackTransport()
        != Audio::Settings::ExternalTransport::None)
    {
      if (auto t = m_transportInterfaces.get(
              Execution::JackTransport::static_concreteKey()))
      {
        return t;
      }
    }
  }
#endif
  return m_transportInterfaces.get(
      Transport::DirectTransport::static_concreteKey());
}

SCORE_SETTINGS_PARAMETER_CPP(ClockFactory::ConcreteKey, Model, Clock)
SCORE_SETTINGS_PARAMETER_CPP(QString, Model, Scheduling)
SCORE_SETTINGS_PARAMETER_CPP(QString, Model, Ordering)
SCORE_SETTINGS_PARAMETER_CPP(QString, Model, Merging)
SCORE_SETTINGS_PARAMETER_CPP(QString, Model, Commit)
SCORE_SETTINGS_PARAMETER_CPP(QString, Model, Tick)
SCORE_SETTINGS_PARAMETER_CPP(int, Model, Rate)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, Parallel)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, ExecutionListening)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, Logging)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, Bench)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, ScoreOrder)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, ValueCompilation)
SCORE_SETTINGS_PARAMETER_CPP(bool, Model, TransportValueCompilation)
}
}
