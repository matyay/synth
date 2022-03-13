# Instruments definition file

Instruments are defined via an XML file. One file can define multiple instruments.

The structure of the XML file:
```
<?xml version="1.0" encoding="UTF-8"?>
<synthesizer>
    <modules>
        <module type="the_top_module">
            ...
        </module>
    </modules>
    <instruments>
        <instrument name="my_instrument" module="the_top_module">
            ...
        </instrument>
    </instruments>
</synthesizer>
```

Under the single top-level `instruments` section there is the `modules` section which holds module definitions and the `instruments` sections that defines the instruments. Both sections are mandatory.

## Instruments

An instrument must have its top-level module where other modules comprising the top one are instantiated and connections between them are made.

Instruments have attributes defined via the `attribute` child section:
```
<attribute name="<name>" value="<value>"/>
```

Currently supported attributes are:

- "midiChannel" MIDI channel number the instrument is bound to (default 0)
- "minNote" Minimum MIDI note index the instrument reacts to (default 0)
- "maxNote" Minimum MIDI note index the instrument reacts to (default 127)
- "maxVoices" Maximum count of active (playing) voices of the instrument (default 1)
- "maxPlayTime" Maximum time (in seconds) a note can play (default 10)
- "minLevel" Audio output level threshold (in dB) under which the instrument is considered as not playing (default -96)
- "minSilentTime" Duration in seconds of the audio output level being below the threshold that is used to consider a voice no longer active (default 0.1)

## Modules

A module is a basic building block of a modular synthesizer. A module has input and output ports, a set of attributes (non-mutable) and parameters (mutable).

A `module` section under `modules` is used to define user module types while a `module` section under another `module` section defines a module instance.

### User modules

User module definition follows the scheme:
```
<module type="my_module">
    <output name="an_output_port"/>
    <input name="an_input_port"/>

    <module type="child_module_type" name="child_instance_name>
        ...
    </module>

    <patch from="from_port" to="to_port"/>
</module>
```

Sections `output` and `input` are used to define module ports.

For the top-level module of an instrument there is a requirement to have either a single output port named `out` (for mono output) or a pair of ports named `outL` and `outR` (for stereo output).

The section `module` defines a child module instance. At minimum it requires `type` and `name` attributes. Both user-defined and built-in modules can be instantiated this way. More on that in the next section

Finally connections between modules are made via the `patch` section where attributes `from` and `to` define source and sink ports respectively. Ports are specified as `<instance>.<port>` for child module ports and `<port>` for the containing module ports.

### Module instances

A module in instantiated in the following way:
```
<module type="type" name="name" ...>
    <parameter name="a_parameter" .../>
    ...
</module>
```

The module type may reference either to an user-defined module type or a built-in module type. The name is used to refer to the instance when making connections.

#### Attributes

Module instances have attributes which are non-mutable. They are defined as additional `<attribute>="<value>"` closures after the `module` tag. Depending on the module type there are different attributes with different function and meaning. Despite the interpretation, values must always be passed as quoted strings, even numbers.

#### Parameters

Each `parameter` section under `module` controls a module parameter. The `name` must always refer to an existing module parameter. Internally parameters can be one of the two types: number and choice. Number parameters refer to a single floating point number while choice parameters refer to a chosen item from a finite set.

Parameters can be dynamically controlled during the synthesizer runtime. The main synthesizer app exposes a TCP server which offers a simple textual interface for listing and manipulating them. For conveniance the control app was created that allows a user to set them via a CLI interface.

Number parameters have associated minimal, maximal and default value along with an increase/decrease step. Those are defined for each module parameter by default but can be overriden in the module instance via `min`, `max`, `def` and `step` attributes of the `parameter` section. Similarly as with attributes, numerical parameter values are defined as strings. Choice parameters have its legal value set fixed that cannot be changed. For them only the `def` attribute apply.

If a desgner of a modular synthesizer structure does not want a particular parameter to be visible, one can lock a parameter making it fixed and non-visible through the control interface by setting the `lock="1"` attribute.
