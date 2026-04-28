#handle input from GND esp32
import serial
import serial.tools.list_ports
import threading
import time

IMG_WIDTH  = 240
IMG_HEIGHT = 240
IMG_SIZE   = IMG_WIDTH * IMG_HEIGHT * 2  # RGB565

class SerialHandler:
    def __init__(self, callback=None, image_callback=None):
        self.ser = None
        self.callback = callback
        self.image_callback = image_callback
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
                    if b'START_IMAGE' in line:
                        self._read_image()
                    elif self.callback and line:
                        self.callback(line)
            except Exception as e:
                print(f"Read error: {e}")
                time.sleep(0.1)

    def _read_image(self):
        """Read raw RGB565 frame after START_IMAGE marker"""
        try:
            raw = bytearray()
            while len(raw) < IMG_SIZE:
                chunk = self.ser.read(min(4096, IMG_SIZE - len(raw)))
                if not chunk:
                    break
                raw.extend(chunk)
            if len(raw) == IMG_SIZE and self.image_callback:
                self.image_callback(bytes(raw))
            else:
                print(f"Dropped frame: {len(raw)}/{IMG_SIZE} bytes")
                self.ser.reset_input_buffer()
        except Exception as e:
            print(f"Image read error: {e}")
    
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