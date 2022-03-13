#include "synth_app.hh"

#include <utils/utils.hh>
#include <utils/args.h>
#include <utils/logging.hh>
#include <utils/exception.hh>

#include <utils/element_tree.hh>
#include <utils/xml2et.hh>

#include <strutils.hh>
#include <stringf.hh>

#include <audio/alsa_sink.hh>

#ifdef SYNTH_USE_PORTAUDIO
#include <audio/portaudio_sink.hh>
#endif

#include <midi/alsaseq_autoconn.hh>

#include <graph/builder.hh>
#include <graph/exception.hh>

#include <instrument/exception.hh>

#ifdef SYNTH_USE_TBB
#include <tbb/tbb.h>
#endif

#include <memory>
#include <fstream>
#include <queue>
#include <algorithm>


// ============================================================================

Instrument::Instrument* SynthApp::createInstrument (Graph::Builder* a_Builder,
                                                    const ElementTree::Node* a_Node)
{
    Instrument::Instrument::Attributes attributes;

    // Get name
    if (!a_Node->hasAttribute("name")) {
        throw Instrument::BuildError("Instrument must have a name!");
    }
    const std::string name = a_Node->getAttribute("name");

    // Get top-level module name
    if (!a_Node->hasAttribute("module")) {
        THROW(Instrument::BuildError,
            "Instrument '%s' must have a top-level module type provided!",
            name.c_str()
        );
    }
    const std::string module = a_Node->getAttribute("module");

    // TODO: Make attribute collection a function.
    // Collect attributes from the "instrument" tag
    for (auto& it : a_Node->getAttributes()) {
        if (it.first == "name" || it.first == "module") {
            continue;
        }

        attributes.set(it.first, it.second);
    }

    // Collect attributes from "attribute" tags
    for (auto node : a_Node->findAll("attribute")) {
        if (!node->hasAttribute("name")) {
            throw Instrument::BuildError("An 'attribute' tag must have a 'name'!");
        }
        if (!node->hasAttribute("value")) {
            throw Instrument::BuildError("An 'attribute' tag must have a 'value'!");
        }

        const std::string name  = node->getAttribute("name");
        const std::string value = node->getAttribute("value");

        if (attributes.has(name)) {
            THROW(Instrument::BuildError, "Attribute '%s' redefined!", name.c_str());
        }

        attributes.set(name, value);
    }

    // Create the instrument
    return new Instrument::Instrument(
        name,
        module,
        a_Builder,
        m_AudioSink->getSampleRate(),
        m_AudioSink->getFramesPerBuffer(),
        attributes
    );
}

// ============================================================================

void SynthApp::loadInstruments (const std::string& a_Config) {

    auto logger = getLogger("app");
    logger->info("Loading instruments from '{}'", a_Config);

    // Load the config
    auto root = xmlToElementTree(a_Config);
    if (root == nullptr) {
        THROW(std::runtime_error, "Error loading file '%s'", a_Config.c_str());
    }

    // Get sections
    auto modules = root->find("modules");
    if (modules == nullptr) {
        throw Instrument::BuildError("No 'modules' section in the config file!");
    }

    auto instruments = root->find("instruments");
    if (instruments == nullptr) {
        throw Instrument::BuildError("No 'instruments' section in the config file!");
    }

    // Create graph builder
    Graph::Builder builder;

    builder.registerBuiltinModules();
    builder.registerDefinedModules(modules.get());

    // Create instruments
    for (auto node : instruments->findAll("instrument")) {

        // Create the instrument
        auto instrument = std::shared_ptr<Instrument::Instrument>(
            createInstrument(&builder, node.get()));

        // Check the name
        auto name = instrument->getName();
        if (m_Instruments.has(name)) {
            logger->error("Duplicate instrument name '{}'! Not adding.",
                name
            );
            continue;
        }

        // Store
        m_Instruments.set(name, instrument);
    }

    // Store the config file name
    m_ConfigFiles.insert(a_Config);
}

void SynthApp::deleteInstruments () {

    auto logger = getLogger("app");
    logger->info("Deleting all instruments");

    // Delete all instruments
    m_Instruments.clear();
}

void SynthApp::dumpInstruments () {

    auto logger = getLogger("app");
    logger->info("Dumping instruments' graphs");

    // Dump graph for each instrument
    for (auto& it : m_Instruments) {
        auto fileName = it.first + ".dot";
        logger->debug("{}: '{}'", it.first, fileName);
        it.second->dumpGraphAsDot(fileName);
    }
}

// ============================================================================

