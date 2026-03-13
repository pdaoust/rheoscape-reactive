#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <regex>
#include <algorithm>

// ============================================================================
// Template Type Parser
// ============================================================================

struct TypeNode {
  std::string name;
  std::vector<TypeNode> args;
  std::string suffix;  // Text after closing '>' (e.g., "::LapScanner").
};

// Trim whitespace from both ends.
static std::string trim(std::string_view sv) {
  auto start = sv.find_first_not_of(" \t\n\r");
  if (start == std::string_view::npos) return "";
  auto end = sv.find_last_not_of(" \t\n\r");
  return std::string(sv.substr(start, end - start + 1));
}

// Parse a type string into a tree of TypeNode.
// Handles balanced <>, (), and splits on ',' at depth 0.
static TypeNode parse_type(const std::string& raw_input) {
  std::string input = trim(raw_input);
  TypeNode node;

  // Find the first '<' that starts template args.
  int depth_angle = 0;
  int depth_paren = 0;
  size_t template_start = std::string::npos;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (c == '(') { depth_paren++; }
    else if (c == ')') { depth_paren--; }
    else if (c == '<' && depth_paren == 0) {
      if (depth_angle == 0) {
        template_start = i;
      }
      depth_angle++;
    }
    else if (c == '>' && depth_paren == 0) {
      depth_angle--;
    }
  }

  if (template_start == std::string::npos) {
    node.name = input;
    return node;
  }

  node.name = trim(input.substr(0, template_start));

  // Find the matching closing >.
  size_t template_end = std::string::npos;
  depth_angle = 0;
  depth_paren = 0;
  for (size_t i = template_start; i < input.size(); ++i) {
    char c = input[i];
    if (c == '(') { depth_paren++; }
    else if (c == ')') { depth_paren--; }
    else if (c == '<' && depth_paren == 0) { depth_angle++; }
    else if (c == '>' && depth_paren == 0) {
      depth_angle--;
      if (depth_angle == 0) {
        template_end = i;
        break;
      }
    }
  }

  if (template_end == std::string::npos) {
    node.name = input;
    return node;
  }

  std::string args_str = input.substr(template_start + 1, template_end - template_start - 1);

  // Parse suffix: skip balanced (...) after '>', then capture remaining text.
  size_t suffix_pos = template_end + 1;
  if (suffix_pos < input.size() && input[suffix_pos] == '(') {
    int pdepth = 0;
    for (size_t i = suffix_pos; i < input.size(); ++i) {
      if (input[i] == '(') pdepth++;
      else if (input[i] == ')') {
        pdepth--;
        if (pdepth == 0) {
          suffix_pos = i + 1;
          break;
        }
      }
    }
  }
  if (suffix_pos < input.size()) {
    node.suffix = trim(input.substr(suffix_pos));
  }

  // Split on ',' at depth 0.
  depth_angle = 0;
  depth_paren = 0;
  size_t arg_start = 0;
  for (size_t i = 0; i < args_str.size(); ++i) {
    char c = args_str[i];
    if (c == '(') { depth_paren++; }
    else if (c == ')') { depth_paren--; }
    else if (c == '<' && depth_paren == 0) { depth_angle++; }
    else if (c == '>' && depth_paren == 0) { depth_angle--; }
    else if (c == ',' && depth_angle == 0 && depth_paren == 0) {
      node.args.push_back(parse_type(args_str.substr(arg_start, i - arg_start)));
      arg_start = i + 1;
    }
  }
  auto last = trim(args_str.substr(arg_start));
  if (!last.empty()) {
    node.args.push_back(parse_type(last));
  }

  return node;
}

// ============================================================================
// Namespace Stripper
// ============================================================================

static const std::vector<std::string> NAMESPACE_PREFIXES = {
  "rheoscape::operators::detail::",
  "rheoscape::states::detail::",
  "rheoscape::sources::detail::",
  "rheoscape::sinks::detail::",
  "rheoscape::ui::detail::",
  "rheoscape::operators::",
  "rheoscape::states::",
  "rheoscape::sources::",
  "rheoscape::sinks::",
  "rheoscape::ui::",
  "rheoscape::types::",
  "rheoscape::",
  // Partial namespace prefixes (when rheoscape:: was already stripped).
  "operators::detail::",
  "states::detail::",
  "sources::detail::",
  "sinks::detail::",
  "ui::detail::",
  "operators::",
  "states::",
  "sources::",
  "sinks::",
  "detail::",
};

static std::string strip_namespaces(const std::string& input) {
  std::string result = input;
  for (const auto& prefix : NAMESPACE_PREFIXES) {
    size_t pos = 0;
    while ((pos = result.find(prefix, pos)) != std::string::npos) {
      result.erase(pos, prefix.size());
    }
  }
  return result;
}

// ============================================================================
// Operator Name Mapping
// ============================================================================

enum class OperatorKind {
  // First template param is an upstream source.
  UNARY,
  // All template params are upstream sources (variadic).
  MULTI_SOURCE,
  // First two params are sources.
  BINARY_SOURCE,
  // Scan-like: first param upstream, second state type, rest args.
  SCAN_LIKE,
  // Source (no upstream).
  SOURCE,
  // PipeFactory (no upstream).
  PIPE,
  // Sink.
  SINK,
};

