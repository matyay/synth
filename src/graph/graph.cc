#include "graph.hh"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Graph {

// ============================================================================

std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("graph");

// ============================================================================

}; // Graph
