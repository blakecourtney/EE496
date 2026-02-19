#handle TLM packets from drone
import struct

PACKET_START_BYTE = 0xFE
PACKET_END_BYTE = 0xFF

PKT_TYPE_TELEMETRY = 1
PKT_TYPE_COMMAND = 2
PKT_TYPE_VIDEO = 3
PKT_TYPE_HEARTBEAT = 4

class TelemetryParser:
    @staticmethod
    def parse(data):
        """Parse telemetry packet from bytes"""
        if len(data) < 6:
            return None
            
        if data[0] != PACKET_START_BYTE:
            return None
        
        drone_id = data[1]
        dest_id = data[2]
        packet_type = data[3]
        
        if packet_type == PKT_TYPE_TELEMETRY:
            return TelemetryParser._parse_telemetry(drone_id, data[4:])
        elif packet_type == PKT_TYPE_HEARTBEAT:
            return TelemetryParser._parse_heartbeat(drone_id, data[4:])
        
        return None
    
    @staticmethod
    def _parse_telemetry(drone_id, payload):
        """Parse full telemetry packet"""
        try:
            offset = 0
            lat = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            lon = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            alt = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            roll = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            pitch = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            yaw = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            battery = struct.unpack('f', payload[offset:offset+4])[0]; offset += 4
            satellites = payload[offset]; offset += 1
            armed = bool(payload[offset]); offset += 1
            streaming = bool(payload[offset]) if offset < len(payload) else False
            
            return {
                'type': 'telemetry',
                'id': drone_id,
                'lat': lat,
                'lon': lon,
                'alt': alt,
                'roll': roll,
                'pitch': pitch,
                'yaw': yaw,
                'battery': battery,
                'satellites': satellites,
                'armed': armed,
                'streaming': streaming
            }
        except Exception as e:
            print(f"Parse error: {e}")
            return None
    
    @staticmethod
    def _parse_heartbeat(drone_id, payload):
        """Parse heartbeat packet"""
        armed = bool(payload[0]) if len(payload) > 0 else False
        return {
            'type': 'heartbeat',
            'id': drone_id,
            'armed': armed
        }