#ifndef GRAPH_PROCESSING_WAVEFORM_HH
#define GRAPH_PROCESSING_WAVEFORM_HH

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Processing {

// ============================================================================

/// Basic waveform functions
namespace Waveform {

/// Pure sine wave
float sine          (float x, float arg);
/// Sine wave, only positive halves 
float half_sine     (float x, float arg);
/// Absolute value of a sine wave
float abs_sine      (float x, float arg);
/// Like abs_sine but non-zero only for quater of period
float pulse_sine    (float x, float arg);
/// Doubled frequency, single sine pulse, then zero
float even_sine     (float x, float arg);
/// Like even_sine but absolute value
float even_abs_sine (float x, float arg);
/// Square wave with variable duty cycle
float square        (float x, float arg);
/// Derived square wave
float derived_square(float x, float arg);
/// Triangle wave with variable rise / fall time
float triangle      (float x, float arg);
/// Sawtooth wave
float sawtooth      (float x, float arg);

}; // Waveform

// ============================================================================

}; // Processing
}; // Graph

#endif // GRAPH_PROCESSING_WAVEFORM_HH
