#ifndef AUDIO_EXCEPTION_HH
#define AUDIO_EXCEPTION_HH

#include <utils/exception.hh>

#include <stdexcept>
#include <string>

// ============================================================================
namespace Audio {

/// Generic exception
DECLARE_EXCEPTION(Exception, std::runtime_error);

/// Device error
DECLARE_EXCEPTION(DeviceError, Exception);
/// Processing error
DECLARE_EXCEPTION(ProcessingError, Exception);

// ============================================================================

}; // Audio

#endif // AUDIO_EXCEPTION_HH
 