struct OperatorInfo {
  std::string display_name;
  OperatorKind kind;
};

static const std::unordered_map<std::string, OperatorInfo>& get_operator_map() {
  static const std::unordered_map<std::string, OperatorInfo> map = {
    // SourceBinder operators.
    {"MapSourceBinder", {"map", OperatorKind::UNARY}},
    {"FilterSourceBinder", {"filter", OperatorKind::UNARY}},
    {"FilterMapSourceBinder", {"filter_map", OperatorKind::UNARY}},
    {"FlatMapSourceBinder", {"flat_map", OperatorKind::UNARY}},
    {"TakeSourceBinder", {"take", OperatorKind::UNARY}},
    {"TakeWhileSourceBinder", {"take_while", OperatorKind::UNARY}},
    {"InspectSourceBinder", {"inspect", OperatorKind::UNARY}},
    {"ThrottleSourceBinder", {"throttle", OperatorKind::UNARY}},
    {"DebounceSourceBinder", {"debounce", OperatorKind::UNARY}},
    {"DedupeSourceBinder", {"dedupe", OperatorKind::UNARY}},
    {"CountSourceBinder", {"count", OperatorKind::UNARY}},
    {"TagCountSourceBinder", {"count", OperatorKind::UNARY}},
    {"LatchSourceBinder", {"latch", OperatorKind::UNARY}},
    {"TimedLatchSourceBinder", {"timed_latch", OperatorKind::UNARY}},
    {"StartWhenSourceBinder", {"start_when", OperatorKind::BINARY_SOURCE}},
    {"SampleSourceBinder", {"sample", OperatorKind::BINARY_SOURCE}},
    {"SettleSourceBinder", {"settle", OperatorKind::UNARY}},
    {"TeeSourceBinder", {"tee", OperatorKind::UNARY}},
    {"CacheSourceBinder", {"cache", OperatorKind::SOURCE}},
    {"SharedSourceBinder", {"share", OperatorKind::SOURCE}},
    {"CombineSourceBinder", {"combine", OperatorKind::MULTI_SOURCE}},
    {"ConcatSourceBinder", {"concat", OperatorKind::BINARY_SOURCE}},
    {"ConcatEndableSourceBinder", {"concat", OperatorKind::BINARY_SOURCE}},
    {"MergeSourceBinder", {"merge", OperatorKind::MULTI_SOURCE}},
    {"MergeMixedSourceBinder", {"merge", OperatorKind::BINARY_SOURCE}},
    {"ChooseSourceBinder", {"choose", OperatorKind::BINARY_SOURCE}},
    {"ScanWithInitialSourceBinder", {"scan", OperatorKind::SCAN_LIKE}},
    {"ScanSourceBinder", {"scan", OperatorKind::UNARY}},
    {"IntervalSourceBinder", {"interval", OperatorKind::BINARY_SOURCE}},

    // PipeFactory operators.
    {"MapPipeFactory", {"map", OperatorKind::PIPE}},
    {"FilterPipeFactory", {"filter", OperatorKind::PIPE}},
    {"FilterMapPipeFactory", {"filter_map", OperatorKind::PIPE}},
    {"FlatMapPipeFactory", {"flat_map", OperatorKind::PIPE}},
    {"TakePipeFactory", {"take", OperatorKind::PIPE}},
    {"TakeWhilePipeFactory", {"take_while", OperatorKind::PIPE}},
    {"TakeUntilPipeFactory", {"take_while", OperatorKind::PIPE}},
    {"InspectPipeFactory", {"inspect", OperatorKind::PIPE}},
    {"ThrottlePipeFactory", {"throttle", OperatorKind::PIPE}},
    {"DebouncePipeFactory", {"debounce", OperatorKind::PIPE}},
    {"DedupePipeFactory", {"dedupe", OperatorKind::PIPE}},
    {"CountPipeFactory", {"count", OperatorKind::PIPE}},
    {"TagCountPipeFactory", {"count", OperatorKind::PIPE}},
    {"LatchPipeFactory", {"latch", OperatorKind::PIPE}},
    {"TimedLatchPipeFactory", {"timed_latch", OperatorKind::PIPE}},
    {"StartWhenPipeFactory", {"start_when", OperatorKind::PIPE}},
    {"SamplePipeFactory", {"sample", OperatorKind::PIPE}},
    {"SampleEveryPipeFactory", {"sample_every", OperatorKind::PIPE}},
    {"SettlePipeFactory", {"settle", OperatorKind::PIPE}},
    {"TeePipeFactory", {"tee", OperatorKind::PIPE}},
    {"CachePipeFactory", {"cache", OperatorKind::PIPE}},
    {"SharePipeFactory", {"share", OperatorKind::PIPE}},
    {"CombineWithPipeFactory", {"combine_with", OperatorKind::PIPE}},
    {"ConcatPipeFactory", {"concat", OperatorKind::PIPE}},
    {"MergePipeFactory", {"merge", OperatorKind::PIPE}},
    {"MergeMixedPipeFactory", {"merge", OperatorKind::PIPE}},
    {"ChooseAmongPipeFactory", {"choose", OperatorKind::PIPE}},
    {"ScanWithInitialPipeFactory", {"scan", OperatorKind::PIPE}},
    {"ScanPipeFactory", {"scan", OperatorKind::PIPE}},
    {"IntervalPipeFactory", {"interval", OperatorKind::PIPE}},
    {"NormalizePipeFactory", {"normalize", OperatorKind::PIPE}},
    {"TimestampPipeFactory", {"timestamp", OperatorKind::PIPE}},
    {"StopwatchWhenPipeFactory", {"stopwatch_when", OperatorKind::PIPE}},
    {"StopwatchChangesPipeFactory", {"stopwatch_changes", OperatorKind::PIPE}},
    {"WavePipeFactory", {"wave", OperatorKind::PIPE}},
    {"SineWavePipeFactory", {"sine_wave", OperatorKind::PIPE}},
    {"SawtoothWavePipeFactory", {"sawtooth_wave", OperatorKind::PIPE}},
    {"TriangleWavePipeFactory", {"triangle_wave", OperatorKind::PIPE}},
    {"SquareWavePipeFactory", {"square_wave", OperatorKind::PIPE}},
    {"PwmWavePipeFactory", {"pwm_wave", OperatorKind::PIPE}},
    {"BangBangPipeFactory", {"bang_bang", OperatorKind::PIPE}},
    {"EmaPipeFactory", {"ema", OperatorKind::PIPE}},
    {"EmaPipeFactoryScalar", {"ema", OperatorKind::PIPE}},
    {"QuadratureEncodePipeFactory", {"quadrature_encode", OperatorKind::PIPE}},
    {"ToggleOnPipeFactory", {"toggle", OperatorKind::PIPE}},
    {"ApplyTogglePipeFactory", {"toggle", OperatorKind::PIPE}},
    {"PidPipeFactory", {"pid", OperatorKind::PIPE}},
    {"PidPipeFactoryNoConverter", {"pid", OperatorKind::PIPE}},
    {"PidDetailedPipeFactory", {"pid_detailed", OperatorKind::PIPE}},
    {"PidDetailedPipeFactoryNoConverter", {"pid_detailed", OperatorKind::PIPE}},
    {"LiftPipeFactory", {"lift", OperatorKind::PIPE}},
    {"RelayAutotunePipeFactory", {"relay_autotune", OperatorKind::PIPE}},
    {"LogErrorsPipeFactory", {"log_errors", OperatorKind::PIPE}},
    {"SplitAndCombinePipeFactory", {"split_and_combine", OperatorKind::PIPE}},
    {"SplitAndCombineSimplePipeFactory", {"split_and_combine", OperatorKind::PIPE}},
    {"ForeachSinkFactory", {"foreach", OperatorKind::SINK}},

    // Source binders.
    {"constant_source_binder", {"constant", OperatorKind::SOURCE}},
    {"empty_source_binder", {"empty", OperatorKind::SOURCE}},
    {"done_source_binder", {"done", OperatorKind::SOURCE}},
    {"from_clock_source_binder", {"from_clock", OperatorKind::SOURCE}},
    {"from_iterator_source_binder", {"from_iterator", OperatorKind::SOURCE}},
    {"from_observable_source_binder", {"from_observable", OperatorKind::SOURCE}},
    {"sequence_source_binder", {"sequence", OperatorKind::SOURCE}},
    {"emitter_source_binder", {"emitter", OperatorKind::SOURCE}},
    {"analog_pin_source_binder", {"analog_pin", OperatorKind::SOURCE}},
    {"digital_pin_source_binder", {"digital_pin", OperatorKind::SOURCE}},
    {"digital_pin_interrupt_source_binder", {"digital_pin_interrupt", OperatorKind::SOURCE}},
    {"bh1750_source_binder", {"bh1750", OperatorKind::SOURCE}},
    {"ds18b20_source_binder", {"ds18b20", OperatorKind::SOURCE}},
    {"sht2x_source_binder", {"sht2x", OperatorKind::SOURCE}},
    {"memory_state_source_binder", {"MemoryState", OperatorKind::SOURCE}},
    {"eeprom_state_source_binder", {"EepromState", OperatorKind::SOURCE}},
    {"widget_event_source_binder", {"widget_event", OperatorKind::SOURCE}},
    {"knn_interpolate_pipe_factory", {"knn_interpolate", OperatorKind::PIPE}},
  };
  return map;
}

