# EE496
EE496: Sonia Ashly Blake

# Drone Mesh Network Ground Control Station

## Hardware Requirements
- 1x ESP32 for GCS radio
- 1x ESP32 for drone simulator
- USB cables

## Setup

### ESP32 Firmware
1. Open `firmware/gcs_esp32/gcs_esp32.ino` in Arduino IDE
2. Install ESP32 board support
3. Flash to GCS ESP32
4. Note the MAC address printed in Serial Monitor
5. Update MAC address in `firmware/drone_simulator/config.h`
6. Flash `drone_simulator.ino` to second ESP32

### Python Ground Station
```bash
cd ground_station
pip install -r requirements.txt
python main.py
```

## Usage
1. Connect GCS ESP32 via USB
2. Power on simulator ESP32
3. Run Python ground station
4. Select COM port when prompted
5. Watch telemetry stream in!

## Testing Checklist
- [ ] Telemetry reception
- [ ] Command transmission
- [ ] Multiple drone tracking
- [ ] Latency measurement
