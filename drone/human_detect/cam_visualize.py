import serial
import cv2
import numpy as np

# Make sure the baud rate matches your Arduino setup (we changed it to 2000000 earlier!)
ser = serial.Serial('COM3', 2000000) 

current_img = None

print("Listening for ESP32-S3 Serial Data...")

while True:
    try:
        line = ser.readline()
        
        # 1. Catch the Image Data
        # (Checking for both IMAGE and JPEG depending on what you left in your Arduino code)
        if b"START_IMAGE" in line or b"START_JPEG" in line:
            end_marker = b"END_IMAGE" if b"START_IMAGE" in line else b"END_JPEG"
            
            # Read raw bytes until the end marker
            raw_data = ser.read_until(end_marker)
            
            # Strip the end marker from the bytes
            frame_data = raw_data[:-len(end_marker)]
            
            # Convert bytes to an OpenCV image
            frame = np.frombuffer(frame_data, dtype=np.uint8)
            current_img = cv2.imdecode(frame, cv2.IMREAD_COLOR)

        # 2. Catch the Probability Score
        elif b"PROBABILITY:" in line:
            # Parse the float value from the serial string
            score_str = line.decode('utf-8').split(":")[1].strip()
            score = float(score_str)
            
            # If we successfully decoded an image right before this
            if current_img is not None:
                
                # Logic: If probability is > 50%, it's a human
                if score > 0.50:
                    text = f"HUMAN DETECTED: {score:.2f}"
                    color = (0, 255, 0) # Green (BGR format in OpenCV)
                else:
                    text = f"NO HUMAN: {score:.2f}"
                    color = (0, 0, 255) # Red
                
                # Draw the text overlay on the image
                cv2.putText(current_img, text, (10, 30), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2, cv2.LINE_AA)
                
                # Display the updated image in the window
                cv2.imshow('ESP32-CAM AI Stream', current_img)
                
                # Required for OpenCV window to refresh
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
                    
    except Exception as e:
        # Serial data over USB can sometimes drop bytes and get corrupted. 
        # This prevents the script from crashing if a frame is garbled.
        pass
        
cv2.destroyAllWindows()
ser.close()