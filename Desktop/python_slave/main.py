import msv2
import struct


PING = 0x00
SHUTDOWN = 0x01
PAYLOAD = 0x02

hb = None

def shutdown():
    print("shutting down...")
    command = "/usr/bin/sudo /sbin/shutdown -h now"
    import subprocess
    process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
    output = process.communicate()[0]
    print(output)
    
def ping():
    print("ping")
    hb.send_from_slave(PING, [0xce, 0xec])
    
def payload(data):
    print("payload_data: {} ".format(opcode, ', '.join(hex(x) for x in data)))

    if data and len(data) == 52:
        sensor_data = struct.unpack("I"+"iii"+"iii"+"ii"+"iiii", bytes(data))
        timestamp = sensor_data[0]
        acc_x = sensor_data[1]
        acc_y = sensor_data[2]
        acc_z = sensor_data[3]
        gyro_x = sensor_data[4]
        gyro_y = sensor_data[5]
        gyro_z = sensor_data[6]
        baro = sensor_data[7]
        cc_pressure = sensor_data[8]
        dynamixel_0 = sensor_data[9]
        dynamixel_1 = sensor_data[10]
        dynamixel_2 = sensor_data[11]
        dynamixel_3 = sensor_data[12]

        command_data[0] = 0 #timestamp
        command_data[1] = 0 #thrust
        command_data[2] = 0 #dynamixel_0
        command_data[3] = 0 #dynamixel_1
        command_data[4] = 0 #dynamixel_2
        command_data[5] = 0 #dynamixel_3

        s_data = struct.pack("I"+"i"+"iiii", command_data)

        hb.send_from_slave(PAYLOAD, s_data)
    else:
        #send error code
        hb.send_from_slave(PAYLOAD, [0xc5, 0xe5])
    


def recv_data(opcode, data):
    print('message ({}): [{}]'.format(opcode, ', '.join(hex(x) for x in data)))
    if opcode == PING:
        ping()
    if opcode == SHUTDOWN:
        shutdown()
    if opcode ==PAYLOAD:
        payload(data)
        
    
    

#connection
hb = msv2.msv2()
if hb.connect("/dev/serial0"):
    print("connected")
    hb.slave(recv_data)
else:
    print("error")
