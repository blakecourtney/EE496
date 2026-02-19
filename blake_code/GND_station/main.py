#main GND station
from tkinter import *
from tkinter import ttk, messagebox
import sys

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
        self.gui = GroundStationGUI(self.root, self.on_command)
        
        # Connect to serial
        self.connect_serial()
        
    def connect_serial(self):
        """Prompt user to select serial port"""
        ports = self.serial.list_ports()
        
        if not ports:
            messagebox.showerror("Error", "No serial ports found!")
            sys.exit(1)
        
        # Simple port selection dialog
        dialog = Toplevel(self.root)
        dialog.title("Select Serial Port")
        dialog.geometry("300x200")
        dialog.transient(self.root)
        dialog.grab_set()
        
        Label(dialog, text="Select GCS ESP32 port:", pady=10).pack()
        
        port_var = StringVar(value=ports[0])
        for port in ports:
            Radiobutton(dialog, text=port, variable=port_var, value=port).pack(anchor=W, padx=20)
        
        def connect():
            port = port_var.get()
            if self.serial.connect(port):
                self.serial.start_reading()
                dialog.destroy()
            else:
                messagebox.showerror("Error", f"Failed to connect to {port}")
        
        Button(dialog, text="Connect", command=connect).pack(pady=10)
        
        dialog.wait_window()
    
    def on_serial_data(self, data):
        """Called when serial data received"""
        try:
            # Parse packet
            parsed = self.parser.parse(data)
            if parsed:
                # Update GUI (must be done in main thread)
                self.root.after(0, self.gui.update_drone, parsed)
        except Exception as e:
            print(f"Parse error: {e}")
    
    def on_command(self, cmd_type, drone_id):
        """Called when user clicks command button"""
        if cmd_type == 'arm':
            packet = CommandBuilder.arm(drone_id)
        elif cmd_type == 'disarm':
            packet = CommandBuilder.disarm(drone_id)
        elif cmd_type == 'start_video':
            packet = CommandBuilder.start_video(drone_id)
        elif cmd_type == 'stop_video':
            packet = CommandBuilder.stop_video(drone_id)
        else:
            return
        
        self.serial.send(packet)
        print(f"Sent {cmd_type} to Drone {drone_id}")
    
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