#ifndef LOGGING_HH
#define LOGGING_HH

#include <spdlog/spdlog.h>
#include <memory>

// ============================================================================

/// Creates a logger or retrieves an existing one
std::shared_ptr<spdlog::logger> getLogger (const std::string& a_Name);

#endif // LOGGING_HH