void SynthApp::saveParameters (const std::string& a_FileName) {
    auto logger = getLogger("app");

    for (auto& it : m_Instruments) {
        try {
            if (!a_FileName.empty()) {
                it.second->saveParameters(a_FileName, true);
            } else {
                it.second->saveParameters();
            }
        }

        catch(const std::runtime_error& ex) {
            logger->error("Error saving parameters for instrument '{}', ",
                it.first, ex.what());
        }
    }
}

void SynthApp::loadParameters (const std::string& a_FileName) {
    auto logger = getLogger("app");

    for (auto& it : m_Instruments) {
        try {
            it.second->loadParameters(a_FileName);
        }

        catch(const std::runtime_error& ex) {
            logger->error("Error loading parameters for instrument '{}', ",
                it.first, ex.what());
        }
    }
}

// ============================================================================

void SynthApp::processCommands () {

    // Get all commands
    auto commands = m_SocketServer->getLines();

    // Process each client
    for (auto& it : commands) {
        int   clientId       = it.first;
        auto& clientCommands = it.second;

        // Process each command line
        std::vector<std::string> response;
        for (auto& line : clientCommands) {
            auto res = processCommand(line, clientId);
            response.insert(response.end(), res.begin(), res.end());
        }

        // Send responses
        m_SocketServer->sendLines(clientId, response);
    }
}

// ============================================================================

extern bool g_GotSigint;

