#include "logging.hh"

#include <spdlog/sinks/stdout_color_sinks.h>

// ============================================================================


std::shared_ptr<spdlog::logger> getLogger (const std::string& a_Name) {

    // Get an existing logger. When not found create it
    std::shared_ptr<spdlog::logger> logger = spdlog::get(a_Name);
    if (!logger) {
        logger = spdlog::stderr_color_mt(a_Name);
    }

    return logger;
}
