#pragma once
#include <cstddef>

#if (!(defined(__APPLE__) || defined(_WIN32))) && __has_include(<xcb/xcb.h>)
#include <Vst3/UI/WindowContainer.hpp>

#include <ossia/detail/algorithms.hpp>
#include <ossia/detail/hash_map.hpp>

#include <QDebug>
#include <QSocketNotifier>
#include <QTimer>
#include <QWindow>

#include <pluginterfaces/gui/iplugview.h>
#include <xcb/xcb.h>

#include <dlfcn.h>

#include <memory>
#include <utility>
#include <vector>

namespace vst3
{
namespace Linux = Steinberg::Linux;

struct SocketPair
{
  SocketPair(int fd)
      : read{fd, QSocketNotifier::Read}
      , write{fd, QSocketNotifier::Write}
      , error{fd, QSocketNotifier::Exception}
  {
  }
  template <typename F>
  void connect(QObject* sink, F f)
  {
    QObject::connect(&read, &QSocketNotifier::activated, sink, f);
    QObject::connect(&write, &QSocketNotifier::activated, sink, f);
    QObject::connect(&error, &QSocketNotifier::activated, sink, f);

    read.setEnabled(true);
    write.setEnabled(true);
    error.setEnabled(true);
  }
  QSocketNotifier read;
  QSocketNotifier write;
  QSocketNotifier error;
};

struct XcbConnection
{
  void* xcb{};
  xcb_connection_t* connection{};
  int fd{-1};

  std::unique_ptr<SocketPair> notifiers;
  XcbConnection()
      : xcb{dlopen("libxcb.so.1", RTLD_LOCAL)}
  {
    if(!xcb)
      return;

    auto xcb_connect
        = reinterpret_cast<decltype(&::xcb_connect)>(dlsym(xcb, "xcb_connect"));
    if(!xcb_connect)
      return;
    auto xcb_get_file_descriptor
        = reinterpret_cast<decltype(&::xcb_get_file_descriptor)>(
            dlsym(xcb, "xcb_get_file_descriptor"));
    if(!xcb_get_file_descriptor)
      return;

    connection = xcb_connect(nullptr, nullptr);
    if(!connection)
      return;

    fd = xcb_get_file_descriptor(connection);
    if(fd < 0)
      return;

    notifiers = std::make_unique<SocketPair>(fd);
  }

  ~XcbConnection()
  {
    if(!xcb)
      return;
    if(!connection)
      return;

    auto xcb_disconnect
        = reinterpret_cast<decltype(&::xcb_disconnect)>(dlsym(xcb, "xcb_disconnect"));
    if(!xcb_disconnect)
      return;
    xcb_disconnect(connection);
  }
};

struct SocketHandler
{
  Linux::IEventHandler* handler{};
  QSocketNotifier* r_notifier{};
  QSocketNotifier* w_notifier{};
  QSocketNotifier* e_notifier{};
  int fd{};
};

class GlobalSocketHandlers : public QObject
{
  XcbConnection xcb;

public:
  ossia::hash_map<int, std::vector<Linux::IEventHandler*>> handlers;
  GlobalSocketHandlers()
  {
    if(xcb.notifiers)
      xcb.notifiers->connect(this, [this] { on_xcb(); });
  }

  void on_xcb()
  {
    if(auto it = handlers.find(xcb.fd); it != handlers.end())
    {
      for(auto& hdl : it->second)
        hdl->onFDIsSet(xcb.fd);
    }
  }

