#ifndef INSTRUMENT_FACTORY_HH
#define INSTRUMENT_FACTORY_HH

#include "instrument.hh"

#include <graph/builder.hh>

#include <utils/dict.hh>
#include <utils/element_tree.hh>

#include <string>
#include <memory>

#include <cstddef>

// ============================================================================
namespace Instrument {

/// A Dictionary of Instrument objects
typedef Dict<std::string, std::shared_ptr<Instrument>> Instruments;

/// Creates a single instrument given a ready GraphBuilder object and a
/// description in a form of ElementTree.
Instrument* createInstrument (Graph::Builder* a_Builder,
                              const ElementTree::Node* a_Node,
                              size_t a_SampleRate,
                              size_t a_BufferSize);

/// Loads instruments from a configuration file
Instruments loadInstruments  (const std::string& a_Config,
                              size_t a_SampleRate,
                              size_t a_BufferSize);

// ============================================================================

}; // Instrument

#endif // INSTRUMENT_FACTORY_HH
 
