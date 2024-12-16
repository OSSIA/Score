#include <Vst3/UI/Linux/PlugFrame.hpp>
#include <Vst3/UI/PlugFrame.hpp>
#include <Vst3/UI/Window.hpp>
namespace vst3
{
WindowContainer createVstWindowContainer(
    Window& parentWindow, const Model& e, const score::DocumentContext& ctx)
{
  WindowContainer wc;

  Steinberg::IPlugView& view = *e.fx.view;

  Steinberg::ViewRect r;
  view.getSize(&r);
  wc.setSizeFromQt(view, r, parentWindow);

  parentWindow.show();
  wc.qwindow = parentWindow.windowHandle();
  wc.container = nullptr;

  if(!e.fx.plugFrame)
    const_cast<Model&>(e).fx.plugFrame = new PlugFrame{parentWindow, wc};
  else
  {
    e.fx.plugFrame->w = &parentWindow;
    e.fx.plugFrame->wc = wc;
  }
  wc.frame = e.fx.plugFrame;
  view.setFrame(wc.frame);
  view.attached((void*)wc.qwindow->winId(), currentPlatform());

  QTimer::singleShot(16, &parentWindow, [&, wc]() mutable {
    Steinberg::ViewRect r;
    view.getSize(&r);
    if(r.getWidth() != 0 && r.getHeight() != 0)
    {
      wc.setSizeFromQt(view, r, parentWindow);

      view.onSize(&r);
    }
  });
  //parentWindow.show();

  return wc;
}

}
