#include "ApplicationPlugin.hpp"

#include <Media/Effect/Settings/Model.hpp>
#if defined(LILV_SHARED)
#  include <Media/Effect/LV2/LV2Context.hpp>
#  include <Media/Effect/LV2/LV2EffectModel.hpp>
#endif
#if defined(HAS_VST2)
#  include <Media/Effect/VST/VSTEffectModel.hpp>
#  include <Media/Effect/VST/VSTLoader.hpp>
#endif
#include <ossia/audio/audio_protocol.hpp>

#include <Protocols/Audio/AudioDevice.hpp>
#include <Device/Protocol/DeviceInterface.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <QDirIterator>
#include <QFileInfo>
#include <QCoreApplication>
#include <wobjectimpl.h>
#include <QProcess>
W_OBJECT_IMPL(Media::ApplicationPlugin)

// TODO remove me in a few versions
static bool vst_invalid_format = false;
#if defined(HAS_VST2)
template <>
void DataStreamReader::read<Media::ApplicationPlugin::vst_info>(
    const Media::ApplicationPlugin::vst_info& p)
{
  m_stream << p.path << p.prettyName << p.displayName << p.author << p.uniqueID << p.controls << p.isSynth << p.isValid;
}
template <>
void DataStreamWriter::write<Media::ApplicationPlugin::vst_info>(
    Media::ApplicationPlugin::vst_info& p)
{
  if(!vst_invalid_format)
    m_stream >> p.path >> p.prettyName >> p.displayName >> p.author >> p.uniqueID >> p.controls >> p.isSynth >> p.isValid;
  if(m_stream.stream.status() != QDataStream::Status::Ok)
  {
    vst_invalid_format = true;
  }
}

Q_DECLARE_METATYPE(Media::ApplicationPlugin::vst_info)
W_REGISTER_ARGTYPE(Media::ApplicationPlugin::vst_info)
Q_DECLARE_METATYPE(std::vector<Media::ApplicationPlugin::vst_info>)
W_REGISTER_ARGTYPE(std::vector<Media::ApplicationPlugin::vst_info>)

#endif

#if defined(LILV_SHARED)
namespace Media::LV2
{
void on_uiMessage(
    SuilController controller,
    uint32_t port_index,
    uint32_t buffer_size,
    uint32_t protocol,
    const void* buffer)
{
  auto& fx = *(LV2EffectModel*)controller;

  auto it = fx.control_map.find(port_index);
  if (it == fx.control_map.end()) {
    qDebug() << fx.effect() << " (LV2): invalid write on port" << port_index;
    return;
  }

  // currently writing from score
  if(it->second.second)
    return;

  Message c{port_index, protocol, {}};
  c.body.resize(buffer_size);
  auto b = (const uint8_t*) buffer;
  for(uint32_t i = 0; i < buffer_size; i++)
    c.body[i] = b[i];

  fx.ui_events.enqueue(std::move(c));
}

uint32_t port_index(SuilController controller, const char* symbol)
{
  auto& p = score::GUIAppContext().applicationPlugin<Media::ApplicationPlugin>();
  LV2EffectModel& fx = (LV2EffectModel&)controller;
  auto n = lilv_new_uri(p.lilv.me, symbol);
  auto port = lilv_plugin_get_port_by_symbol(fx.plugin, n);
  lilv_node_free(n);
  return port ? lilv_port_get_index(fx.plugin, port) : LV2UI_INVALID_PORT_INDEX;
}
}
#endif