// ============================================================================
// Simplify Lambda Names
// ============================================================================

// Shorten lambda names like "main()::<lambda(int, float)>" to "lambda(int, float)".
// Also handles standalone "<lambda(...)>" (after namespace stripping).
// Handles nested parens in lambda args (e.g., "<lambda(std::tuple<A, B>)>").
static std::string simplify_lambda(const std::string& name) {
  // Try "::<lambda(" first (qualified lambda).
  std::string marker = "::<lambda(";
  auto mpos = name.find(marker);
  if (mpos == std::string::npos) {
    // Try standalone "<lambda(" (e.g., the whole name is "<lambda(...)>").
    marker = "<lambda(";
    if (name.substr(0, marker.size()) == marker) {
      mpos = 0;
    } else {
      return name;
    }
  }

  size_t args_start = mpos + marker.size();
  // Find matching ')>' by tracking paren depth.
  int depth = 1;
  size_t i = args_start;
  for (; i < name.size() && depth > 0; ++i) {
    if (name[i] == '(') depth++;
    else if (name[i] == ')') depth--;
  }
  // i is now one past the matching ')'.
  if (i <= name.size() && depth == 0) {
    std::string args = name.substr(args_start, i - 1 - args_start);
    return "lambda(" + args + ")";
  }
  return name;
}

