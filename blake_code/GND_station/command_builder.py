#build CMD packets to send to drones
PACKET_START_BYTE = 0xFE
PACKET_END_BYTE = 0xFF

PKT_TYPE_COMMAND = 2

CMD_ARM = 10
CMD_DISARM = 11
CMD_START_VIDEO = 20
CMD_STOP_VIDEO = 21

class CommandBuilder:
    @staticmethod
    def build(drone_id, command_type, params=None):
        """Build command packet as bytes"""
        packet = bytearray()
        packet.append(PACKET_START_BYTE)
        packet.append(drone_id)
        packet.append(0)  # Destination (0 = from GCS)
        packet.append(PKT_TYPE_COMMAND)
        packet.append(command_type)
        
        if params:
            packet.extend(params)
        
        packet.append(PACKET_END_BYTE)
        return bytes(packet)
    
    @staticmethod
    def arm(drone_id):
        return CommandBuilder.build(drone_id, CMD_ARM)
    
    @staticmethod
    def disarm(drone_id):
        return CommandBuilder.build(drone_id, CMD_DISARM)
    
    @staticmethod
    def start_video(drone_id):
        return CommandBuilder.build(drone_id, CMD_START_VIDEO)
    
    @staticmethod
    def stop_video(drone_id):
        return CommandBuilder.build(drone_id, CMD_STOP_VIDEO)