# main GND station
from tkinter import *
from tkinter import ttk, messagebox
import sys
import time
import threading
from serial_handler import SerialHandler
from telemetry_parser import TelemetryParser
from command_builder import CommandBuilder
from gui import GroundStationGUI


class GroundStation:
    def __init__(self):
        self.serial = SerialHandler(callback=self.on_serial_data)
        self.parser = TelemetryParser()

        # Setup GUI
        self.root = Tk()
        self.gui = GroundStationGUI(self.root, self.on_command,
                                    change_port_callback=self.reconnect_serial)

        # Connect to serial
        self.connect_serial()

        # Start drone timeout checker
        self.start_drone_timeout_checker()

    def connect_serial(self):
        """Prompt user to select serial port via dropdown"""
        ports = self.serial.list_ports()
        if not ports:
            messagebox.showerror("Error", "No serial ports found!")
            sys.exit(1)

        dialog = Toplevel(self.root)
        dialog.title("Select Serial Port")
        dialog.geometry("300x130")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)

        Label(dialog, text="Select GCS ESP32 port:", pady=10).pack()

        port_var = StringVar(value=ports[0])
        ttk.Combobox(dialog, textvariable=port_var, values=ports,
                     state="readonly", width=25).pack(padx=20, pady=5)

        def connect():
            port = port_var.get()
            if self.serial.connect(port):
                self.serial.start_reading()
                self.gui.status_label.config(text=f"Connected to {port}")
                dialog.destroy()
            else:
                messagebox.showerror("Error", f"Failed to connect to {port}")

        Button(dialog, text="Connect", command=connect).pack(pady=10)
        dialog.wait_window()

    def reconnect_serial(self):
        """Close current connection and prompt for a new port"""
        self.serial.close()
        self.connect_serial()

    def on_serial_data(self, data):
        try:
            data = data.strip()
            if not data:
                return
            # skip ESP log lines (they start with 'I', 'W', 'E', 'D')
            if data[0:1] in [b'I', b'W', b'E', b'D']:
                return
            parsed = self.parser.parse(data)
            if parsed:
                self.root.after(0, self.gui.update_drone, parsed)
        except Exception as e:
            print(f"Parse error: {e}")

    def on_command(self, cmd_type, drone_id, *args):
        """Called when user clicks a command button"""
        if cmd_type == 'arm':
            packet = CommandBuilder.arm(drone_id)
        elif cmd_type == 'disarm':
            packet = CommandBuilder.disarm(drone_id)
        elif cmd_type == 'request_image':
            packet = CommandBuilder.flag_ack(drone_id)
        elif cmd_type == 'waypoint':
            lat, lon, alt = args
            packet = CommandBuilder.waypoint(drone_id, lat, lon, alt)
        else:
            return
        self.serial.send(packet)
        print(f"Sent {cmd_type} to Drone {drone_id}")

    def start_drone_timeout_checker(self):
        """Background thread that turns indicators red when a drone stops sending"""
        def check():
            while True:
                now = time.time()
                for did, data in list(self.gui.drones.items()):
                    connected = (now - data.get('last_seen', 0)) < 10
                    self.root.after(0, self.gui.update_drone_indicator, did, connected)
                time.sleep(3)

        t = threading.Thread(target=check, daemon=True)
        t.start()

    def run(self):
        """Start the application"""
        try:
            self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
            self.root.mainloop()
        except KeyboardInterrupt:
            self.on_closing()

    def on_closing(self):
        """Cleanup on exit"""
        self.serial.close()
        self.root.destroy()


if __name__ == "__main__":
    app = GroundStation()
    app.run()