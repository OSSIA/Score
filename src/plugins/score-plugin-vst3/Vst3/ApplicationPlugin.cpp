#include <Vst3/ApplicationPlugin.hpp>

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/tools/Bind.hpp>

#include <ossia/detail/math.hpp>
#include <ossia/detail/algorithms.hpp>

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>

#include <wobjectimpl.h>

#include <sstream>

W_OBJECT_IMPL(vst3::ApplicationPlugin)

SCORE_SERALIZE_DATASTREAM_DEFINE(vst3::AvailablePlugin)
SCORE_SERALIZE_DATASTREAM_DEFINE(std::vector<vst3::AvailablePlugin>)

Q_DECLARE_METATYPE(vst3::AvailablePlugin)
W_REGISTER_ARGTYPE(vst3::AvailablePlugin)
Q_DECLARE_METATYPE(std::vector<vst3::AvailablePlugin>)
W_REGISTER_ARGTYPE(std::vector<vst3::AvailablePlugin>)

template <>
void DataStreamReader::read<VST3::Hosting::ClassInfo>(
    const VST3::Hosting::ClassInfo& pp)
{
  auto& p = const_cast<VST3::Hosting::ClassInfo&>(pp);
  auto& d = p.get();
  m_stream << d.classID.toString() << d.cardinality << d.category << d.name
           << d.vendor << d.version << d.sdkVersion << d.subCategories
           << (const uint32_t&)d.classFlags;
}
template <>
void DataStreamWriter::write<VST3::Hosting::ClassInfo>(
    VST3::Hosting::ClassInfo& p)
{
  auto& d = p.get();
  std::string clsid;
  m_stream >> clsid >> d.cardinality >> d.category >> d.name >> d.vendor
      >> d.version >> d.sdkVersion >> d.subCategories
      >> (uint32_t&)d.classFlags;
  if(auto id = VST3::UID::fromString(clsid))
    d.classID = *id;
  else
    qDebug() << "Invalid VST3 UID:" << clsid.c_str();
}

