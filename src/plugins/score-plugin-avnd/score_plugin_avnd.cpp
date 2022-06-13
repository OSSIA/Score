/*
#include <Examples/AudioEffect.hpp>
#include <Examples/AudioEffectWithSidechains.hpp>
#include <Examples/Empty.hpp>
#include <Examples/Distortion.hpp>
#include <Examples/ZeroDependencyAudioEffect.hpp>

#include <Examples/SampleAccurateGenerator.hpp>
#include <Examples/SampleAccurateFilter.hpp>

#include <Examples/TextureGenerator.hpp>
#include <Examples/TextureFilter.hpp>
#include <Examples/TrivialGenerator.hpp>
#include <Examples/TrivialFilter.hpp>
#include <Examples/RawPorts.hpp>
#include <Examples/ControlGallery.hpp>

#include <Examples/CCC.hpp>
#include <Examples/Synth.hpp>

*/
#include "score_plugin_avnd.hpp"


#include <score/plugins/FactorySetup.hpp>

#include <boost/pfr.hpp>
#include <Process/ProcessMetadata.hpp>
#include <Process/Process.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/Dataflow/Port.hpp>
#include <Process/Dataflow/PortFactory.hpp>
#include <score_plugin_engine.hpp>
#include <ossia/detail/typelist.hpp>

#define AVND_TEST_BUILD 0
#if AVND_TEST_BUILD
#include <avnd/../../examples/Raw/Addition.hpp>
#include <avnd/../../examples/Raw/Callback.hpp>
#include <avnd/../../examples/Raw/Init.hpp>
#include <avnd/../../examples/Raw/Presets.hpp>
#include <avnd/../../examples/Raw/PerSampleProcessor2.hpp>
#include <avnd/../../examples/Raw/Lowpass.hpp>
#include <avnd/../../examples/Raw/Ui.hpp>
#include <avnd/../../examples/Raw/Midi.hpp>
#include <avnd/../../examples/Raw/PerSampleProcessor.hpp>
#include <avnd/../../examples/Raw/Modular.hpp>
#include <avnd/../../examples/Raw/SampleAccurateControls.hpp>
#include <avnd/../../examples/Raw/Minimal.hpp>
#include <avnd/../../examples/Raw/Messages.hpp>
#include <avnd/../../examples/Raw/Sines.hpp>
#include <avnd/../../examples/Helpers/Controls.hpp>
#include <avnd/../../examples/Helpers/FFTDisplay.hpp>
#include <avnd/../../examples/Helpers/ImageUi.hpp>
#include <avnd/../../examples/Helpers/Logger.hpp>
#include <avnd/../../examples/Helpers/Lowpass.hpp>
#include <avnd/../../examples/Helpers/Messages.hpp>
#include <avnd/../../examples/Helpers/Noise.hpp>
#include <avnd/../../examples/Helpers/PerSample.hpp>
#include <avnd/../../examples/Helpers/PerBus.hpp>
#include <avnd/../../examples/Helpers/Peak.hpp>
#include <avnd/../../examples/Helpers/Midi.hpp>
#include <avnd/../../examples/Helpers/Sines.hpp>
#include <avnd/../../examples/Helpers/Ui.hpp>
#include <avnd/../../examples/Helpers/UiBus.hpp>
#include <avnd/../../examples/Tutorial/ZeroDependencyAudioEffect.hpp>
#include <avnd/../../examples/Tutorial/AudioEffectExample.hpp>
#include <avnd/../../examples/Tutorial/TextureFilterExample.hpp>
#include <avnd/../../examples/Tutorial/TrivialGeneratorExample.hpp>
#include <avnd/../../examples/Tutorial/TrivialFilterExample.hpp>
// #include <avnd/../../examples/Tutorial/Synth.hpp>
#include <avnd/../../examples/Tutorial/TextureGeneratorExample.hpp>
#include <avnd/../../examples/Tutorial/EmptyExample.hpp>
#include <avnd/../../examples/Tutorial/SampleAccurateFilterExample.hpp>
#include <avnd/../../examples/Tutorial/ControlGallery.hpp>
#include <avnd/../../examples/Tutorial/AudioSidechainExample.hpp>
#include <avnd/../../examples/Tutorial/SampleAccurateGeneratorExample.hpp>
#include <avnd/../../examples/Tutorial/Distortion.hpp>
#include <avnd/../../examples/Ports/LitterPower/CCC.hpp>
#include <avnd/../../examples/Gpu/Compute.hpp>
#include <avnd/../../examples/Gpu/DrawRaw.hpp>
#include <avnd/../../examples/Gpu/DrawWithHelpers.hpp>
#include <avnd/../../examples/Gpu/SolidColor.hpp>
#include <avnd/../../examples/Helpers/PeakBandFFTPort.hpp>
#include <avnd/../../examples/Advanced/Granular/Granolette.hpp>
#include <brigand/sequences/list.hpp>
#endif
#include <avnd/../../examples/Tests/ParametricEq.hpp>


/**
 * This file instantiates the classes that are provided by this plug-in.
 */

#include <halp/meta.hpp>
#include <Avnd/Factories.hpp>

score_plugin_avnd::score_plugin_avnd() = default;
score_plugin_avnd::~score_plugin_avnd() = default;

std::vector<std::unique_ptr<score::InterfaceBase>>
score_plugin_avnd::factories(
    const score::ApplicationContext& ctx,
    const score::InterfaceKey& key) const
{
  using namespace oscr;
#if AVND_TEST_BUILD
  using namespace examples;
  using namespace examples::helpers;

  struct config {
      using logger_type = halp::basic_logger;
  };
#endif

  return oscr::instantiate_fx<
      examples::ParametricEq
    #if AVND_TEST_BUILD
      oscr::Granolette
      , examples::helpers::FFTDisplay
      , examples::helpers::MessageBusUi
      , examples::helpers::AdvancedUi
      , Addition
      , Callback
      , Controls
      , Logger<config>
      , examples::Lowpass
      , examples::helpers::Lowpass
      , examples::Messages
      , examples::helpers::Messages<config>
      , examples::helpers::Midi
      , examples::helpers::WhiteNoise
      , examples::helpers::Peak
      , examples::helpers::PerBusAsArgs
      , examples::helpers::PerBusAsPortsFixed
      , examples::helpers::PerBusAsPortsDynamic
      , examples::helpers::PerSampleAsArgs
      , examples::helpers::PerSampleAsPorts
      , examples::helpers::Sine
      , examples::Init
      , litterpower_ports::CCC
      , examples::Midi
      , examples::Minimal
      , examples::Modular
      , examples::PerSampleProcessor
      , examples::PerSampleProcessor2
      , examples::Presets
      , examples::SampleAccurateControls
      , examples::Sine
      , examples::Ui
      , Distortion
      , AudioEffectExample
      , AudioSidechainExample
      , EmptyExample
      , SampleAccurateGeneratorExample
      , SampleAccurateFilterExample
      , ControlGallery
//      , RawPortsExample
//      , Synth
#if SCORE_PLUGIN_GFX
      , TextureGeneratorExample
      , TextureFilterExample
      , examples::GpuComputeExample
      , examples::GpuFilterExample
      , examples::GpuRawExample
      , examples::GpuSolidColorExample
#endif
      , TrivialGeneratorExample
      , TrivialFilterExample
      , ZeroDependencyAudioEffect
    #endif
      >(ctx, key);
}
std::vector<score::PluginKey> score_plugin_avnd::required() const
{
  return {score_plugin_engine::static_key()};
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_plugin_avnd)
