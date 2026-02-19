#handle input from GND esp32
import serial
import serial.tools.list_ports
import threading
import time

class SerialHandler:
    def __init__(self, callback=None):
        self.ser = None
        self.callback = callback
        self.running = False
        self.thread = None
    
    def list_ports(self):
        """List available serial ports"""
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]
    
    def connect(self, port, baud=115200):
        """Connect to serial port"""
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
            time.sleep(2)  # Wait for Arduino reset
            print(f"Connected to {port}")
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False
    
    def start_reading(self):
        """Start reading in background thread"""
        if self.ser and not self.running:
            self.running = True
            self.thread = threading.Thread(target=self._read_loop, daemon=True)
            self.thread.start()
    
    def stop_reading(self):
        """Stop reading thread"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=2)
    
    def _read_loop(self):
        """Background thread that reads serial data"""
        while self.running:
            try:
                if self.ser.in_waiting:
                    line = self.ser.readline()
                    if self.callback and line:
                        self.callback(line)
            except Exception as e:
                print(f"Read error: {e}")
                time.sleep(0.1)
    
    def send(self, data):
        """Send data to serial port"""
        if self.ser:
            try:
                self.ser.write(data + b'\n')
                return True
            except Exception as e:
                print(f"Send error: {e}")
                return False
        return False
    
    def close(self):
        """Close serial connection"""
        self.stop_reading()
        if self.ser:
            self.ser.close()