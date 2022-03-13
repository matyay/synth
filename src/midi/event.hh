#ifndef MIDI_EVENT_HH
#define MIDI_EVENT_HH

#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>

namespace MIDI {

// ============================================================================

struct Event {

    /// Event types
    enum class Type {

        // Voice events
        NOTE_ON,
        NOTE_OFF,
        CONTROLLER,

        // System events
        RESET,
    };

    /// Type
    Type     type;
    /// Timestamp [ms]
    int64_t  time;

    /// Data
    union {

        /// Note event
        struct {
            uint8_t     channel;
            uint8_t     note;
            uint8_t     velocity[2]; // On / off
            uint32_t    duration;
        } note;

        /// Controller event
        struct {
            uint8_t     channel;
            uint32_t    param;
            int32_t     value;
        } ctrl;

    } data;

    // ........................................................................

    // Logs the event
    void log (
        spdlog::logger* a_Logger, 
        spdlog::level::level_enum a_Level = spdlog::level::debug
    );
};

// ============================================================================

}; // MIDI

#endif // MIDI_EVENT_HH
