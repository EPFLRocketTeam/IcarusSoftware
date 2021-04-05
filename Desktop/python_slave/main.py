import msv2

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
