# Signals

A physical modular synthesizer operates on electrical signals which voltage affects operation of its modules. Naturally available voltage levels are limited by hardware capabilities hence some signals cannot be encoded as voltages directly (eg. negative gains in dB).

The simulation do not impose such limits and singal values passed between modules are completely arbitrary. There are some conventions though:

- **Audio signal** - The full span is from -1.0 to +1.0. Anything below or above will be clipped right after exitting the simulated synthesizer and entering the audio output backend.

- **Control Voltage (CV)** - Controls pitch, encoded as 1V per octave in the same way as in physical modular synthesizers. The value of 1.0 corresponds to note A1, 2.0 to note A2 etc.

- **Gain** - Depending on module. Linear gain is encoded directly (1.0 - max, 0.0 - silence). Logarightmic gain is expressed directly in dB.