// ============================================================================
// Pipeline Types and Rendering
// ============================================================================

struct Pipeline;

struct PipelineStep {
  std::string operator_name;
  std::vector<std::string> args;          // Non-source args.
  std::vector<Pipeline> sub_pipelines;    // For multi-source ops (combine, merge).
  bool is_source = false;
};

struct Pipeline {
  std::vector<PipelineStep> steps;
};

// Forward declarations.
static std::string render_pipeline(const Pipeline& pipeline);
static bool build_pipeline(const TypeNode& node, Pipeline& pipeline);

// Render a TypeNode back to a compact string.
// If simplify is true, recursively simplifies known operator types.
static std::string render_type_compact(const TypeNode& node, bool simplify = false);

// Check if a name is a known operator (by struct name or display name).
// Returns the display name if found, empty string otherwise.
static std::string find_operator_display_name(const std::string& name) {
  const auto& op_map = get_operator_map();
  // Check struct name first.
  auto it = op_map.find(name);
  if (it != op_map.end()) return it->second.display_name;
  // Check if the name itself is a known display name.
  for (const auto& [key, info] : op_map) {
    if (info.display_name == name) return name;
  }
  return "";
}

static std::string render_type_compact(const TypeNode& node, bool simplify) {
  std::string name = node.name;
  bool drop_args = false;

  if (simplify) {
    // Check the full name (possibly with ::suffix in the name field).
    std::string base_name = name;
    std::string name_suffix;
    auto colpos = name.find("::");
    if (colpos != std::string::npos) {
      base_name = name.substr(0, colpos);
      name_suffix = name.substr(colpos);
    }

    std::string display = find_operator_display_name(base_name);
    if (!display.empty()) {
      name = display + name_suffix;
      drop_args = true;
    }
  }

  std::string result = name;
  if (!node.args.empty() && !drop_args) {
    result += "<";
    for (size_t i = 0; i < node.args.size(); ++i) {
      if (i > 0) result += ", ";
      result += render_type_compact(node.args[i], simplify);
    }
    result += ">";
  }
  result += node.suffix;
  return simplify_lambda(result);
}

// Recursively build a pipeline from a TypeNode.
static bool build_pipeline(const TypeNode& node, Pipeline& pipeline) {
  auto it = get_operator_map().find(node.name);
  if (it == get_operator_map().end()) {
    return false;
  }

  const auto& info = it->second;
  PipelineStep step;
  step.operator_name = info.display_name;

  switch (info.kind) {
    case OperatorKind::UNARY: {
      if (!node.args.empty()) {
        build_pipeline(node.args[0], pipeline);
        for (size_t i = 1; i < node.args.size(); ++i) {
          step.args.push_back(render_type_compact(node.args[i], true));
        }
      }
      break;
    }
    case OperatorKind::SCAN_LIKE: {
      if (!node.args.empty()) {
        build_pipeline(node.args[0], pipeline);
      }
      if (node.args.size() > 1) {
        step.operator_name += "<" + render_type_compact(node.args[1], true) + ">";
      }
      for (size_t i = 2; i < node.args.size(); ++i) {
        step.args.push_back(render_type_compact(node.args[i], true));
      }
      break;
    }
    case OperatorKind::MULTI_SOURCE: {
      step.is_source = true;
      for (const auto& arg : node.args) {
        Pipeline sub;
        if (build_pipeline(arg, sub)) {
          step.sub_pipelines.push_back(std::move(sub));
        } else {
          // Wrap as a single-step pipeline.
          Pipeline fallback;
          PipelineStep fallback_step;
          fallback_step.operator_name = render_type_compact(arg, true);
          fallback_step.is_source = true;
          fallback.steps.push_back(std::move(fallback_step));
          step.sub_pipelines.push_back(std::move(fallback));
        }
      }
      break;
    }
    case OperatorKind::BINARY_SOURCE: {
      step.is_source = true;
      for (size_t i = 0; i < std::min(node.args.size(), size_t(2)); ++i) {
        Pipeline sub;
        if (build_pipeline(node.args[i], sub)) {
          step.sub_pipelines.push_back(std::move(sub));
        } else {
          Pipeline fallback;
          PipelineStep fallback_step;
          fallback_step.operator_name = render_type_compact(node.args[i], true);
          fallback_step.is_source = true;
          fallback.steps.push_back(std::move(fallback_step));
          step.sub_pipelines.push_back(std::move(fallback));
        }
      }
      for (size_t i = 2; i < node.args.size(); ++i) {
        step.args.push_back(render_type_compact(node.args[i], true));
      }
      break;
    }
    case OperatorKind::SOURCE: {
      step.is_source = true;
      for (const auto& arg : node.args) {
        step.args.push_back(render_type_compact(arg, true));
      }
      break;
    }
    case OperatorKind::PIPE:
    case OperatorKind::SINK: {
      for (const auto& arg : node.args) {
        step.args.push_back(render_type_compact(arg, true));
      }
      break;
    }
  }

  pipeline.steps.push_back(std::move(step));
  return true;
}

