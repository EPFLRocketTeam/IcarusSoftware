#!/usr/bin/env python3

import numpy as np
import math
import time



import can
import struct


# DATA ID
DATA_ID_PRESSURE        = 0 
DATA_ID_ACCELERATION_X  = 1 
DATA_ID_ACCELERATION_Y  = 2 
DATA_ID_ACCELERATION_Z  = 3 
DATA_ID_GYRO_X          = 4 
DATA_ID_GYRO_Y          = 5 
DATA_ID_GYRO_Z          = 6 

DATA_ID_GPS_HDOP        = 7 
DATA_ID_GPS_LAT         = 8 
DATA_ID_GPS_LONG        = 9 
DATA_ID_GPS_ALTITUDE    = 10
DATA_ID_GPS_SATS        = 11

DATA_ID_TEMPERATURE     = 12
DATA_ID_CALIB_PRESSURE  = 13

DATA_ID_AB_STATE        = 16
DATA_ID_AB_INC          = 17
DATA_ID_AB_AIRSPEED     = 18
DATA_ID_AB_ALT          = 19

DATA_ID_KALMAN_STATE    = 38
DATA_ID_KALMAN_X        = 40
DATA_ID_KALMAN_Y        = 41
DATA_ID_KALMAN_Z        = 42
DATA_ID_KALMAN_VX       = 43
DATA_ID_KALMAN_VY       = 44
DATA_ID_KALMAN_VZ       = 45
DATA_ID_KALMAN_YAW      = 46
DATA_ID_KALMAN_PITCH    = 47
DATA_ID_KALMAN_ROLL     = 48
DATA_ID_ALTITUDE        = 49
DATA_ID_COMMAND         = 80
 
DATA_ID_PRESS_1         = 85
DATA_ID_PRESS_2         = 86
DATA_ID_TEMP_1          = 87
DATA_ID_TEMP_2          = 88
DATA_ID_TEMP_3          = 89
DATA_ID_STATUS          = 90
DATA_ID_MOTOR_POS       = 91
DATA_ID_VANE_POS_1      = 92
DATA_ID_VANE_POS_2      = 93
DATA_ID_VANE_POS_3      = 94
DATA_ID_VANE_POS_4      = 95
 
DATA_ID_TVC_COMMAND     = 100
DATA_ID_THRUST_CMD      = 101
DATA_ID_VANE_CMD_1      = 102
DATA_ID_VANE_CMD_2      = 103
DATA_ID_VANE_CMD_3      = 104
DATA_ID_VANE_CMD_4      = 105
 
DATA_ID_TVC_HEARTBEAT   = 106

T2P_CONVERSION = 76.233


TVC_CMD_BOOT = 1
TVC_CMD_SHUTDOWN = 2
TVC_CMD_ABORT = 3

STATE_IDLE = 0
STATE_BOOT = 1
STATE_COMPUTE = 2
STATE_SHUTDOWN = 3
STATE_ABORT = 4


bus = None


def send_message(data_id, data, timestamp=0):
    b_data = struct.pack(">i", data)
    b_data_id = struct.pack("b", data_id)
    b_timestamp = struct.pack(">i", timestamp)
    send_msg = can.Message(data=b_data+b_data_id+b_timestamp[0:3])
    bus.send(send_msg) 


def recv_message(timeout=0.01):
    recv_msg = bus.recv(timeout)
    if(recv_msg is not None and len(recv_msg.data) == 8):
        data_id = recv_msg.data[4]
        data = struct.unpack(">i", recv_msg.data[0:4])[0]
        timestamp = struct.unpack(">i", b'\0'+recv_msg.data[5:8])[0]
        return {"valid" : True, "id" : data_id, "data" : data, "timestamp" : timestamp}
    return {"valid" : False, "id" : 0, "data" : 0, "timestamp" : 0}



if __name__ == '__main__':
    print(can)
    bus = can.interface.Bus(interface='pcan', channel='PCAN_USBBUS1', bitrate=250000, state=can.bus.BusState.ACTIVE)

    if bus is None:
        exit(1)


    send_message(DATA_ID_TVC_COMMAND, TVC_CMD_BOOT)