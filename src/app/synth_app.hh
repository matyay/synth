#ifndef APP_SYNTH_HH
#define APP_SYNTH_HH

#include <utils/dict.hh>
#include <utils/element_tree.hh>

#include <midi/alsaseq_source.hh>

#include <audio/buffer.hh>
#include <audio/audio_sink.hh>
#include <audio/recorder.hh>

#include <instrument/instrument.hh>

#include <iface/socket_server.hh>

#include <memory>
#include <unordered_set>

// ============================================================================

class SynthApp {
public:

    /// Runs the app
    int run (int argc, const char* argv[]);

protected:

    /// Creates a single instrument
    Instrument::Instrument* createInstrument (Graph::Builder* a_Builder,
                                              const ElementTree::Node* a_Node);

    /// Loads instruments
    void loadInstruments   (const std::string& a_Config);
    /// Reloads instruments
    void reloadInstruments ();
    /// Deletes instruments
    void deleteInstruments ();
    /// Dumps instruments' graphs as GraphViz DOT files
    void dumpInstruments   ();

    // ....................................................

    /// Saves all mutable instrument parameters to a file
    void saveParameters (const std::string& a_FileName = std::string());
    /// Loads instrument parameters from a file
    void loadParameters (const std::string& a_FileName = std::string());

    // ....................................................

    std::vector<std::string> cmdClearInstruments  (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdLoadInstruments   (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdReloadInstruments (const std::vector<std::string>& a_Args);

    std::vector<std::string> cmdDumpInstruments (const std::vector<std::string>& a_Args);

    std::vector<std::string> cmdListParams  (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdGetParam    (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdSetParam    (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdSaveParams  (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdLoadParams  (const std::vector<std::string>& a_Args);
    std::vector<std::string> cmdResetParams (const std::vector<std::string>& a_Args);

    std::vector<std::string> cmdRecord      (const std::vector<std::string>& a_Args);

    // ....................................................

    /// Processes a single client command
    std::vector<std::string> processCommand (const std::string& a_Command,
                                             int a_ClientId);

    /// Processes client commands
    void processCommands ();

    // ....................................................

    /// Audio sink
    std::unique_ptr<Audio::AudioSink> m_AudioSink;
    /// MIDI source
    std::unique_ptr<MIDI::AlsaSeqSource>  m_MidiSource;

    /// Instruments
    Dict<std::string, std::shared_ptr<Instrument::Instrument>> m_Instruments;
    /// Loaded config files
    std::unordered_set<std::string> m_ConfigFiles;

    /// Socket server
    std::unique_ptr<Interface::SocketServer> m_SocketServer;
    /// Audio recorder
    std::unique_ptr<Audio::Recorder> m_Recorder;
};

#endif // APP_SYNTH_HH
