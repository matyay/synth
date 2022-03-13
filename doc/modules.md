# Built-in modules


## midiSource

This module is used to introduce MIDI note events to the synthesizer. At least one module of this type must be present in each instrument (at any level of hierarchy).

### Ports

- **cv (out)** - Control "voltage"
  One "volt" per octave. 1.0 refers to note A1, 2.0 to note A2 etc.
- **gate (out)** - Gate signal
  Output signal equals 1.0 when a MIDI note is on and to 0.0 when it's off.
- **velocity (out)** - Note velocity
  Proportional to the played note velocity (0.0 - min, 1.0 - max)

Value of `cv` and `velocity` never changes as long as `gate` is "on" (1.0).

### Attributes

- **minNote** - Minimum note value that the module responds to.
- **maxNote** - Maximum note value that the module responds to.


## midiController

Represents a midi controller (eg. a knob). Presents a value of the knob on its output port in real time.

There is no filtration of the input controller value.

### Ports

- **out (out)** - Output value

### Attributes

- **controller** - Controller index (def. "0")
- **default** - Default output state (def. "0.0")
- **min** - Output value for minimal controller setting (def. "0.0")
- **max** - Output value for maximal controller setting (def. "1.0")


## vco

Basic voltage controlled oscillator. Can output one of the predefined waveforms. Supports dynamic amplitude and frequency modulation and duty cycle change (for some waveforms).

For AM and FM modulation the amplitude and frequence are calculated according to the equations using data from modulation inputs and corresponding parameters:

A = log2lin(amplitude) * (1.0 + amGain * am)
F = cvToFrequency(cv + detune) * (1.0 + fmGain * fm)

log2lin(x) - converts amplitude in dB to signal level
cvToFrequency(x) - converts control voltage to frequency in Hz

### Ports

- **cv (in)** - Control voltage input
- **am (in)** - AM signal input
- **fm (in)** - FM signal input
- **pwm (in)** - Duty cycle input (from 0.0 to 1.0)
- **out (out)** - Signal output

### Parameters

- **waveform** - Waveform type ("sine", "half_sine", "abs_sine", "pulse_sine", "even_sine", "even_abs_sine", "square", "derived_square", "triangle", "sawtooth").
- **amplitude** - Output amplitude in dB
- **phase** - Phase offset [deg]
- **detune** - Detune amount in semitones
- **amGain** - Amplitude modulation index
- **fmGain** - Frequency modulation index

Waveforms:
  - "sine" - Pure sine wave - sin(x),
  - "half_sine" - Only the first half of the sine wave, the second half is zeroed,
  - "abs_sine" - Absolute value of a sine wave - abs(sin(x)),
  - "pulse_sine" - Absolute value of 1st and 3rd quater of the sine wave, the rest is zeroed,
  - "even_sine" - Single period of a double frequency sine wave, the rest is zeroed,
  - "even_abs_sine" - As above but an absolute value of,
  - "square" - Square wave, variable duty cycle,
  - "derived_square" - Low-pass filtered first order derivative of a square wave.
  - "triangle" - Triangle wave,
  - "sawtooth" - Sawtooth (rising) wave.


## sampler (experimental!)

A sampler module where a single period of the output signal is represented by a waveform stored in a file.

For AM and FM modulation the amplitude and frequence are calculated according to the equations using data from modulation inputs and corresponding parameters:

A = log2lin(amplitude) * (1.0 + amGain * am)
F = cvToFrequency(cv + detune) * (1.0 + fmGain * fm)

log2lin(x) - converts amplitude in dB to signal level
cvToFrequency(x) - converts control voltage to frequency in Hz

### Ports

- **cv (in)** - Control voltage input
- **am (in)** - AM signal input
- **fm (in)** - FM signal input
- **out (out)** - Signal output

### Attributes

- **file** - Name of a WAV file containing the waveform. The file must be mono,
- **note** - Corresponding note of the waveform when played at original rate.

### Parameters

- **amplitude** - Output amplitude in dB
- **amGain** - Amplitude modulation index
- **fmGain** - Frequency modulation index


## noise

A white noise source.

### Ports

- **out (out)** - Signal output

### Attributes

- **seed** - Random generator seed

### Parameters

- **amplitude** - Noise amplitude [dB]


## constant

A constant signal source.

### Ports

- **out (out)** - Signal output

### Parameters

- **value** - Output signal value.


## multiplier

N-input generic multiplier. Each input has its own bias that can be added to the input signal. The module provides output gain control (linear)

### Ports

- **in<n> (in)** - N-th input where n ranges from 0 to N-1,
- **out (out)** - Signal output

### Attributes

- **numInputs** - Number of inputs. Must be greater or equal than 2

### Parameters

- **bias<n>** - Bias of the n-th input.


## adder

N-input generic adder. Each input has its own gain control. There is an additional bias that can be added to the final product.

### Ports

- **in<n> (in)** - N-th input where n ranges from 0 to N-1,
- **out (out)** - Signal output

### Attributes

- **numInputs** - Number of inputs. Must be greater or equal than 2

### Parameters

- **gain<n>** - Gain of the n-th input (linear).


### mixer

N-input mixer intended for audio signals. Each input has its own gain control. Gains are expressed in dB

### Ports

- **in<n> (in)** - N-th input where n ranges from 0 to N-1,
- **out (out)** - Signal output

### Attributes

- **numInputs** - Number of inputs. Must be greater or equal than 2

### Parameters

- **gain<n>** - Gain of the n-th input (in decibels).


## vga

Variable gain amplifier with logarithmic gain input.

### Ports

- **in (in)** - Signal input,
- **gain (in)** - Gain input (logarightmic, in dB),
- **out (out)** - Signal output

### Attributes

- **cutoff** - Cutoff level (in dB) below which the output is zeroed (def. -96.0)


## vcf

Dynamically controlled filter. Supports different filter types. All filter are based on a single biquad IIR structure. Allows dynamic control of frequency, gain and Q.

### Ports

- **in (in)** - Signal input,
- **freq (in)** - Filter frequency parameter input (1V / octave),
- **gain (in)** - Filter gain parameter input,,
- **q (in)** - Filter Q parameter input,
- **out (out)** - Signal output.

### Parameters

- **type** - Filter type. One of: "lpf", "hpf", "bpf", "notch", "apf", "peaking", "lowShelf", "highShelf"
- **bypass** - Enables the filter bypass


## softClipper

A soft clipper module, the cutoff level can be controlled dynamically via an input port.

### Ports

- **in (in)** - Signal input,
- **level (in)** - Soft cutoff level in dB,
- **out (out)** - Signal output.


## adsr

An ADSR envelope generator

### Ports

- **gate (in)** - Gate input. A rising edge of this signal triggers the generator,
- **out (out)** - Envelope signal output.

### Parameters

- attackTime - Attack time [s]
- decayTime - Decay time [s]
- sustainTime - Sustain time [s]
- releaseTime - Release time [s]
- attackLevel - Peak level of the envelope on attack in dB
- sustainLevel - Level of the envelope during the sustain period in dB
- releaseLevel - Output level when the generator is not active. Also target value for the release period. In dB
- sustainEnable - When set to "yes" the envelope keeps the sustain level beyond the sustain time untill the gate signal is high. When set to "no" the gate signal is ignored after the initial trigger and the sustain time is not extended.

