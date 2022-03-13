#!/usr/bin/env python3
import argparse
import time
import re
import traceback

from collections import defaultdict

import curses
import curses.textpad

from socket_client import SocketClient

# =============================================================================

class App:

    class Parameter:
        """
        A parameter
        """
        def __init__(self, string):
            fields = string.split(",")

            if len(fields) < 3:
                raise ValueError(
                    "Malformed parameter definition '{}'".format(string))

            self.name = fields[0]

            self.ref  = fields[0]
            self.type = fields[2]

            # This is a numerical parameter
            if self.type == "NUMBER":
                self.choices    = []
                self.value      = float(fields[1])
                self.min_value  = float(fields[3])
                self.max_value  = float(fields[4])
                self.step       = float(fields[5])
                self.desc       = fields[6]

            # This is a choice parameter
            elif self.type == "CHOICE":
                choice = re.match(r"^([^(^)]+)\(([0-9]+)\)$", fields[1])

                self.value      = int(choice.group(2))
                self.choices    = fields[3].split(";")
                self.min_value  = 0
                self.max_value  = len(self.choices) - 1
                self.step       = 1
                self.desc       = fields[4]

            # Unsupported
            else:
                raise ValueError(
                    "Unsupported parameter type '{}'".format(self.type))

    class Group:
        """
        A group of items (parameters and / or other groups)
        """
        def __init__(self, name, items):
            self.name     = name
            self.items    = items
            self.unfolded = False

        def make_sub_groups(self):
            """
            Groups items into subgroups recursively
            """

            # Split into sub-groups
            sub_groups = defaultdict(lambda: [])
            for item in self.items:

                # No hierarchy, leave as it is
                if "." not in item.name:
                    sub_groups[item.name].append(item)

                # Group by first field of the hierarchical name
                else:
                    prefix, suffix = item.name.split(".", maxsplit=1)

                    # Rename and assign to a group
                    item.name = suffix
                    sub_groups[prefix].append(item)

            # Make Group objects
            self.items = []
            for group_name, items in sub_groups.items():

                # Only one item
                if len(items) == 1:
                    item = items[0]

                    # Rename
                    if group_name != item.name:
                        item.name = "{}.{}".format(group_name, item.name)

                    self.items.append(item)

                # A group
                else:
                    self.items.append(App.Group(
                        group_name,
                        items
                    ))

            # Sort by name
            self.items.sort(key=lambda i: i.name)

            # Recurse
            for item in self.items:
                if isinstance(item, App.Group):
                    item.make_sub_groups()


        def dump(self, indent=0):
            """
            Dumps the hierarchy
            """
            for i, item in enumerate(self.items):
                print(" " * indent, i, item.name)
                if isinstance(item, App.Group):
                    item.dump(indent+1)


    def __init__(self, client):
        self.quit = False
        self.win  = None

        self.client = client

        self.groups    = []
        self.items     = []
        self.selection = 0
        self.view      = 0

        self.is_recording = False

        self.status_msg   = ""
        self.status_code  = 0

        # List parameters
        self.list_parameters()

        # Check if there is a recording active
        sts, response = self.send_command("record status", 1.0)
        if sts == "OK":
            if len(response) >= 1 and response[0] == "recording":
                is_recording = True

    def send_command(self, command, timeout=1.0):
        """
        Sends a command to the server. Waits for the response.
        Raises TimeoutError on timeout
        """

        # Send the command
        self.client.tx_queue.put(command)

        # Receive response
        result   = None
        response = []
        deadline = time.time() + timeout

        while result is None:

            # Got something in the queue
            while not self.client.rx_queue.empty():
                line = self.client.rx_queue.get()

                # It is a token
                if line == "OK" or line.startswith("ERR"):
                    result = line
                    break

                # Part of the response
                response.append(line)

            # Check deadline
            if time.time() > deadline:
                raise TimeoutError

            # Sleep to save CPU
            time.sleep(0.001)

        # Return the response
        return result, response

    def status(self, sts):
        """
        Formats an error message and sets status bar text
        """

        # Get the message
        if ":" in sts:
            msg = sts.split(":", maxsplit=1)[1]
        else:
            msg = sts

        # Set status bar
        if sts.startswith("OK"):
            self.status_msg  = "Success: " + msg
            self.status_code = 0
        else:
            self.status_msg  = "Error: " + msg
            self.status_code = -1

    def list_parameters(self):
        """
        List parameters, updates the parameter group list
        """

        # Clear
        self.groups = []
        self.items  = []

        # Reset view
        self.view = 0
        self.selection = 0

        # List parameters
        sts, response = self.send_command("list_params", 5.0)
        if sts != "OK":
            self.status(sts)
            return

        # Decode parameters
        parameters = [self.Parameter(l) for l in response]

        # Create the root group and split it into subgroups wherever possible
        root = self.Group("root", parameters)
        root.make_sub_groups()

        if len(root.items) == 1:
            self.groups = root.items
        else:
            self.groups = [root]

        # Update items
        self.update_items()

    def update_parameter(self, parameter):
        """
        Updates the parameter value on the server
        """

        # Send command
        sts, response = self.send_command("set_param {} {}".format(
                            parameter.ref, float(parameter.value)))

        # Check status
        if sts != "OK":
            self.status(sts)

    def clear_instruments(self):
        """
        Clears all loaded instruments
        """

        # Clear
        self.groups = []
        self.items  = []

        # Reset view
        self.view = 0
        self.selection = 0

        # Clear
        sts, response = self.send_command("clear_instruments", 5.0)
        self.status(sts)

    def reload_instruments(self):
        """
        Reload all configuration files
        """

        # Clear
        self.groups = []
        self.items  = []

        # Reset view
        self.view = 0
        self.selection = 0

        # Reload
        sts, response = self.send_command("reload_instruments", 5.0)
        self.status(sts)
        if sts != "OK":
            return

        # List parameters
        self.list_parameters()

    def start_recording(self):
        """
        Starts recording
        """

        # Send command
        sts, response = self.send_command("record start")

        # Check status
        if sts.startswith("OK"):
            self.is_recording = True

        else:
            self.status(sts)

    def stop_recording(self):
        """
        Stops recording
        """

        # Send command
        sts, response = self.send_command("record stop")

        # Check status
        if sts.startswith("OK"):
            self.is_recording = False

        else:
            self.status(sts)


    @staticmethod
    def get_max_view(height):
        return (height - 4) // 2

    def update_items(self):
        """
        Update the item list
        """

        self.items = []

        def append_items(group, level=0):
            """
            Recursively appends items
            """
            for item in group.items:

                # Append the item
                self.items.append((item, level,))

                # This is a group. If unfolded then add all of its items
                if isinstance(item, App.Group) and item.unfolded:
                    append_items(item, level+1)

        # Append root group(s)
        for group in self.groups:
            append_items(group)

        # Limit the selection
        self.selection = min(self.selection, len(self.items) - 1)

    def draw_bar(self, x0, x1, y, value, fg=2, bk=3):
        """
        Draws a horizontal bar
        """

        if value < 0.0:
            value = 0.0
        if value > 1.0:
            value = 1.0

        total_len = x1 - x0 + 1
        fill_len  = int(total_len * value)
        pad_len   = total_len - fill_len

        self.win.addstr(y, x0, " " * fill_len, curses.color_pair(fg))
        self.win.addstr(y, x0 + fill_len, " " * pad_len, curses.color_pair(bk))

    def draw_parameter(self, o, y, parameter, highlight=False):
        """
        Draws a single parameter.
        """
        h, w = self.win.getmaxyx()

        if highlight:
            fg = 4
            bar_fg = 5
            bar_bk = 6
        else:
            fg = 1
            bar_fg = 2
            bar_bk = 3

        name_len  = 30
        value_len = 19

        def center(s, pad):
            lnt = len(s[:pad])
            ofs = 0 if lnt >= pad else (pad - lnt) // 2

            n1 = ofs
            n2 = pad - lnt - ofs

            return " " * n1 + s[:pad] + " " * n2

        # Selection
        if highlight:
            self.win.addstr(y, 0, "->")

        # Parameter name
        x = 3 + o 
        n = name_len - o
        self.win.addstr(y, x, parameter.name[:(n-1)], curses.color_pair(fg))

        # Parameter value
        if parameter.type == "CHOICE":
            value_str = parameter.choices[int(parameter.value)]
        else:
            value_str = "{:+7.3f}".format(parameter.value)

        s = "[ " + center(value_str, value_len - 5) + " ]"
        x = 3 + name_len
        self.win.addstr(y, x, s, curses.color_pair(fg))

        # Bar
        x = 3 + name_len + value_len
        p = (parameter.value - parameter.min_value) / (parameter.max_value - parameter.min_value)
        self.draw_bar(x, w - 2, y, p, bar_fg, bar_bk)

    def draw_group(self, x, y, group, highlight=False):
        """
        Draws a parameter group header
        """
        h, w = self.win.getmaxyx()

        if highlight:
            fg = 4
        else:
            fg = 1

        # Selection
        if highlight:
            self.win.addstr(y, 0, "->")

        # Name
        self.win.addstr(y, x + 3, group.name, curses.color_pair(fg))

    def draw(self):
        """
        Redraws the screen
        """
        h, w = self.win.getmaxyx()

        # Title
        self.win.erase()
        self.win.addstr(0, 0, "Synth control app.")

        # Server address
        self.win.addstr(0, 20, "[{}:{}]".format(self.client.ip, self.client.port))

        # Record status
        if self.is_recording:
            self.win.addstr(0, w - 12, " RECORDING ", curses.color_pair(7))

        # Groups / parameters
        if len(self.items):
            for i in range(self.get_max_view(h)):
                j = i + self.view
                if j >= len(self.items):
                    break    

                item, indent = self.items[j]

                x = indent
                y = 2 + 2 * i

                if isinstance(item, self.Parameter):
                    self.draw_parameter(x, y, item, (j == self.selection))
                if isinstance(item, self.Group):
                    self.draw_group(x, y, item, (j == self.selection))

        # Status bar
        if self.status_msg != "":
            s  = self.status_msg[:w-1].ljust(w-1)
            fg = 8 if self.status_code == 0 else 7
            self.win.addstr(h-2, 0, " " + s, curses.color_pair(fg))

    def decrease_parameter(self, parameter, amount=1):
        """
        Decrease the selected parameter
        """
        parameter.value -= parameter.step * amount
        if parameter.value < parameter.min_value:
            parameter.value = parameter.min_value

        self.update_parameter(parameter)

    def increase_parameter(self, parameter, amount=1):
        """
        Increase the selected parameter
        """
        parameter.value += parameter.step * amount
        if parameter.value > parameter.max_value:
            parameter.value = parameter.max_value

        self.update_parameter(parameter)

    def on_key(self, key):
        """
        Called on keypress
        """
        h, w = self.win.getmaxyx()

        # Clear status
        self.status_msg = ""

        # Get selected item
        if self.selection != -1 and len(self.items):
            item, _ = self.items[self.selection]
        else:
            item = None

        # Move selection to 0
        if key == curses.KEY_HOME:
            self.selection  = 0
        # Move selection up by 5
        elif key == curses.KEY_PPAGE:
            self.selection -= 5
        # Move selection up by 1
        elif key == curses.KEY_UP or key == ord('k'):
            self.selection -= 1
        # Move selection down by 1
        elif key == curses.KEY_DOWN or key == ord('j'):
            self.selection += 1
        # Move selection down by 5
        elif key == curses.KEY_NPAGE:
            self.selection += 5
        # Move selection to the end
        elif key == curses.KEY_END:
            self.selection  = len(self.items) - 1

        # Unfold a group
        elif key == ord('+') and isinstance(item, self.Group):
            item.unfolded = True
            self.update_items()
        # Fold a group
        elif key == ord('-') and isinstance(item, self.Group):
            item.unfolded = False
            self.update_items()
        # Toggle group fold
        elif (key == ord('\n') or key == ord(' ')) \
            and isinstance(item, self.Group):
            item.unfolded = not item.unfolded
            self.update_items()

        # Decrease by 5
        elif (key == ord('D') or key == ord('H')) and \
            isinstance(item, self.Parameter):
            self.decrease_parameter(item, 5)
        # Decrease by 1
        elif (key == curses.KEY_LEFT   or key == ord('h')) and \
            isinstance(item, self.Parameter):
            self.decrease_parameter(item, 1)
        # Increase by 1
        elif (key == curses.KEY_RIGHT  or key == ord('l')) and \
            isinstance(item, self.Parameter):
            self.increase_parameter(item, 1)
        # Increase by 5
        elif (key == ord('C') or key == ord('L')) and \
            isinstance(item, self.Parameter):
            self.increase_parameter(item, 5)

        # Start / stop recording
        elif key == ord(' '):
            if not self.is_recording:
                self.start_recording()
            else:
                self.stop_recording()

        # Clear instruments
        elif key == ord('c'):
            self.clear_instruments()
        # Reload instruments
        elif key == ord('r'):
            self.reload_instruments()

        # Quit
        elif key == ord('q'):
            self.quit = True

        # Limit selection
        if self.selection < 0:
            self.selection = 0
        if self.selection >= len(self.items):
            self.selection = len(self.items) - 1

        # Move and limit view position
        if self.selection <  self.view:
            self.view = self.selection
        if self.selection >= self.view + self.get_max_view(h):
            self.view = self.selection - self.get_max_view(h) + 1

    def init(self, args):
        """
        Initialization routine
        """

        # Initialize curses window
        self.win = curses.initscr()
        curses.start_color()

        curses.init_pair(1, curses.COLOR_CYAN,  curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_CYAN)
        curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLUE)

        curses.init_pair(4, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_WHITE)
        curses.init_pair(6, curses.COLOR_WHITE, curses.COLOR_BLUE)

        curses.init_pair(7, curses.COLOR_BLACK, curses.COLOR_RED)
        curses.init_pair(8, curses.COLOR_BLACK, curses.COLOR_GREEN)

        curses.noecho()
        curses.cbreak()
        curses.curs_set(False)
        self.win.keypad(True)

    def exit(self):
        """
        Shutdown routine
        """

        # Shutdown curses window
        curses.nocbreak()
        self.win.keypad(False)
        curses.echo()
        curses.endwin()

    def loop(self):
        """
        Inner loop routine
        """

        # Redraw
        self.draw()

        # Get key
        key = self.win.getch()
        self.on_key(key)

        # Check if the client is still there
        if not self.client.is_connected():
            raise RuntimeError("Server disconnected")

    def run(self, args):
        """
        The application entry point
        """

        # Initialize
        self.init(args)

        # The main loop
        try:
            exception = None
            while not self.quit:
                self.loop()

        except KeyboardInterrupt:
            exception = None
        except Exception as ex:
            exception = ex

        # Exit
        self.exit()

        # Re-raise
        if exception is not None:
            raise exception

# =============================================================================

def main():

    # Parse arguments
    parser = argparse.ArgumentParser(formatter_class = \
                                     argparse.ArgumentDefaultsHelpFormatter)

    args = parser.parse_args()

    # Connect to the server
    try:
        client = SocketClient("127.0.0.1", 10000)
    except Exception as ex:
        print(repr(ex))
        exit(-1)

    # Run the app
    try:
        app = App(client)
        app.run(args)

    # On error stop the client
    except Exception as ex:
        client.stop()
        traceback.print_exc()
        exit(-1)

    # Just stop the client
    client.stop()

# =============================================================================


if __name__ == "__main__":
    main()
