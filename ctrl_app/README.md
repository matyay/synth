# Control application

This is a control application written in Python. Connects to a running `synth` via a TCP socket and allows manipulating module parameters, changing instruments and enabling/disabling audio recording.

The interface is based on the `curses` module (text based). Individual module parameters are displayed as a tree where each branch represents a single module. Branches can be expanded/collapsed.

The key bindings are:
- Up/Down or k/j - Navigate the parameter tree,
- PgUp/PgDown - Navigate the parameter tree jumping by 5 parameters,
- Home - Move to the first parameter,
- End - Move to the last parameter,
- + - Expand a tree branch,
- - - Collapse a tree branch,
- Enter - Toggle branch expand/collapse,
- Left/Right or h/l - Decrease/increase a parameter by a single step,
- D/C or H/L - Decrease/increase a parameter by 5 steps,
- Space - toggle recording audio to a WAV file,
- c - Clears (unloads) all instruments,
- r - Reloads current instrument description from file,
- q - Exist the control app.
