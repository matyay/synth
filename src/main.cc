#include "app/synth_app.hh"

#include <utils/args.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <signal.h>
#include <unistd.h>

// ============================================================================

bool g_GotSigint = false;

void sigintHandler (int sig) {
    (void)sig;
    g_GotSigint = true;
}

// ============================================================================

int main(int argc, const char* argv[]) {

    spdlog::set_pattern("%n: %^%v%$");
    spdlog::set_level(spdlog::level::info);

    auto logger = spdlog::stderr_color_mt("master");
    logger->info("spdlog v{}.{}.{}", SPDLOG_VER_MAJOR,
                                     SPDLOG_VER_MINOR,
                                     SPDLOG_VER_PATCH);

    // Catch SIGINT
    signal (SIGINT, sigintHandler);

    int exitCode = 0;

#ifndef DEBUG
    try {
#endif
        auto app = SynthApp();
        exitCode = app.run(argc, argv);
#ifndef DEBUG
    }

    catch (const std::runtime_error& ex) {
        logger->critical("std::runtime_error: {}", ex.what());
        exitCode = -1;
    }
    
    catch (const std::exception& ex) {
        logger->critical("std::exception: '{}'", ex.what());
        exitCode = -1;
    }
#endif

    SPDLOG_LOGGER_DEBUG(logger, "Exitting with {}", exitCode);
    return exitCode;
}
