import serial
import cv2
import numpy as np

ser = serial.Serial('COM3', 115200)

while True:
    line = ser.readline()
    if b"START_IMAGE" in line:
        # Read raw bytes until the end marker
        raw_data = ser.read_until(b"END_IMAGE")
        # Convert bytes to image
        frame = np.frombuffer(raw_data[:-9], dtype=np.uint8)
        img = cv2.imdecode(frame, cv2.IMREAD_COLOR)
        if img is not None:
            cv2.imshow('ESP32-CAM Stream', img)
            cv2.waitKey(1)