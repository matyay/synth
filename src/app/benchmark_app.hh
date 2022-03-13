#ifndef APP_BENCHMARK_HH
#define APP_BENCHMARK_HH

#include <instrument/factory.hh>

#include <spdlog/spdlog.h>

#include <memory>

// ============================================================================

class BenchmarkApp {
public:

    BenchmarkApp ();

    /// Runs the app
    int run (int argc, const char* argv[]);

protected:

    /// Logger
    std::shared_ptr<spdlog::logger> m_Logger;

    /// Instruments
    Instrument::Instruments m_Instruments;
};

#endif // APP_BENCHMARK_HH
