#ifndef INSTRUMENT_EXCEPTION_HH
#define INSTRUMENT_EXCEPTION_HH

#include <utils/exception.hh>

#include <stdexcept>
#include <string>

// ============================================================================
namespace Instrument {

/// Generic exception
DECLARE_EXCEPTION(Exception, std::runtime_error);

/// Build error
DECLARE_EXCEPTION(BuildError, Exception);
/// Processing error
DECLARE_EXCEPTION(ProcessingError, Exception);


// ============================================================================

}; // Instrument

#endif // INSTRUMENT_EXCEPTION_HH
 
