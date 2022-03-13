#ifndef GRAPH_EXCEPTION_HH
#define GRAPH_EXCEPTION_HH

#include <utils/exception.hh>

#include <stdexcept>
#include <string>

// ============================================================================
namespace Graph {

/// Generic graph exception
DECLARE_EXCEPTION(Exception, std::runtime_error);

/// Graph build error
DECLARE_EXCEPTION(BuildError, Exception);
/// Connection error
DECLARE_EXCEPTION(ConnectionError, BuildError);

/// Module error
DECLARE_EXCEPTION(ModuleError, Exception);
/// Processing error
DECLARE_EXCEPTION(ProcessingError, Exception);
/// Parameter error
DECLARE_EXCEPTION(ParameterError, Exception);


// ============================================================================

}; // Graph

#endif // GRAPH_EXCEPTION_HH
 
