import time
import queue
from threading import Thread, Event

import socket

# =============================================================================

class SocketClient(Thread):
    """
    A line-oriented TCP socket client.
    """

    def __init__(self, ip, port):
        Thread.__init__(self)

        self.ip   = ip
        self.port = port

        self.stop_request = Event()

        # Rx and Tx buffers
        self.rx_data = ""
        self.tx_data = bytearray()

        # Rx and Tx queues
        self.rx_queue = queue.Queue()
        self.tx_queue = queue.Queue()

        # Create the socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Try connecting to the server
        self.socket.connect((ip, port))
        self.socket.setblocking(0)

        # Start the worker
        self.start()

    def _disconnect(self):
        """
        Disconnects and closes the socket
        """
        self.socket.shutdown(socket.SHUT_RDWR)
        self.socket.close()
        self.socket = None    

    def _get_lines(self):
        """
        Extracts lines from the receive buffer
        """

        # Split the receive buffer into lines
        while len(self.rx_data):

            # Find next new line. If not found then break
            p = self.rx_data.find("\n")
            if p == -1:
                break

            # Cut the line and stript it. Remove the data from the buffer
            line = self.rx_data[:p+1].rstrip()
            self.rx_data = self.rx_data[p+1:]

            # Put to the queue
            self.rx_queue.put(line)


    def is_connected(self):
        """
        Checks whether the client is connected
        """
        return self.is_alive()

    def stop(self):
        """
        Disconnects and stops the client
        """

        # Stop the worker
        if self.is_alive():
            self.stop_request.set()
            self.join()

        # Disconnect
        if self.socket is not None:
            self._disconnect()

    def loop(self):
        """
        Worker loop
        """
        can_sleep = True

        # Receive data
        try:
            chunk = self.socket.recv(256)

            # Error or disconnected
            if chunk == b'':
                self._disconnect()
                self.stop_request.set()
                return

            # Append to the receive buffer
            self.rx_data += chunk.decode("utf-8")
            can_sleep = False

            # Process data
            self._get_lines()

        except BlockingIOError as ex:
            pass

        # Check if there are lines to be sent
        while not self.tx_queue.empty():
            line = self.tx_queue.get_nowait()
            self.tx_data += str.encode(line + "\n", "utf-8")

        # Send data
        if len(self.tx_data):
            try:
                sent = self.socket.send(self.tx_data)

                # Error
                if sent <= 0:
                    self._disconnect()
                    self.stop_request.set()
                    return

                # Remove data from the buffer
                self.tx_data = self.tx_data[sent:]

            except BlockingIOError as ex:
                pass

        # Sleep to save CPU cycles
        if can_sleep:
            time.sleep(0.01)

    def run(self):
        """
        Worker entry point
        """

        # Main loop
        try:
            while not self.stop_request.is_set():
                self.loop()

        # Don't print exception. The thread will die and the app will notice
        # that the connection is lost.
        except Exception as ex:
            pass