namespace Media
{

ApplicationPlugin::ApplicationPlugin(const score::ApplicationContext& app)
    : score::ApplicationPlugin{app}
#if defined(LILV_SHARED)
, lv2_context{std::make_unique<LV2::GlobalContext>(64, lv2_host_context)},
    lv2_host_context
{
  lv2_context.get(), nullptr, lv2_context->features(), lilv
}
#endif
{

#if defined(HAS_VST2)
  qRegisterMetaType<vst_info>();
  qRegisterMetaTypeStreamOperators<vst_info>();
  qRegisterMetaType<std::vector<vst_info>>();
  qRegisterMetaTypeStreamOperators<std::vector<vst_info>>();
#endif

#if defined(LILV_SHARED) // TODO instead add a proper preprocessor macro that
                         // also works in static case
  static int argc{0};
  static char** argv{nullptr};
  suil_init(&argc, &argv, SUIL_ARG_NONE);
  lv2_context->loadPlugins();

  lv2_context->ui_host = suil_host_new(
        Media::LV2::on_uiMessage,
        Media::LV2::port_index,
        nullptr, nullptr);
#endif
}

void ApplicationPlugin::initialize()
{
#if defined(HAS_VST2)
  // init with the database
  QSettings s;
  auto val = s.value("Effect/KnownVST2");
  if (val.canConvert<std::vector<vst_info>>())
  {
    vst_infos = val.value<std::vector<vst_info>>();
  }

  if(vst_invalid_format)
  {
    vst_infos.clear();
    vst_invalid_format = false;
  }

  auto& set = context.settings<Media::Settings::Model>();
  con(set, &Media::Settings::Model::VstPathsChanged, this,
      &ApplicationPlugin::rescanVSTs);
  rescanVSTs(set.getVstPaths());
#endif
}

#if defined(HAS_VST2)
void ApplicationPlugin::rescanVSTs(const QStringList& paths)
{
  // 1. List all plug-ins in new paths
  QSet<QString> newPlugins;
  for (QString dir : paths)
  {
#  if defined(__APPLE__)
    {
      QDirIterator it(
          dir, QStringList{"*.vst", "*.component"}, QDir::AllEntries,
          QDirIterator::Subdirectories);

      while (it.hasNext())
        newPlugins.insert(it.next());
    }
    {
      QDirIterator it(
          dir, QStringList{"*.dylib"}, QDir::Files,
          QDirIterator::Subdirectories);
      while (it.hasNext())
      {
        auto path = it.next();
        if (!path.contains(".vst") && !path.contains(".component"))
          newPlugins.insert(path);
      }
    }
#  else
    QDirIterator it(
        dir, QStringList{Media::VST::default_filter}, QDir::Files,
        QDirIterator::Subdirectories);
    while (it.hasNext())
      newPlugins.insert(it.next());
#  endif
  }

  // 2. Remove plug-ins not in these paths
  for (auto it = vst_infos.begin(); it != vst_infos.end();)
  {
    auto new_it = newPlugins.find(it->path);
    if (new_it != newPlugins.end())
    {
      // plug-in is in both set, we ignore it
      newPlugins.erase(new_it);
      ++it;
    }
    else
    {
      it = vst_infos.erase(it);
    }
  }

  // 3. Add remaining plug-ins
  auto add_invalid = [this] (const QString& path) {
    vst_info i;
    i.path = path;
    i.prettyName = "invalid";
    i.uniqueID = -1;
    i.isSynth = false;
    i.isValid = false;
    vst_infos.push_back(i);
  };

  m_processes.clear();
  m_processes.reserve(newPlugins.size());
  int i = 0;
  for (const QString& path : newPlugins)
  {
    qDebug() << "Loading VST " << path;
    m_processes.emplace_back(path, std::make_unique<QProcess>());
    auto& p = *m_processes.back().second;
    connect(m_processes.back().second.get(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            [=,&p] (int code, QProcess::ExitStatus e) {
      QJsonDocument doc = QJsonDocument::fromJson(p.readAllStandardOutput());
      bool valid =
          e == QProcess::ExitStatus::NormalExit
       && code == 0
       && doc.isObject();

      if(valid)
      {
        const auto& obj = doc.object();
        vst_info i;
        i.path = path;
        i.uniqueID = obj["UniqueID"].toInt();
        i.isSynth = obj["Synth"].toBool();
        i.author = obj["Author"].toString();
        i.displayName = obj["PrettyName"].toString();
        i.controls = obj["Controls"].toInt();
        i.isValid = true;

        // Only way to get a separation between Kontakt 5 / Kontakt 5 (8
        // out) / Kontakt 5 (16 out),  etc...
        i.prettyName = QFileInfo(path).baseName();

        vst_modules.insert({i.uniqueID, nullptr});
        vst_infos.push_back(std::move(i));
      }
      else
      {
        add_invalid(path);
      }

      // write in the database
      QSettings{}.setValue("Effect/KnownVST2", QVariant::fromValue(vst_infos));

    });
    m_processes.back().second->start("ossia-score-vstpuppet", {path}, QProcess::ReadOnly);
    i++;
  }

}
#endif

ApplicationPlugin::~ApplicationPlugin()
{
#if defined(HAS_VST2)
  for (auto& e : vst_modules)
  {
    delete e.second;
  }
#endif

#if defined(LILV_SHARED)
  suil_host_free(lv2_context->ui_host);
#endif
}

GUIApplicationPlugin::GUIApplicationPlugin(
    const score::GUIApplicationContext& app)
    : score::GUIApplicationPlugin{app}
{
}

void GUIApplicationPlugin::initialize()
{
}
}
