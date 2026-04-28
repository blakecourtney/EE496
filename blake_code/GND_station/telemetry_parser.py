import struct

PACKET_START_BYTE = 0xFE
PACKET_END_BYTE = 0xFF

PKT_TYPE_TELEMETRY  = 1
PKT_TYPE_COMMAND    = 2
PKT_TYPE_FLAG       = 3
PKT_TYPE_HEARTBEAT  = 4
PKT_TYPE_WAYPOINT   = 5
PKT_TYPE_ACK        = 7
PKT_TYPE_FLAG_ACK   = 8
PKT_TYPE_PHOTO_CHUNK = 9
PKT_TYPE_PHOTO_DONE = 10
PKT_TYPE_PHOTO_START = 11

PACKET_SIZE = 68  # 1 start + 1 drone_id + 1 type + 64 payload + 1 end

class TelemetryParser:
    @staticmethod
    def parse(data):
        """Parse packet from bytes"""
        if len(data) < PACKET_SIZE:
            return None
        if data[0] != PACKET_START_BYTE:
            return None
        if data[PACKET_SIZE - 1] != PACKET_END_BYTE:
            return None

        drone_id    = data[1]
        packet_type = data[2]
        payload     = data[3:67]  # 64 bytes

        if packet_type == PKT_TYPE_TELEMETRY:
            return TelemetryParser._parse_telemetry(drone_id, payload)
        elif packet_type == PKT_TYPE_HEARTBEAT:
            return TelemetryParser._parse_heartbeat(drone_id, payload)
        elif packet_type == PKT_TYPE_FLAG:
            return TelemetryParser._parse_flag(drone_id, payload)
        elif packet_type == PKT_TYPE_PHOTO_DONE:
            return {'type': 'photo_done', 'id': drone_id}
        elif packet_type == PKT_TYPE_PHOTO_START:
            return {'type': 'photo_start', 'id': drone_id}


        return None

    @staticmethod
    def _parse_telemetry(drone_id, payload):
        """Parse telemetry packet"""
        try:
            lat, lon, alt, roll, pitch, yaw, battery = struct.unpack_from('7f', payload, 0)
            satellites = payload[28]
            armed      = bool(payload[29])
            return {
                'type':       'telemetry',
                'id':         drone_id,
                'lat':        lat,
                'lon':        lon,
                'alt':        alt,
                'roll':       roll,
                'pitch':      pitch,
                'yaw':        yaw,
                'battery':    battery,
                'satellites': satellites,
                'armed':      armed,
                'streaming':  False  # video logging only, no live stream
            }
        except Exception as e:
            print(f"Telemetry parse error: {e}")
            return None

    @staticmethod
    def _parse_heartbeat(drone_id, payload):
        """Parse heartbeat packet"""
        return {
            'type':  'heartbeat',
            'id':    drone_id,
            'armed': False
        }

    @staticmethod
    def _parse_flag(drone_id, payload):
        """Parse ML person detection flag"""
        try:
            lat, lon, alt, confidence = struct.unpack_from('4f', payload, 0)
            return {
                'type':       'flag',
                'id':         drone_id,
                'lat':        lat,
                'lon':        lon,
                'alt':        alt,
                'confidence': confidence
            }
        except Exception as e:
            print(f"Flag parse error: {e}")
            return None