#pragma once
#include <ossia/detail/logger.hpp>

#include <Engine/Node/PdNode.hpp>
#include <exprtk.hpp>
#include <numeric>
namespace Nodes
{
template <typename State>
bool updateExpr(State& self, const std::string& expr)
{
  if (expr != self.cur_expr_txt)
  {
    self.cur_expr_txt = expr;
    self.ok = self.parser.compile(self.cur_expr_txt, self.expr);
    if (!self.ok)
    {
      ossia::logger().error("Error while parsing: {}", self.parser.error());
    }
  }

  return self.ok;
}

namespace MathGenerator
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Value Generator";
    static const constexpr auto objectKey = "MathGenerator";
    static const constexpr auto category = "Control";
    static const constexpr auto author = "ossia score, ExprTK (Arash Partow)";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const constexpr auto kind = Process::ProcessCategory::Generator;
    static const constexpr auto description = "Generate a signal from a math expression.\n"
                                              "See the documentation at http://www.partow.net/programming/exprtk";
    static const constexpr auto uuid
        = make_uuid("d757bd0d-c0a1-4aec-bf72-945b722ab85b");

    static const constexpr value_out value_outs[]{"out"};

    static const constexpr auto controls = std::make_tuple(
        Control::LineEdit("Expression (ExprTK)", "cos(t) + log(pos * 1 / dt)"),
        Control::FloatSlider("Param (a)", 0., 1., 0.5),
        Control::FloatSlider("Param (b)", 0., 1., 0.5),
        Control::FloatSlider("Param (c)", 0., 1., 0.5));
  };
  struct State
  {
    State()
    {
      syms.add_variable("t", cur_time);
      syms.add_variable("dt", cur_deltatime);
      syms.add_variable("pos", cur_pos);
      syms.add_variable("a", p1);
      syms.add_variable("b", p2);
      syms.add_variable("c", p3);
      syms.add_constants();

      expr.register_symbol_table(syms);
    }
    double cur_time{};
    double cur_deltatime{};
    double cur_pos{};
    double p1{}, p2{}, p3{};
    exprtk::symbol_table<double> syms;
    exprtk::expression<double> expr;
    exprtk::parser<double> parser;
    std::string cur_expr_txt;
    bool ok = false;
  };

  using control_policy = ossia::safe_nodes::last_tick;
  static void
  run(const std::string& expr,
      float a,
      float b,
      float c,
      ossia::value_port& output,
      ossia::token_request tk,
      ossia::exec_state_facade st,
      State& self)
  {
    if (!updateExpr(self, expr))
      return;

    self.cur_time = tk.date.impl;
    self.cur_deltatime = tk.date.impl - tk.prev_date.impl;
    self.cur_pos = tk.position;
    self.p1 = a;
    self.p2 = b;
    self.p3 = c;

    auto res = self.expr.value();
    output.write_value(res, tk.tick_start());
  }
};
}

namespace MathAudioGenerator
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Audio Generator";
    static const constexpr auto objectKey = "MathAudioGenerator";
    static const constexpr auto category = "Audio";
    static const constexpr auto author = "ossia score, ExprTK (Arash Partow)";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const constexpr auto kind = Process::ProcessCategory::Generator;
    static const constexpr auto description = "Generate an audio signal from a math expression.\n"
                                              "See the documentation at http://www.partow.net/programming/exprtk";
    static const constexpr auto uuid
        = make_uuid("eae294b3-afeb-4fba-bbe4-337998d3748a");

    static const constexpr audio_out audio_outs[]{"out"};

    static const constexpr auto controls = std::make_tuple(
        Control::LineEdit(
            "Expression (ExprTK)", "a * cos( 2 * pi * t * 440 * b / fs )"),
        Control::FloatSlider("Param (a)", 0., 1., 0.5),
        Control::FloatSlider("Param (b)", 0., 1., 0.5),
        Control::FloatSlider("Param (c)", 0., 1., 0.5));
  };

  struct State
  {
    State()
    {
      syms.add_variable("t", cur_time);
      syms.add_variable("a", p1);
      syms.add_variable("b", p2);
      syms.add_variable("c", p3);
      syms.add_variable("fs", fs);
      syms.add_constants();

      expr.register_symbol_table(syms);
    }
    double cur_time{};
    double p1{}, p2{}, p3{};
    double fs{44100};
    exprtk::symbol_table<double> syms;
    exprtk::expression<double> expr;
    exprtk::parser<double> parser;
    std::string cur_expr_txt;
    bool ok = false;
  };

  using control_policy = ossia::safe_nodes::last_tick;
  static void
  run(const std::string& expr,
      float a,
      float b,
      float c,
      ossia::audio_port& output,
      ossia::token_request tk,
      ossia::exec_state_facade st,
      State& self)
  {
    if (tk.date > tk.prev_date)
    {
      auto count = tk.date - tk.prev_date;
      if (!updateExpr(self, expr))
        return;

      output.samples.resize(1);
      auto& cur = output.samples[0];
      if ((int64_t)cur.size() < tk.offset + count)
        cur.resize(tk.offset + count);

      self.p1 = a;
      self.p2 = b;
      self.p3 = c;
      self.fs = st.sampleRate();
      for (int64_t i = 0; i < count; i++)
      {
        self.cur_time = tk.prev_date + i;
        cur[tk.offset + i] = self.expr.value();
      }
    }
  }
};
}
}