// Render a single pipeline step inline (without arrow prefix).
static std::string render_step(const PipelineStep& step) {
  std::ostringstream out;
  out << step.operator_name;

  if (!step.sub_pipelines.empty()) {
    out << "(";
    for (size_t i = 0; i < step.sub_pipelines.size(); ++i) {
      if (i > 0) out << ", ";
      out << render_pipeline(step.sub_pipelines[i]);
    }
    for (const auto& a : step.args) {
      out << ", " << a;
    }
    out << ")";
  } else if (!step.args.empty()) {
    out << "(";
    for (size_t i = 0; i < step.args.size(); ++i) {
      if (i > 0) out << ", ";
      out << step.args[i];
    }
    out << ")";
  }
  return out.str();
}

// Render pipeline as a single line (used for inline/nested pipelines).
static std::string render_pipeline(const Pipeline& pipeline) {
  if (pipeline.steps.empty()) return "<?>";

  std::ostringstream out;
  bool first = true;
  for (const auto& step : pipeline.steps) {
    if (!first) {
      out << " \xe2\x86\x92 ";
    }
    first = false;
    out << render_step(step);
  }
  return out.str();
}

// Render pipeline as multi-line with incremental indentation.
// Returns one string per line (no trailing newlines in strings).
static std::vector<std::string> render_pipeline_lines(
    const Pipeline& pipeline, int base_indent);

static std::vector<std::string> render_pipeline_lines(
    const Pipeline& pipeline, int base_indent) {
  std::vector<std::string> lines;
  if (pipeline.steps.empty()) {
    lines.push_back(std::string(base_indent, ' ') + "<?>");
    return lines;
  }

  // UTF-8 arrow.
  const std::string arrow = "\xe2\x86\x92 ";

  for (size_t step_idx = 0; step_idx < pipeline.steps.size(); ++step_idx) {
    const auto& step = pipeline.steps[step_idx];
    int step_indent = base_indent + static_cast<int>(step_idx) * 2;
    std::string pad(step_indent, ' ');
    std::string prefix = (step_idx > 0) ? (pad + arrow) : pad;

    if (!step.sub_pipelines.empty()) {
      // Multi-source op: operator_name(\n  sub1,\n  sub2\n)
      std::string header = prefix + step.operator_name + "(";
      lines.push_back(header);

      int sub_indent = step_indent + 2;
      for (size_t si = 0; si < step.sub_pipelines.size(); ++si) {
        auto sub_lines = render_pipeline_lines(step.sub_pipelines[si], sub_indent);
        for (size_t li = 0; li < sub_lines.size(); ++li) {
          if (li == sub_lines.size() - 1 && si < step.sub_pipelines.size() - 1) {
            sub_lines[li] += ",";
          }
          lines.push_back(sub_lines[li]);
        }
      }
      // Closing paren.
      std::string close = std::string(step_indent, ' ') + ")";
      lines.push_back(close);
    } else if (!step.args.empty()) {
      // Render args.
      std::string inline_render = step.operator_name + "(";
      for (size_t i = 0; i < step.args.size(); ++i) {
        if (i > 0) inline_render += ", ";
        inline_render += step.args[i];
      }
      inline_render += ")";

      if (step.operator_name.size() > 40) {
        // Break args onto next line.
        lines.push_back(prefix + step.operator_name + "(");
        int arg_indent = step_indent + 2;
        std::string arg_pad(arg_indent, ' ');
        std::string arg_line = arg_pad;
        for (size_t i = 0; i < step.args.size(); ++i) {
          if (i > 0) arg_line += ", ";
          arg_line += step.args[i];
        }
        lines.push_back(arg_line);
        lines.push_back(std::string(step_indent, ' ') + ")");
      } else {
        lines.push_back(prefix + inline_render);
      }
    } else {
      lines.push_back(prefix + step.operator_name);
    }
  }

  return lines;
}

// ============================================================================
// Full Type Simplifier
// ============================================================================

// Collapse GCC's C++03-style "> >" spacing to ">>".
static std::string collapse_angle_spacing(const std::string& input) {
  // Collapse "> >" to ">>".
  std::string result;
  result.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == ' ' && i + 1 < input.size() && input[i + 1] == '>' &&
        i > 0 && input[i - 1] == '>') {
      continue;
    }
    result += input[i];
  }
  return result;
}

static std::string simplify_type(const std::string& raw) {
  std::string stripped = collapse_angle_spacing(strip_namespaces(raw));
  TypeNode tree = parse_type(stripped);

  Pipeline pipeline;
  if (build_pipeline(tree, pipeline) && !pipeline.steps.empty()) {
    return render_pipeline(pipeline);
  }

  return render_type_compact(tree, true);
}