int SynthApp::run (int argc, const char* argv[]) {

    int  res;
    auto logger = getLogger("app");

    // ........................................................................

    // Usage
    if (argt(argc, argv, "-h") || argt(argc, argv, "--help")) {
        printf("Usage: synth [options] [--instruments <instruments.xml>]\n");
        printf("\n");
        printf(" --backend <backend>    Audio backend\n");
        printf(" --device <device>      Audio device name\n");
        printf(" --sample-rate <rate>   Specify sample rate in Hz\n");
        printf(" --period <num samples> Specify audio buffer size in samples\n");
        printf(" --auto-connect         Automatically connect to MIDI input devices\n");
        printf(" --record               Start recording to a WAV file immediately\n");
        printf(" --dump-dot             Dump the instrument graph to a graphvis .dot file\n");
        printf(" --no-save-params       Do not save instrument parameters on exit\n");

        return 1;
    }

    // ........................................................................

#ifdef SYNTH_USE_PORTAUDIO
    const char* defaultBackend = "portaudio";
#else
    const char* defaultBackend = "alsa";
#endif

    const std::string backend = args(argc, argv, "--backend", defaultBackend);
    const std::string deviceName = args(argc, argv, "--device", "default");
    size_t sampleRate = argi(argc, argv, "--sample-rate", 48000);
    size_t bufferSize = argi(argc, argv, "--period", 256);

    // ........................................................................

    // Initialize the Audio sink
    logger->info("Initializing audio sink...");
    if (backend == "alsa") {
        m_AudioSink.reset(new Audio::AlsaSink());
    }
#ifdef SYNTH_USE_PORTAUDIO
    else if (backend == "portaudio") {
        m_AudioSink.reset(new Audio::PortAudioSink());
    }
#endif
    else {
        logger->critical("Unknown audio backend '{}'", backend);
        return -1;
    }

    // Open the audio device
    res = m_AudioSink->open(deviceName, sampleRate, 2, bufferSize);
    if(res) {
        if (res < 0) {
            logger->critical("Error opening audio device!");

        } else {
            logger->error("Available valid devices are:");
            auto devices = m_AudioSink->listDevices ();
            for (auto& name : devices) {
                logger->error(" {}", name);
            }
        }
        return -1;
    }

    // Initialize ALSA sequencer source
    logger->info("Initializing ALSA sequencer source...");
    m_MidiSource.reset(new MIDI::AlsaSeqSource());

    if(!m_MidiSource->open("synth")) {
        logger->critical("Error opening MIDI source!");
        return -1;
    }

    // Initialize ALSA sequence auto connector
    std::shared_ptr<MIDI::AlsaSeqAutoConnector> midiConnector;
    if (argt(argc, argv, "--auto-connect")) {
        logger->info("Initializing ALSA auto-connector...");
        midiConnector.reset(new MIDI::AlsaSeqAutoConnector(
            m_MidiSource->getId(),
            m_MidiSource->getPort()
        ));
    }

    // ........................................................................

    // Load instruments
    if (argt(argc, argv, "--instruments")) {

        try {
            const std::string fileName = args(argc, argv, "--instruments", "");
            loadInstruments(fileName);
        }

        catch (const std::runtime_error& ex) {
            logger->critical("Configuration error: {}", ex.what());
            return -1;
        }
    }

    // Load instrument's parameters
    loadParameters();

    // Dump instruments' graphs
    if (argt(argc, argv, "--dump-dot")) {
        dumpInstruments();
    }

    // ........................................................................

    // Create the socket server
    // FIXME: Parametrize the TCP port and max. client count
    m_SocketServer.reset(new Interface::SocketServer(10000, 2));
    if (!m_SocketServer->start()) {
        return -1;
    }

    // Create the recorder
    m_Recorder.reset(new Audio::Recorder());

    // ........................................................................

    // Start the audio stream
    if(!m_AudioSink->start()) {
        return -1;
    }
    // Start the MIDI source
    if(!m_MidiSource->start()) {
        return -1;
    }

    // Start the MIDI auto connector
    if (midiConnector) {
        midiConnector->start();
    }

    // Begin the recording immediately
    if (argt(argc, argv, "--record")) {
        m_Recorder->start();
    }

    // ........................................................................

    int64_t currTime = 0;
    int64_t prevTime = 0;

    Audio::Buffer<float> masterMix (
        m_AudioSink->getFramesPerBuffer(),
        m_AudioSink->getChannels()
    );

    size_t audioSize = masterMix.getSize() * masterMix.getChannels();
    std::unique_ptr<float> audioData(new float[audioSize]);

    std::queue<MIDI::Event> midiEvents;
    std::vector<Instrument::Voice*> activeVoices;

    // Main loop
    logger->info("Running...");
    while (!g_GotSigint) {

        // Process commands
        processCommands();

        // The audio sink is ready
        int64_t audioTime;
        if (m_AudioSink->isReady(&audioTime)) {

            // Take a timestamp
            prevTime = currTime;
            currTime = audioTime;

            int32_t sampleRate = m_AudioSink->getSampleRate();

            // Get new MIDI events
            for (auto& event : m_MidiSource->getEventsBefore(currTime)) {
                midiEvents.push(event);
            }

            // Process MIDI events.
            std::vector<MIDI::Event> midiEventsPeriod;
            while (!midiEvents.empty()) {
                auto& event = midiEvents.front();

                // Compute sample time relative to the current period
                int32_t relativeTime = event.time - prevTime;
                int32_t sampleTime   = (relativeTime * sampleRate) / 1000;

                // Late event
                if (sampleTime < 0) {
                    event.time = sampleTime;
                    event.log(logger.get(), spdlog::level::warn);

                    sampleTime = 0;
                }

                // The sample time fits in this period, get it
                if (sampleTime < (int32_t)m_AudioSink->getFramesPerBuffer()) {

                    MIDI::Event newEvent = event;
                    newEvent.time = sampleTime;

                    midiEventsPeriod.push_back(newEvent);
                    midiEvents.pop();
                }
                
                // The sample time is in the next period, or even later. Leave
                // it and all subsequent ones.
                else {
                    break;
                }
            }

            // Clear the active buffer
            masterMix.clear();

            // Build a list of all active voices
            activeVoices.clear();
            for (auto& it : m_Instruments) {
                auto& instr = it.second;
                instr->processEvents(midiEventsPeriod, activeVoices);
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

            // Output stereo, interleave channels
            if (m_AudioSink->getChannels() == 2) {
                size_t size = masterMix.getSize();
                float* ptrL = masterMix.data(0);
                float* ptrR = masterMix.data(1);
                float* ptr  = audioData.get();

                for (size_t i=0; i<size; ++i) {
                    *ptr++ = *ptrL++;
                    *ptr++ = *ptrR++;
                }

                m_AudioSink->writeBuffer(audioData.get());
            }
            // Output mono, just copy
            else if (m_AudioSink->getChannels() == 1) {
                m_AudioSink->writeBuffer(masterMix.data());
            }
            // Unsupported
            else {
                THROW(std::runtime_error, "Unsupported channel count %d",
                    m_AudioSink->getChannels());
            }

            // Check if the recorder is active. If so then pass the buffer to
            // it.
            if (m_Recorder && m_Recorder->isRecording()) {
                m_Recorder->push(masterMix);
            }
        }

        // Sleep to save CPU cycles
        usleep(10);
    }

    // ........................................................................

    // Stop MIDI auto connector
    if (midiConnector) {
        midiConnector->stop();
    }
    // Stop MIDI source
    m_MidiSource->stop();
    // Stop audio stream
    m_AudioSink->stop();

    // Stop the socket server
    m_SocketServer->stop();

    // Save all instrument's parameters
    if (!argt(argc, argv, "--no-save-params")) {
        saveParameters();
    }
    else {
        logger->info("NOT saving instrument parameters");
    }

    return 0;
}