template <>
void DataStreamReader::read<vst3::AvailablePlugin>(
    const vst3::AvailablePlugin& p)
{
  m_stream << p.path << p.name << p.classInfo << p.isValid;
}
template <>
void DataStreamWriter::write<vst3::AvailablePlugin>(vst3::AvailablePlugin& p)
{
  m_stream >> p.path >> p.name >> p.classInfo >> p.isValid;
}
namespace vst3
{
namespace
{
#if defined(__APPLE__)
static const QStringList default_paths = {"/Library/Audio/Plug-Ins/VST3"};
static const constexpr auto default_filter = "*.vst3";
static const constexpr auto default_format = QDir::Dirs;
#elif defined(__linux__)
static const QStringList default_paths = {QStringLiteral("/usr/lib/vst3"), QStringLiteral("/usr/lib64/vst3")};
static const constexpr auto default_filter = "*.vst3";
static const constexpr auto default_format = QDir::Dirs;
#elif defined(_WIN32)
static const QStringList default_paths = {"C:\\Program Files\\Common Files\\VST3"};
static const constexpr auto default_filter = "*.vst3";
static const constexpr auto default_format = QDir::Files;
#else
static const constexpr auto default_paths = {QStringLiteral("/usr/lib/vst3")};
static const constexpr auto default_filter = "";
#endif
}
ApplicationPlugin::ApplicationPlugin(const score::ApplicationContext& ctx)
    : score::ApplicationPlugin{ctx}
    , m_wsServer("vst3-notification-server", QWebSocketServer::NonSecureMode)
{
  qRegisterMetaType<AvailablePlugin>();
  qRegisterMetaType<std::vector<AvailablePlugin>>();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  qRegisterMetaTypeStreamOperators<AvailablePlugin>();
  qRegisterMetaTypeStreamOperators<std::vector<AvailablePlugin>>();
#endif

  m_wsServer.listen({}, 37588);
  con(m_wsServer, &QWebSocketServer::newConnection, this, [this] {
    QWebSocket* ws = m_wsServer.nextPendingConnection();
    if (!ws)
      return;

    connect(
        ws, &QWebSocket::textMessageReceived, this, [=](const QString& txt) {
          processIncomingMessage(txt);
          ws->deleteLater();
        });
  });
}

void ApplicationPlugin::initialize()
{
  // init with the database
  QSettings s;
  auto val = s.value("Effect/KnownVST3");
  if (val.canConvert<std::vector<AvailablePlugin>>())
  {
    vst_infos = val.value<std::vector<AvailablePlugin>>();
  }

  vstChanged();

  //! TODO
  // auto& set = context.settings<Media::Settings::Model>();
  // con(set, &Media::Settings::Model::VstPathsChanged, this, &ApplicationPlugin::rescanVSTs);
  //

  if (qEnvironmentVariableIsEmpty("SCORE_DISABLE_AUDIOPLUGINS"))
    rescan(default_paths);
}

void ApplicationPlugin::rescan(const QStringList& paths)
{
  // 1. List all plug-ins in new paths
  QStringList exploredPaths;
  QSet<QString> newPlugins;
  for (const QString& dir : paths)
  {
    auto canonical_path = QDir{dir}.canonicalPath();
    if(exploredPaths.contains(canonical_path))
      continue;

    exploredPaths.push_back(canonical_path);

    QDirIterator it(
        dir,
        QStringList{default_filter},
        default_format,
        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (it.hasNext())
      newPlugins.insert(it.next());
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

  vstChanged();

  // 3. Add remaining plug-ins
  m_processes.clear();
  m_processes.reserve(newPlugins.size());
  int i = 0;
  for (const QString& path : newPlugins)
  {
    auto proc = std::make_unique<QProcess>();

#if defined(__APPLE__)
    {
      QString bundle_vstpuppet = qApp->applicationDirPath() + "/ossia-score-vst3puppet.app/Contents/MacOS/ossia-score-vst3puppet";
      if (QFile::exists(bundle_vstpuppet))
        proc->setProgram(bundle_vstpuppet);
      else
        proc->setProgram(
            qApp->applicationDirPath() + "/ossia-score-vst3puppet");
    }
#else
    proc->setProgram(qApp->applicationDirPath() + "/ossia-score-vst3puppet");
#endif
    proc->setArguments({path, QString::number(i)});
    connect(proc.get(), &QProcess::errorOccurred, this, [proc=proc.get(), path] {
      qDebug() << " == VST3: error => " << path;
      qDebug() << "VST3 out: " << proc->readAllStandardOutput().constData();
      qDebug() << "VST3 error: " << proc->readAllStandardError().constData();
    });
    m_processes.push_back({path, std::move(proc), false, {}});
    i++;
  }

  scanVSTsEvent();
  /*
  for(const auto& vst : newPlugins)
  {
    try {
      auto module = getModule(vst.toStdString());
      auto& v = vst_infos.emplace_back(AvailablePlugin{vst});

      const auto& factory = module->getFactory();
      for (const auto& class_info : factory.classInfos())
      {
        if (class_info.category() == kVstAudioEffectClass)
        {
          v.classInfo.push_back(class_info);
        }
      }

      v.isValid = v.classInfo.size() > 0;
    } catch(std::exception& e) {
      qDebug() << e.what();
    }
  }
  */
}

void ApplicationPlugin::processIncomingMessage(const QString& txt)
{
  QJsonDocument doc = QJsonDocument::fromJson(txt.toUtf8());

  if (doc.isObject())
  {
    auto obj = doc.object();
    addVST(obj["Path"].toString(), obj);
    int id = obj["Request"].toInt();

    if (ossia::valid_index(id, m_processes))
    {
      if (m_processes[id].process)
      {
        m_processes[id].process->close();
        if (m_processes[id].process->state()
            == QProcess::ProcessState::NotRunning)
        {
          m_processes[id] = {};
        }
        else
        {
          connect(
              m_processes[id].process.get(),
              qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
              this,
              [this, id] { m_processes[id] = {}; });
        }
      }
    }
    else
    {
      qDebug() << "Got invalid VST3 request ID" << id;
    }
  }
}

void ApplicationPlugin::addInvalidVST(const QString& path)
{
  AvailablePlugin i;
  i.path = path;
  i.name = "invalid";
  i.isValid = false;
  vst_infos.push_back(i);

  // write in the database
  QSettings{}.setValue("Effect/KnownVST3", QVariant::fromValue(vst_infos));

  vstChanged();
}

VST3::Hosting::ClassInfo::SubCategories
parseSubCategories(const std::string& str) noexcept
{
  std::vector<std::string> vec;
  std::stringstream stream(str);
  std::string item;
  while (std::getline(stream, item, '|'))
    vec.emplace_back(move(item));
  return vec;
}

void ApplicationPlugin::addVST(const QString& path, const QJsonObject& obj)
{
  AvailablePlugin i;
  i.path = path;
  i.name = obj["Name"].toString();
  i.isValid = true;

  const auto& classes = obj["Classes"].toArray();
  i.classInfo.reserve(classes.size());

  for (const QJsonValue& v : classes)
  {
    const QJsonObject& obj = v.toObject();
    const auto uid = VST3::UID::fromString(obj["UID"].toString().toStdString());
    if(!uid)
      continue;

    i.classInfo.resize(i.classInfo.size() + 1);
    VST3::Hosting::ClassInfo& cls = i.classInfo.back();

    cls.get().classID = *uid;
    cls.get().cardinality = obj["Cardinality"].toInt();
    cls.get().category = obj["Category"].toString().toStdString();
    cls.get().name = obj["Name"].toString().toStdString();
    cls.get().vendor = obj["Vendor"].toString().toStdString();
    cls.get().version = obj["Version"].toString().toStdString();
    cls.get().sdkVersion = obj["SDKVersion"].toString().toStdString();
    cls.get().subCategories
        = parseSubCategories(obj["Subcategories"].toString().toStdString());
    cls.get().classFlags = obj["Version"].toDouble();
  }

  if(i.classInfo.empty())
    return;

  vst_infos.push_back(std::move(i));

  // write in the database
  QSettings{}.setValue("Effect/KnownVST3", QVariant::fromValue(vst_infos));

  vstChanged();
}

VST3::Hosting::Module::Ptr
ApplicationPlugin::getModule(const std::string& path)
{
  std::string err;
  auto it = modules.find(path);
  if (it != modules.end())
  {
    return it->second;
  }
  else
  {
    auto module = VST3::Hosting::Module::create(path, err);

    if (!module)
      throw std::runtime_error(fmt::format("Failed to load VST3 ({}) : {}", path, err));

    modules[path] = module;
    return module;
  }
}

void ApplicationPlugin::scanVSTsEvent()
{
  constexpr int max_in_flight = 8;
  int in_flight = 0;

  for (auto& proc : m_processes)
  {
    // Already scanned processes
    if (!proc.process)
      continue;

    if (!proc.scanning)
    {
      proc.process->start(QProcess::ReadOnly);
      proc.scanning = true;
      proc.timer.start();
    }
    else
    {
      if (proc.timer.elapsed() > 10000)
      {
        addInvalidVST(proc.path);
        proc.process.reset();

        in_flight--;
        continue;
      }
    }

    in_flight++;

    if (in_flight == max_in_flight)
    {
      QTimer::singleShot(1000, this, &ApplicationPlugin::scanVSTsEvent);
      return;
    }
  }
}

std::pair<const AvailablePlugin*, const VST3::Hosting::ClassInfo*> ApplicationPlugin::classInfo(const VST3::UID& uid) const noexcept
{
  // OPTIMIZEME with a small id -> {plugin, class} cache
  for(auto& plug : this->vst_infos)
  {
    for(auto& cls : plug.classInfo)
    {
      if(cls.ID() == uid)
        return {&plug, &cls};
    }
  }
  return {};
}

QString ApplicationPlugin::pathForClass(const VST3::UID& uid) const noexcept
{
  // OPTIMIZEME with the same cache than above
  for(auto& plug : this->vst_infos)
  {
    for(auto& cls : plug.classInfo)
    {
      if(cls.ID() == uid)
        return plug.path;
    }
  }
  return {};
}

std::optional<VST3::UID> ApplicationPlugin::uidForPathAndClassName(
    const QString& path,
    const QString& cls) const noexcept
{
  auto it = ossia::find_if(this->vst_infos, [&] (auto& info) { return info.path == path; });
  if(it == this->vst_infos.end())
    return {};

  auto cls_it = ossia::find_if(it->classInfo, [&, n = cls.toStdString()] (auto& info) { return info.name() == n; });
  if(cls_it == it->classInfo.end())
    return {};

  return cls_it->ID();
}
}
