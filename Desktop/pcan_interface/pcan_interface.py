#EXAMPLE
import can
import struct




bus = can.interface.Bus(interface='pcan', channel='PCAN_USBBUS1', bitrate=250000, state=can.bus.BusState.ACTIVE)

def send_message(data_id, data, timestamp):
    b_data = struct.pack(">i", data)
    b_data_id = struct.pack("b", data_id)
    b_timestamp = struct.pack(">i", timestamp)
    send_msg = can.Message(data=b_data+b_data_id+b_timestamp[0:3])
    bus.send(send_msg) 


send_message(100, 1, 0)

while(1):
    recv_msg = bus.recv()
    if(len(recv_msg.data) == 8):
        data_id = recv_msg.data[4]
        data = struct.unpack(">i", recv_msg.data[0:4])[0]
        timestamp = struct.unpack(">i", b'\0'+recv_msg.data[5:8])[0]
        print(data_id, data, timestamp)




    