// Simplify a type and render as multi-line pipeline lines.
// Returns empty vector if no multi-step pipeline was found.
static std::vector<std::string> simplify_type_multiline_lines(
    const std::string& raw, int indent) {
  std::string stripped = collapse_angle_spacing(strip_namespaces(raw));
  TypeNode tree = parse_type(stripped);

  Pipeline pipeline;
  if (build_pipeline(tree, pipeline) && pipeline.steps.size() > 1) {
    return render_pipeline_lines(pipeline, indent);
  }

  return {};
}

// ============================================================================
// Line-Level Type Replacement
// ============================================================================

// Find the end of a balanced type expression starting at pos.
static size_t find_type_end(const std::string& line, size_t pos) {
  size_t i = pos;
  int angle_depth = 0;
  int paren_depth = 0;

  while (i < line.size()) {
    char c = line[i];
    if (c == '<' && paren_depth == 0) {
      angle_depth++;
    } else if (c == '>' && paren_depth == 0) {
      if (angle_depth == 0) break;
      angle_depth--;
      if (angle_depth == 0) {
        i++;
        break;
      }
    } else if (c == '(') {
      paren_depth++;
    } else if (c == ')') {
      if (paren_depth == 0) break;
      paren_depth--;
    } else if (angle_depth == 0 && paren_depth == 0) {
      if (c == ';' || c == ',' || c == '\'' || c == ']' || c == '[') break;
      if (c == ' ' || c == '&' || c == '*') break;
    }
    i++;
  }
  return i;
}

// Scan a line for known operator struct names, extract each full
// balanced type expression, simplify it, and replace inline.
static std::string simplify_types_in_line(const std::string& line) {
  std::string stripped = strip_namespaces(line);

  const auto& op_map = get_operator_map();

  // Build sorted list of operator names (longest first) once.
  static std::vector<std::string> sorted_names;
  if (sorted_names.empty()) {
    for (const auto& [name, _] : op_map) {
      sorted_names.push_back(name);
    }
    std::sort(sorted_names.begin(), sorted_names.end(),
      [](const auto& a, const auto& b) { return a.size() > b.size(); });
  }

  std::string result;
  size_t pos = 0;

  while (pos < stripped.size()) {
    bool found = false;
    for (const auto& name : sorted_names) {
      if (pos + name.size() > stripped.size()) continue;
      if (stripped.compare(pos, name.size(), name) != 0) continue;

      // Word boundary check.
      if (pos > 0 && (std::isalnum(stripped[pos - 1]) || stripped[pos - 1] == '_')) continue;

      size_t after_name = pos + name.size();
      size_t type_end;
      if (after_name < stripped.size() && stripped[after_name] == '<') {
        type_end = find_type_end(stripped, pos);
      } else {
        type_end = after_name;
      }

      std::string type_str = stripped.substr(pos, type_end - pos);
      result += simplify_type(type_str);
      pos = type_end;
      found = true;
      break;
    }
    if (!found) {
      result += stripped[pos];
      pos++;
    }
  }

  return collapse_angle_spacing(result);
}

// Like simplify_types_in_line, but renders multi-step pipelines across
// multiple lines with indentation. Used for diagnostic lines (error:, note:)
// where the types are long enough to benefit from multi-line rendering.
// base_indent is the indentation for continuation lines.
static std::string simplify_types_in_line_multiline(
    const std::string& line, int base_indent) {
  std::string stripped = strip_namespaces(line);

  const auto& op_map = get_operator_map();

  // Build sorted list of operator names (longest first) once.
  static std::vector<std::string> sorted_names;
  if (sorted_names.empty()) {
    for (const auto& [name, _] : op_map) {
      sorted_names.push_back(name);
    }
    std::sort(sorted_names.begin(), sorted_names.end(),
      [](const auto& a, const auto& b) { return a.size() > b.size(); });
  }

  std::string result;
  size_t pos = 0;

  while (pos < stripped.size()) {
    bool found = false;
    for (const auto& name : sorted_names) {
      if (pos + name.size() > stripped.size()) continue;
      if (stripped.compare(pos, name.size(), name) != 0) continue;

      // Word boundary check.
      if (pos > 0 && (std::isalnum(stripped[pos - 1]) || stripped[pos - 1] == '_')) continue;

      size_t after_name = pos + name.size();
      size_t type_end;
      if (after_name < stripped.size() && stripped[after_name] == '<') {
        type_end = find_type_end(stripped, pos);
      } else {
        type_end = after_name;
      }

      std::string type_str = stripped.substr(pos, type_end - pos);

      // Try multi-line pipeline rendering for this type.
      auto ml_lines = simplify_type_multiline_lines(type_str, base_indent);
      if (!ml_lines.empty()) {
        // Multi-step pipeline: render across multiple lines.
        result += "\n";
        for (size_t li = 0; li < ml_lines.size(); ++li) {
          result += ml_lines[li];
          if (li < ml_lines.size() - 1) result += "\n";
        }
      } else {
        // Single type or no pipeline: render inline.
        result += simplify_type(type_str);
      }

      pos = type_end;
      found = true;
      break;
    }
    if (!found) {
      result += stripped[pos];
      pos++;
    }
  }

  return collapse_angle_spacing(result);
}

