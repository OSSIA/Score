#pragma once
#include <ossia/dataflow/exec_state_facade.hpp>
#include <ossia/dataflow/execution_state.hpp>
#include <ossia/detail/algorithms.hpp>
#include <ossia/network/common/path.hpp>

#include <halp/controls.hpp>
#include <halp/meta.hpp>

namespace avnd_tools
{

// Given:
// address: /foo.*/value
//  -> /foo.1/value, /foo.2/value, etc.
// input: [1, 34, 6, 4]
// -> writes 1 on foo.1/value, 34 on foo.2/value, etc

struct PatternObject
{
  ossia::exec_state_facade ossia_state;
  std::optional<ossia::traversal::path> m_path;
  std::vector<ossia::net::node_base*> roots;
};

struct PatternSelector : halp::lineedit<"Pattern", "">
{
  void update(PatternObject& p)
  {
    current_object = &p;
    if(!p.ossia_state.impl)
      return;
    if(value.empty())
    {
      p.m_path = {};
      p.roots.clear();
      return;
    }

    p.m_path = ossia::traversal::make_path(value);
    reprocess();
  }

  void reprocess()
  {
    if(!current_object)
      return;

    auto& p = *current_object;
    ossia::execution_state& st = *p.ossia_state.impl;
    const auto& rdev = st.exec_devices();
    p.roots.clear();

    if(!p.m_path)
      return;

    for(auto& dev : rdev)
    {
      if(devices_observed.find(dev) == devices_observed.end())
      {
        devices_observed.insert(dev);
        dev->on_node_created.connect<&PatternSelector::mark_dirty>(this);
        dev->on_node_removing.connect<&PatternSelector::mark_dirty>(this);
      }
      p.roots.push_back(&dev->get_root_node());
    }

    for(auto it = devices_observed.begin(); it != devices_observed.end();)
    {
      if(!ossia::contains(rdev, *it))
        it = devices_observed.erase(it);
      else
        ++it;
    }

    ossia::traversal::apply(*p.m_path, p.roots);
    devices_dirty = false;
  }

  ~PatternSelector()
  {
    reprocess();
    for(auto* dev : devices_observed)
    {
      dev->on_node_created.disconnect<&PatternSelector::mark_dirty>(this);
      dev->on_node_removing.disconnect<&PatternSelector::mark_dirty>(this);
    }
  }

  void mark_dirty(ossia::net::node_base&) { devices_dirty = true; }

  PatternObject* current_object{};
  bool devices_dirty = false;

  ossia::flat_set<ossia::net::device_base*> devices_observed;
};

struct PatternUnfolder : PatternObject
{
  halp_meta(name, "Pattern applier")
  halp_meta(author, "ossia team")
  halp_meta(category, "Control/Data processing")
  halp_meta(description, "Send a message to all nodes matching a pattern")
  halp_meta(c_name, "avnd_pattern_apply")
  halp_meta(uuid, "44a55ee1-c2c9-43d5-a655-8eaedaff394c")
  halp_meta(manual_url, "https://ossia.io/score-docs/processes/pattern-applier.html#pattern-applier")

  struct
  {
    halp::val_port<"Input", ossia::value> input;
    PatternSelector pattern;
  } inputs;

  struct
  {

  } outputs;

  void operator()()
  {
    if(!m_path)
      return;

    auto process = [this](const std::vector<ossia::value>& vec) {
      const auto N = std::min(roots.size(), vec.size());
      for(std::size_t i = 0; i < N; i++)
      {
        if(auto p = roots[i]->get_parameter())
        {
          p->push_value(vec[i]);
        }
      }
    };

    if(auto vvec = inputs.input.value.target<std::vector<ossia::value>>())
    {
      process(*vvec);
    }
    else
    {
      process(ossia::convert<std::vector<ossia::value>>(inputs.input.value));
    }
  }
};
}
