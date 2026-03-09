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
static std::string simplify_lambda(const std::string& name) {
  static const std::regex lambda_re(R"(.*::<lambda\(([^)]*)\)>)");
  std::smatch m;
  if (std::regex_match(name, m, lambda_re)) {
    return "lambda(" + m[1].str() + ")";
  }
  static const std::regex lambda_re2(R"(.*::<lambda\(\)>)");
  if (std::regex_match(name, m, lambda_re2)) {
    return "lambda()";
  }
  return name;
}

// ============================================================================
// Pipeline Types and Rendering
// ============================================================================

struct PipelineStep {
  std::string operator_name;
  std::vector<std::string> args;
  std::vector<std::string> source_labels;
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

static std::string render_type_compact(const TypeNode& node, bool simplify) {
  if (simplify) {
    Pipeline pipeline;
    if (build_pipeline(node, pipeline) && !pipeline.steps.empty()) {
      return render_pipeline(pipeline);
    }
  }

  std::string result = node.name;
  if (!node.args.empty()) {
    result += "<";
    for (size_t i = 0; i < node.args.size(); ++i) {
      if (i > 0) result += ", ";
      result += render_type_compact(node.args[i], simplify);
    }
    result += ">";
  }
  return result;
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
          step.args.push_back(simplify_lambda(render_type_compact(node.args[i], true)));
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
        step.args.push_back(simplify_lambda(render_type_compact(node.args[i], true)));
      }
      break;
    }
    case OperatorKind::MULTI_SOURCE: {
      step.is_source = true;
      for (const auto& arg : node.args) {
        Pipeline sub;
        if (build_pipeline(arg, sub)) {
          step.source_labels.push_back(render_pipeline(sub));
        } else {
          step.source_labels.push_back(render_type_compact(arg, true));
        }
      }
      break;
    }
    case OperatorKind::BINARY_SOURCE: {
      step.is_source = true;
      for (size_t i = 0; i < std::min(node.args.size(), size_t(2)); ++i) {
        Pipeline sub;
        if (build_pipeline(node.args[i], sub)) {
          step.source_labels.push_back(render_pipeline(sub));
        } else {
          step.source_labels.push_back(render_type_compact(node.args[i], true));
        }
      }
      for (size_t i = 2; i < node.args.size(); ++i) {
        step.args.push_back(simplify_lambda(render_type_compact(node.args[i], true)));
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
        step.args.push_back(simplify_lambda(render_type_compact(arg, true)));
      }
      break;
    }
  }

  pipeline.steps.push_back(std::move(step));
  return true;
}

static std::string render_pipeline(const Pipeline& pipeline) {
  if (pipeline.steps.empty()) return "<?>";

  std::ostringstream out;
  bool first = true;
  for (const auto& step : pipeline.steps) {
    if (!first) {
      out << " \xe2\x86\x92 ";
    }
    first = false;

    out << step.operator_name;

    if (!step.source_labels.empty()) {
      out << "(";
      for (size_t i = 0; i < step.source_labels.size(); ++i) {
        if (i > 0) out << ", ";
        out << step.source_labels[i];
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
  }
  return out.str();
}

// ============================================================================
// Full Type Simplifier
// ============================================================================

static std::string simplify_type(const std::string& raw) {
  std::string stripped = strip_namespaces(raw);
  TypeNode tree = parse_type(stripped);

  Pipeline pipeline;
  if (build_pipeline(tree, pipeline) && !pipeline.steps.empty()) {
    return render_pipeline(pipeline);
  }

  return render_type_compact(tree, true);
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

  return result;
}

// ============================================================================
// Main Processing
// ============================================================================

// Detect source code lines (e.g., "  294 |     code here").
static bool is_source_code_line(const std::string& line) {
  static const std::regex source_re(R"(^\s*\d+\s*\|)");
  return std::regex_search(line, source_re);
}

static void process_stream(std::istream& in) {
  std::string line;

  while (std::getline(in, line)) {
    if (is_source_code_line(line)) {
      std::cout << line << "\n";
    } else {
      std::cout << simplify_types_in_line(line) << "\n";
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
