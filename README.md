# Synth

A headless simulator of a generic modular synthesizer.

## Building

Prerequisities:
- python3.6 or higher (for the control app)
- libxml
- libtbb (optional)
- libportaudio (optional)
- libasound2
- libsndfile

To clone and build synth:
```
git clone https://github.com/matyay/synth
cd synth
mkdir build && cd build
cmake ..
make
```

Useful CMake options:
- USE_TBB enables / disables use of the TBB library
- USE_PORTAUDIO enables / disables the portaudio library for audio playback

To install requirements for the controll app:
```
cd ctrl_app
pip install -r requirements.txt
```

## Usage

Starting the main synth app (headless):
```
synth [options] [--instruments <instruments_file.xml>]
```

To get the list of available options use `-h`/`--help` switch. Example instrument definitions can be found under the _examples_ directory.

Starting the control app:
```
cd ctrl_app
python3 ctrl_app.py
```

The controll app has a CLI interface.

## The idea

This project is a headless simulator of a modular synthesizer. A user can instantiate modules from the available module library and connect them together to build a playable virtual instrument. Furthermore, one can define its own modules that encapsulate other modules and their connectivity. There is no limit on the depth of the hierarchy.

The top-level entity is an instrument. An instrument encapsulates a single modular synthesizer setup represented by a top-level module. The top-level modules contains instances of other modules. Unlike in a physical modular synthesizer which has only a single voice (one note can be played at a time) each instrument can have multiple voices. Each voice corresponds to the same module setup but playing a different note.

The main synthesize app is headless - it only requires access to a local audio output device and a MIDI source. Control of all the "knobs" of modules is done via the control application which communicates with the main one via a TCP socket. The communication protocol is text based.

The instrument configuration can be loaded/reloaded/changed at runtime.

## Instrument definition

Instruments are defined using an XML file. For details refer to the instrument definition documentation under the _doc/_ folder.

Here is an example of an instrument made of a single sinewave oscillator (no envelope control, just on/off):
```
<?xml version="1.0" encoding="UTF-8"?>
<synthesizer>
    <modules>
        <module type="basic">
            <output name="out"/>

            <module type="midiSource" name="midi"/>
            <module type="vco" name="oscillator"/>
            <module type="multiplier" name="mult" numPorts="2"/>

            <patch from="midi.cv" to="oscillator.cv"/>
            <patch from="midi.gate" to="mult.in0"/>
            <patch from="oscillator.out" to="mult.in1"/>
            <patch from="mult.out" to="out"/>
        </module>
    </modules>
    <instruments>
        <instrument name="basic" module="basic">
            <attribute name="midiChannel" value="1" />
            <attribute name="maxVoices" value="16" />
            <attribute name="minLevel" value="-60.0" />
        </instrument>
    </instruments>
</synthesizer>
```

## Technicals

Originally written and tested on Linux.

Currently audio playback is possible either via `portaudio` (in theory this is cross-plaform) or directly via `ALSA` on a Linux system.

MIDI input is possible via the `ALSA` sequencer which limits the application to Linux systems only for now.
