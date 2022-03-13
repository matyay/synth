#include "event.hh"
#include <chrono>

namespace MIDI {

// ============================================================================

void Event::log(
        spdlog::logger* a_Logger, 
        spdlog::level::level_enum a_Level
    )
{
    switch (type)
    {
    case Type::RESET:
        a_Logger->log(a_Level,
            "Event [t={:4d}] RESET");
        break;

    case Type::NOTE_ON:
        a_Logger->log(a_Level,
            "Event [t={:4d}] NOTE_ON  chn={:<2d} note={:<3d} velocity={:<3d}",
            time,
            data.note.channel,
            data.note.note,
            data.note.velocity[0]
        );
        break;    

    case Type::NOTE_OFF:
        a_Logger->log(a_Level,
            "Event [t={:4d}] NOTE_OFF chn={:<2d} note={:<3d} velocity={:<3d}",
            time,
            data.note.channel,
            data.note.note,
            data.note.velocity[0]
        );
        break;    

    case Type::CONTROLLER:
        a_Logger->log(a_Level,
            "Event [t={:4d}] CONTROLLER chn={:<2d} param={:<3d} value={:<3d}",
            time,
            data.ctrl.channel,
            data.ctrl.param,
            data.ctrl.value
        );
        break;    
    }
}

// ============================================================================
}; // MIDI
