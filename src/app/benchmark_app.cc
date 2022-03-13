#include "benchmark_app.hh"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <utils/utils.hh>
#include <utils/args.h>
#include <utils/exception.hh>

#include <audio/buffer.hh>
#include <audio/recorder.hh>

#include <strutils.hh>
#include <stringf.hh>

#ifdef SYNTH_USE_TBB
#include <tbb/tbb.h>
#endif

#include <memory>
#include <functional>
#include <fstream>
#include <algorithm>

// ============================================================================

BenchmarkApp::BenchmarkApp () {

    // Create logger
    m_Logger = spdlog::stderr_color_mt("app");
}

// ============================================================================

extern bool g_GotSigint;

int BenchmarkApp::run (int argc, const char* argv[]) {

    (void)argc;
    (void)argv;

    // ........................................................................

    size_t sampleRate = argi(argc, argv, "--sample-rate", 48000);
    size_t bufferSize = argi(argc, argv, "--period",      256);

    m_Logger->info("SampleRate: {}", sampleRate);
    m_Logger->info("BufferSize: {}", bufferSize);

    // ........................................................................

    // Load instruments
    if (!argt(argc, argv, "--instruments")) {
        throw std::runtime_error("Specify the '--instruments' option!");
    }
    
    m_Instruments.update(Instrument::loadInstruments(
        args(argc, argv, "--instruments", nullptr),
        sampleRate,
        bufferSize
    ));

    // ........................................................................

    // Initialize (and start) audio recorder
    Audio::Recorder recorder;
//    recorder.start();

    // ........................................................................

    Audio::Buffer<float> masterMix (
        bufferSize, 2
    );

    int64_t currSample = 0;
    int64_t maxSamples = 15 * 60 * sampleRate;

    int64_t trigSample = bufferSize / 2;

    std::vector<Instrument::Voice*> activeVoices;

    // Begin the benchmark
    m_Logger->info("Running benchmark...");
    int64_t timeStart = Utils::makeTimestamp();

    while (currSample < maxSamples) {
        std::vector<MIDI::Event> midiEvents;

        // Check if we need to trigger a note
        int64_t nextSample = currSample + bufferSize;
        if (nextSample > trigSample) {

            // Sample index within the current period
            int32_t eventTime = trigSample - currSample;
            // Schedule next trigger
            trigSample += sampleRate / 2; // 0.5s

            // Do not trigger to close to the end of period, need a time to
            // insert NOTE OFF event.
            if ((size_t)(eventTime + 4) >= bufferSize) {
                eventTime = (bufferSize - 1) - 4;
            }

            // Send events to instruments
            for (auto it : m_Instruments) {
                auto instr = it.second;

                // TODO
                int c = 0;
                int n = 60;
                int v = 64;

                for(n=60; n<=65; ++n) {
                // Note on
                MIDI::Event evOn;
                evOn.type = MIDI::Event::Type::NOTE_ON;
                evOn.time = eventTime;
                evOn.data.note.channel     = c;
                evOn.data.note.note        = n;
                evOn.data.note.velocity[0] = v;
                evOn.data.note.velocity[1] = v;
                evOn.data.note.duration    = 0;
                midiEvents.push_back(evOn);

                // Note off
                MIDI::Event evOff;
                evOff.type = MIDI::Event::Type::NOTE_OFF;
                evOff.time = eventTime + 16;
                evOff.data.note.channel     = c;
                evOff.data.note.note        = n;
                evOff.data.note.velocity[0] = 0;
                evOff.data.note.velocity[1] = 0;
                evOff.data.note.duration    = 0;
                midiEvents.push_back(evOff);
                }
            }

            // Sort events by time
            std::sort(std::begin(midiEvents), std::end(midiEvents),
                [](MIDI::Event& a, MIDI::Event& b) {return a.time < b.time;});
        }

        // Clear the active buffer
        masterMix.clear();

        // Build a list of all active voices
        activeVoices.clear();
        for (auto& it : m_Instruments) {
            auto& instr = it.second;
            instr->processEvents(midiEvents, activeVoices);
        }

        // Process voices
#ifdef SYNTH_USE_TBB
        tbb::parallel_for( tbb::blocked_range<size_t>(0, activeVoices.size()),
            [=](const tbb::blocked_range<size_t>& r) {
                for (size_t i=r.begin(); i!=r.end(); ++i) {
                    auto& voice = activeVoices[i];
                    voice->process();
                }
            }
        );
#else
        for (auto& voice : activeVoices) {
            voice->process();
        }
#endif

        // Downmix
        for (auto& voice : activeVoices) {
            masterMix += voice->getBuffer();
        }

        // Record audio
        if (recorder.isRecording()) {
            recorder.push(masterMix);
        }

        // Advance time
        currSample += bufferSize;
    }

    // Finish the benchmark
    int64_t timeElapsed = Utils::makeTimestamp() - timeStart;
    int64_t audioTime   = (1000L * currSample) / sampleRate;

    m_Logger->info("Elapsed time: {}ms", timeElapsed);
    m_Logger->info("Audio time  : {}ms", audioTime);
    m_Logger->info("Ratio       : x{:.3f}", (double)audioTime / (double)timeElapsed);

    return 0;
}