// ============================================================================
// ANSI Code Handling
// ============================================================================

// Strip ANSI escape sequences from a string.
static std::string strip_ansi(const std::string& input) {
  std::string result;
  result.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '[') {
      // Skip ESC [ ... m/K sequence.
      i += 2;
      while (i < input.size() && input[i] != 'm' && input[i] != 'K') {
        i++;
      }
      // Skip the terminating 'm' or 'K'.
      continue;
    }
    result += input[i];
  }
  return result;
}

// Split a line into a "diagnostic prefix" and the remainder,
// both with ANSI codes stripped.
// The prefix is everything up to and including the first
// error:/note:/warning:/required from/In instantiation/In substitution keyword.
// Returns {stripped_prefix, stripped_rest}.
// ANSI is stripped from both parts because:
// - The formatter restructures text, making embedded ANSI codes meaningless.
// - PlatformIO / VSCode apply their own coloring on top.
// - Embedded GCC ANSI resets would clash with PlatformIO's outer coloring.
static std::pair<std::string, std::string> split_diagnostic_prefix(const std::string& line) {
  std::string stripped = strip_ansi(line);

  // Keywords that mark the end of the diagnostic prefix.
  static const std::vector<std::string> keywords = {
    "error: ", "note: ", "warning: ",
    "required from here", "required from ",
    "In instantiation of ", "In substitution of ",
    "required for the satisfaction of ",
  };

  for (const auto& kw : keywords) {
    size_t pos = stripped.find(kw);
    if (pos == std::string::npos) continue;

    size_t prefix_end = pos + kw.size();
    return {stripped.substr(0, prefix_end), stripped.substr(prefix_end)};
  }

  // No keyword found: return the whole stripped line as rest.
  return {"", stripped};
}

// ============================================================================
// Main Processing
// ============================================================================

// Detect source code lines (e.g., "  294 |     code here").
static bool is_source_code_line(const std::string& line) {
  std::string stripped = strip_ansi(line);
  // Match numbered source lines (e.g., "  294 |  code here")
  // and caret/underline lines (e.g., "      |      ^~~~~~~~").
  static const std::regex source_re(R"(^\s*(\d+\s*)?\|)");
  return std::regex_search(stripped, source_re);
}

// ============================================================================
// [with ...] Clause Formatting
// ============================================================================

// Find matching closing bracket/paren at depth 0,
// tracking balanced <>, (), [].
static size_t find_matching_close(const std::string& s, size_t start, char open, char close) {
  int depth = 0;
  int angle = 0, paren = 0, bracket = 0;
  for (size_t i = start; i < s.size(); ++i) {
    char c = s[i];
    if (c == '<') angle++;
    else if (c == '>') angle--;
    else if (c == '(') paren++;
    else if (c == ')') paren--;
    else if (c == '[') bracket++;
    else if (c == ']') {
      if (c == close && angle == 0 && paren == 0 && bracket == 0) return i;
      bracket--;
    }
    if (c == open) depth++;
    else if (c == close && angle == 0 && paren == 0 && bracket == 0) {
      return i;
    }
  }
  return std::string::npos;
}

// Split a string on a delimiter character at depth 0
// (not inside balanced <>, (), []).
static std::vector<std::string> split_at_depth0(
    const std::string& s, char delim) {
  std::vector<std::string> parts;
  int angle = 0, paren = 0, bracket = 0;
  size_t start = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '<') angle++;
    else if (c == '>') angle--;
    else if (c == '(') paren++;
    else if (c == ')') paren--;
    else if (c == '[') bracket++;
    else if (c == ']') bracket--;
    else if (c == delim && angle == 0 && paren == 0 && bracket == 0) {
      parts.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  }
  parts.push_back(s.substr(start));
  return parts;
}

