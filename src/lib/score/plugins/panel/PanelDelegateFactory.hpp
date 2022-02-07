#pragma once
#include <score/plugins/Interface.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/panel/PanelDelegate.hpp>

namespace score
{
//! Reimplement this interface to register new panels.
class SCORE_LIB_BASE_EXPORT PanelDelegateFactory : public score::InterfaceBase
{
  SCORE_INTERFACE(PanelDelegateFactory, "8d6211f7-5244-44f9-94dd-f3e32255c43e")
public:
  static const constexpr bool ui_interface = true;
  virtual ~PanelDelegateFactory();

  //! Create an instance of a PanelDelegate. Will only be called once.
  virtual std::unique_ptr<PanelDelegate>
  make(const score::GUIApplicationContext& ctx) = 0;
};

//! All the panels are registered in this interface list.
class SCORE_LIB_BASE_EXPORT PanelDelegateFactoryList final
    : public InterfaceList<score::PanelDelegateFactory>
{
public:
  using object_type = PanelDelegate;
  ~PanelDelegateFactoryList();
};
}