  void registerHandler(Linux::IEventHandler* handler, int fd)
  {
    handlers[fd].push_back(handler);
  }
  void unregisterHandler(Linux::IEventHandler* handler, int fd)
  {
    if(auto it = handlers.find(fd); it != handlers.end())
    {
      ossia::remove_erase(it->second, handler);
      if(it->second.empty())
        handlers.erase(it);
    }
  }
  static GlobalSocketHandlers& instance()
  {
    static GlobalSocketHandlers handlers;
    return handlers;
  }
};
class PlugFrame final
    : virtual public Steinberg::IPlugFrame
    , virtual public Steinberg::Linux::IRunLoop
{
public:
  using TUID = Steinberg::TUID;
  using FUID = Steinberg::FUID;
  using uint32 = Steinberg::uint32;
  using tresult = Steinberg::tresult;

  QObject internalContextObject;

  std::vector<std::pair<Linux::ITimerHandler*, QTimer*>> timers;

  std::vector<SocketHandler> handlers;

  tresult queryInterface(const TUID _iid, void** obj) override
  {
    using namespace Steinberg;
    if(FUnknownPrivate::iidEqual(_iid, FUnknown::iid))
    {
      *obj = this;
      return kResultOk;
    }
    if(FUnknownPrivate::iidEqual(_iid, IPlugFrame::iid))
    {
      *obj = this;
      return kResultOk;
    }
    if(FUnknownPrivate::iidEqual(_iid, Linux::IRunLoop::iid))
    {
      *obj = static_cast<Linux::IRunLoop*>(this);
      return kResultOk;
    }
    *obj = nullptr;
    return kNoInterface;
  }

  uint32 addRef() override { return 1; }
  uint32 release() override { return 1; }

  tresult registerEventHandler(Linux::IEventHandler* handler, Linux::FileDescriptor fd)
      SMTG_OVERRIDE
  {
    using namespace Steinberg;
    if(!handler)
      return kInvalidArgument;
    auto existing = ossia::find_if(handlers, [=](auto p) { return p.fd == fd; });

    if(existing != handlers.end())
      return kInvalidArgument;

    auto readnotifier = new QSocketNotifier{fd, QSocketNotifier::Read};
    readnotifier->setEnabled(true);
    readnotifier->setParent(&internalContextObject);
    auto on_fd = [=] { handler->onFDIsSet(fd); };
    QObject::connect(
        readnotifier, &QSocketNotifier::activated, &internalContextObject, on_fd);

    auto writenotifier = new QSocketNotifier{fd, QSocketNotifier::Write};
    writenotifier->setEnabled(true);
    writenotifier->setParent(&internalContextObject);
    QObject::connect(
        writenotifier, &QSocketNotifier::activated, &internalContextObject, on_fd);

    auto errnotifier = new QSocketNotifier{fd, QSocketNotifier::Exception};
    errnotifier->setEnabled(true);
    errnotifier->setParent(&internalContextObject);
    QObject::connect(
        errnotifier, &QSocketNotifier::activated, &internalContextObject, on_fd);

    handlers.push_back(
        SocketHandler{handler, readnotifier, writenotifier, errnotifier, fd});
    GlobalSocketHandlers::instance().registerHandler(handler, fd);

    return kResultTrue;
  }

  tresult unregisterEventHandler(Linux::IEventHandler* handler) SMTG_OVERRIDE
  {
    using namespace Steinberg;
    if(!handler)
    {
      return kInvalidArgument;
    }

    tresult res{kResultFalse};
    for(auto it = handlers.begin(); it != handlers.end();)
    {
      if(it->handler == handler)
      {
        GlobalSocketHandlers::instance().unregisterHandler(handler, it->fd);
        delete it->r_notifier;
        delete it->w_notifier;
        delete it->e_notifier;
        it = handlers.erase(it);
        res = kResultTrue;
      }
      else
      {
        ++it;
      }
    }

    return res;
  }

  tresult registerTimer(
      Linux::ITimerHandler* handler, Linux::TimerInterval milliseconds) override
  {
    auto t = new QTimer;
    t->setParent(&internalContextObject);
    QObject::connect(t, &QTimer::timeout, [=] { handler->onTimer(); });
    t->start(milliseconds);
    timers.push_back({handler, t});
    return Steinberg::kResultOk;
  }

  tresult unregisterTimer(Linux::ITimerHandler* handler) override
  {
    auto t = ossia::find_if(timers, [=](auto& p1) { return p1.first == handler; });
    if(t != timers.end())
    {
      delete t->second;
      timers.erase(t);
    }
    return Steinberg::kResultOk;
  }

  QDialog* w;
  WindowContainer wc;
  explicit PlugFrame(QDialog& w, WindowContainer wc)
      : w{&w}
      , wc{wc}
  {
  }

  void cleanup()
  {
    for(auto timer : timers)
      delete timer.second;
    timers.clear();
    for(auto handler : handlers)
    {
      GlobalSocketHandlers::instance().unregisterHandler(handler.handler, handler.fd);
      delete handler.r_notifier;
      delete handler.w_notifier;
      delete handler.e_notifier;
    }
    handlers.clear();
  }

  ~PlugFrame() { cleanup(); }

  tresult resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
  {
    wc.setSizeFromVst(*view, *newSize, *w);
    return Steinberg::kResultOk;
  }
};

}
#endif