// Format a line containing a [with ...] clause with multi-line pipeline rendering.
// The line should already be ANSI-stripped.
static std::string format_with_clause_line(const std::string& line) {
  std::string stripped = strip_namespaces(line);

  // Find "[with " in the line.
  size_t with_start = stripped.find("[with ");
  if (with_start == std::string::npos) return simplify_types_in_line(line);

  // Find matching ']'.
  size_t content_start = with_start + 6;  // past "[with "
  size_t with_end = std::string::npos;
  {
    int angle = 0, paren = 0, bracket = 1;
    for (size_t i = content_start; i < stripped.size(); ++i) {
      char c = stripped[i];
      if (c == '<') angle++;
      else if (c == '>') angle--;
      else if (c == '(') paren++;
      else if (c == ')') paren--;
      else if (c == '[') bracket++;
      else if (c == ']') {
        bracket--;
        if (bracket == 0 && angle == 0 && paren == 0) {
          with_end = i;
          break;
        }
      }
    }
  }
  if (with_end == std::string::npos) return simplify_types_in_line(line);

  std::string prefix = simplify_types_in_line(stripped.substr(0, with_start));
  std::string with_content = stripped.substr(content_start, with_end - content_start);
  std::string suffix_str = stripped.substr(with_end + 1);

  // Split on ';' at depth 0.
  auto assignments = split_at_depth0(with_content, ';');

  std::ostringstream out;
  out << prefix << "[with\n";

  for (size_t ai = 0; ai < assignments.size(); ++ai) {
    std::string asgn = trim(assignments[ai]);
    if (asgn.empty()) continue;

    // Split on first '='.
    auto eq_pos = asgn.find('=');
    if (eq_pos == std::string::npos) {
      // No '=', just output as-is.
      out << "  " << collapse_angle_spacing(strip_namespaces(asgn));
      if (ai < assignments.size() - 1) out << ";";
      out << "\n";
      continue;
    }

    std::string var_name = trim(asgn.substr(0, eq_pos));
    std::string type_str = trim(asgn.substr(eq_pos + 1));

    // Extract trailing ref qualifier (& or &&).
    std::string ref_qual;
    while (!type_str.empty() && type_str.back() == '&') {
      ref_qual = "&" + ref_qual;
      type_str.pop_back();
    }
    type_str = trim(type_str);

    out << "  " << var_name << " =\n";

    // Try multi-line pipeline rendering.
    auto pipeline_lines = simplify_type_multiline_lines(type_str, 4);
    if (!pipeline_lines.empty()) {
      for (size_t li = 0; li < pipeline_lines.size(); ++li) {
        out << pipeline_lines[li];
        if (li == pipeline_lines.size() - 1) {
          out << ref_qual;
          if (ai < assignments.size() - 1) out << ";";
        }
        out << "\n";
      }
    } else {
      // Single type, render inline at indent 4.
      std::string simplified = collapse_angle_spacing(
          strip_namespaces(simplify_type(type_str)));
      out << "    " << simplified << ref_qual;
      if (ai < assignments.size() - 1) out << ";";
      out << "\n";
    }
  }

  out << "]" << simplify_types_in_line(suffix_str);
  return out.str();
}

// Determine the ANSI color code for a diagnostic prefix.
// Matches PlatformIO's coloring: error → red, everything else → yellow.
static std::string ansi_color_for_prefix(const std::string& prefix) {
  if (prefix.find("error:") != std::string::npos) return "\033[31m";
  return "\033[33m";  // yellow for warnings, notes, and other diagnostics
}

// Output a potentially multi-line formatted string.
// The first line is printed as-is (PlatformIO colors it).
// Continuation lines get an ANSI color prefix so they inherit the
// diagnostic level's color even though PlatformIO defaults them to yellow.
static void output_formatted(const std::string& formatted, const std::string& ansi_color) {
  size_t pos = 0;
  bool first = true;
  while (pos < formatted.size()) {
    size_t nl = formatted.find('\n', pos);
    std::string segment;
    if (nl == std::string::npos) {
      segment = formatted.substr(pos);
      pos = formatted.size();
    } else {
      segment = formatted.substr(pos, nl - pos);
      pos = nl + 1;
    }
    if (!first && !ansi_color.empty()) {
      std::cout << ansi_color;
    }
    std::cout << segment << "\n";
    first = false;
  }
}

static void process_stream(std::istream& in) {
  std::string line;

  while (std::getline(in, line)) {
    if (is_source_code_line(line)) {
      // Source code lines: pass through unchanged (preserve ANSI).
      std::cout << line << "\n";
    } else {
      // Split into ANSI-stripped diagnostic prefix and rest.
      auto [prefix, rest] = split_diagnostic_prefix(line);

      std::string ansi = prefix.empty() ? "" : ansi_color_for_prefix(prefix);

      if (rest.find("[with ") != std::string::npos) {
        output_formatted(prefix + format_with_clause_line(rest), ansi);
      } else if (!prefix.empty()) {
        // Has diagnostic prefix.
        // rest is ANSI-stripped; use multi-line rendering for long types.
        std::string formatted = simplify_types_in_line_multiline(rest, 2);
        if (formatted.find('\n') != std::string::npos) {
          // Multi-line result: break after the opening quote so
          // the type/pipeline content starts indented on the next line.
          std::string combined = prefix + formatted;
          size_t quote_pos = combined.find('\'');
          if (quote_pos != std::string::npos && quote_pos < combined.size() - 1) {
            std::string before = combined.substr(0, quote_pos + 1);
            std::string after = combined.substr(quote_pos + 1);
            // Trim leading whitespace/newline from 'after' and re-indent.
            size_t first_non_ws = after.find_first_not_of(" \n");
            if (first_non_ws != std::string::npos) {
              after = after.substr(first_non_ws);
            }
            output_formatted(before + "\n  " + after, ansi);
          } else {
            output_formatted(combined, ansi);
          }
        } else {
          output_formatted(prefix + formatted, ansi);
        }
      } else {
        // No diagnostic keyword found: process the whole stripped line.
        std::cout << simplify_types_in_line(rest) << "\n";
      }
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    std::string filename = argv[1];
    if (filename == "--help" || filename == "-h") {
      std::cerr << "Usage: " << argv[0] << " [file]\n"
                << "  Reads compiler output from file or stdin.\n"
                << "  Simplifies rheoscape template types in error messages.\n";
      return 0;
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: cannot open file: " << filename << "\n";
      return 1;
    }
    process_stream(file);
  } else {
    process_stream(std::cin);
  }
  return 0;
}
