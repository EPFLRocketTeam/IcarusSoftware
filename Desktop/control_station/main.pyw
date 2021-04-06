# File: main.py
import sys
import os
import platform
import re
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import QApplication, QWidget
from PySide2.QtCore import QFile, QIODevice, QThread, QObject, Slot, Signal, QTimer


import numpy as np

import struct
import msv2





DATA_BUFFER_LEN = 500
HEART_BEAT = 100


SENSOR_REMOTE_BUFFER = 5

counter = 0


time_data = []
window = []
#m = msv2.msv2()
worker = []

total_data = 1

#COMMANDS
GET_STAT = 	0x00
BOOT = 		0x01
SHUTDOWN = 	0x02
DOWNLOAD = 	0x03
TVC_MOVE = 	0x04
ABORT = 	0x05
RECOVER =	0x06
TRANSACTION = 0x07

#MOVE MODES


connection_status = "DISCONNECTED"
status_state = 0;


#recording
recording = 0
rec_file = None
start_rec = None

data_labels = ['pres_1 [mBar]', 'pres_2 [mBar]', 'temp_1 [0.1deC]', 'temp_2 [0.1deC]', 'temp_3 [0.1degC]', 'sensor_time [ms]']
remote_labels = ['data_id', 'temp_1', 'temp_2', 'temp_3', 'pres_1', 'pres_2', 'motor_pos', 'sensor_time', 'system state', 'counter_active', 'padding', 'counter']

def safe_int(d):
    try:
        return int(d)
    except:
        return 0

def safe_float(d):
    try:
        return float(d)
    except:
        return 0

def inc2deg(inc):
    return round(-inc/4/1024/66*1*360, 2)

def deg2inc(deg):
    return int(round(-deg*4*1024*66/1/360))

#4096 -> 360 and 2048 is the middle
def deg2dyn(deg):
    return int(round(deg*4096/360 + 2048))

def dyn2deg(dyn):
    return round((dyn - 2048)*360/4096, 2)

@Slot()
def connect_trig():
    device = window.connect_device.text()
    if connection_status == "DISCONNECTED":
        serial_worker.ser_connect(device)
    else:
        serial_worker.ser_disconnect()
        

@Slot(str)
def connect_cb(stat):
    global connection_status
    connection_status = stat
    window.connect_status.setText(stat)
    print("connection_updated")

def tvc_motor_move_trig():
    target = deg2dyn(safe_float(window.tvc_motor_target.text()))
    bin_data = struct.pack("i", target)
    serial_worker.send_generic(TVC_MOVE, bin_data)

def transaction_trig():
    acc_x = safe_int(window.trans_acc_x.text())
    acc_y = safe_int(window.trans_acc_y.text())
    acc_z = safe_int(window.trans_acc_z.text())

    gyro_x = safe_int(window.trans_gyro_x.text())
    gyro_y = safe_int(window.trans_gyro_y.text())
    gyro_z = safe_int(window.trans_gyro_z.text())

    baro = safe_int(window.trans_baro.text())
    cc_pres = safe_int(window.trans_cc_pres.text())
    bin_data = struct.pack("iii"+"iii"+"ii", 
                            acc_x,
                            acc_y,
                            acc_z,
                            gyro_x,
                            gyro_y,
                            gyro_z,
                            baro,
                            cc_pres)


    serial_worker.send_generic(TRANSACTION, bin_data)

def boot_trig():
    serial_worker.send_generic(BOOT, [0x00, 0x00])

def shutdown_trig():
    serial_worker.send_generic(SHUTDOWN, [0x00, 0x00])

def abort_trig():
    serial_worker.send_generic(ABORT, [0x00, 0x00])

def recover_trig():
    serial_worker.send_generic(RECOVER, [0x00, 0x00])

def ping_trig():
    serial_worker.send_ping()


def id_2_mem(id):
    usage = float(id)*32
    u_str = "B"
    u_flt = usage
    if(usage > 1000):
        u_str = "KB"
        u_flt =usage / 1000
    if(usage > 1000000):
        u_str = "MB"
        u_flt =usage / 1000000
    return "{:.3f} {}".format(u_flt, u_str)


def ping_cb(stat):
    global status_state
    global counter
    global total_data

    if(stat and len(stat) == 20):
        data = struct.unpack("HHiIiHBb", bytes(stat))
        #data [state, padding, counter, memory, tvc_pos, tvc_psu, tvc_error, tvc_temp]
        state = data[0]
        status_state = state
        window.status_state.clear()
        window.tvc_psu.clear()
        window.tvc_motor_current.clear()
        window.tvc_error.clear()
        window.tvc_temperature.clear()
        window.status_counter.setText(str(round(float(data[2])/1000, 1)))

        state_text = ['IDLE', 'BOOT', 'COMPUTE', 'SHUTDOWN', 'ABORT', 'ERROR']
        window.status_state.insert(state_text[state])
        window.dl_used.setText(id_2_mem(data[3]))
        total_data = data[3]
        window.tvc_psu.insert(str(data[5]/10))
        window.tvc_motor_current.insert(str(dyn2deg(data[4])))
        window.tvc_error.insert(hex(data[6]))
        window.tvc_temperature.insert(str(data[7]))




def transaction_cb(cmd):
    if(cmd and len(cmd) == 20):
        data = struct.unpack("i"+"iiii", bytes(cmd))
        window.trans_thrust.clear()
        window.trans_dyn_0.clear()
        window.trans_dyn_1.clear()
        window.trans_dyn_2.clear()
        window.trans_dyn_3.clear()
        window.trans_thrust.insert(str(data[0]))
        window.trans_dyn_0.insert(str(data[1]))
        window.trans_dyn_1.insert(str(data[2]))
        window.trans_dyn_2.insert(str(data[3]))
        window.trans_dyn_3.insert(str(data[4]))



    


def write_csv(file, data):
    for i, d in enumerate(data):
        file.write(str(d))
        if i == len(data)-1:
            file.write('\n')
        else:
            file.write(';')

def start_record():
    global recording
    global rec_file
    global start_rec
    if recording:
        recording=0
        window.local_record_stat.setText("")
        rec_file.close()
    else:
        recording=1
        window.local_record_stat.setText("Recording...")
        fn = window.local_record_fn.text()
        if(fn == ''):
            fn = 'local'
        num = 0
        fnam = "{}{}.csv".format(fn, num);
        while(os.path.isfile(fnam)):
            num += 1
            fnam = "{}{}.csv".format(fn, num);
        rec_file = open(fnam, 'w')
        write_csv(rec_file, data_labels)


def record_sample(data):
    if recording:
        write_csv(rec_file, data)
        data_sampled = 0

def download_trig():
    global rem_file
    fn = window.dl_name.text()
    if(fn == ''):
        fn = 'remote'
    num = 0
    fnam = "{}{}.csv".format(fn, num);
    while(os.path.isfile(fnam)):
        num += 1
        fnam = "{}{}.csv".format(fn, num);
    rem_file = open(fnam, 'w')
    write_csv(rem_file, remote_labels)
    serial_worker.download()


def download_cb(data, cnt):
    progress = cnt/total_data*100
    print(progress)
    print(data)
    window.dl_bar.setValue(progress)
    for d in data:
        if(d[0] != 0xffff):
            write_csv(rem_file, d)
    if(cnt > total_data):
        rem_file.close()



class Serial_worker(QObject):
    update_status_sig = Signal(list) #status
    connect_sig = Signal(str)
    download_sig = Signal(list, int)
    transaction_sig = Signal(list)

    def __init__(self):
        QObject.__init__(self)
        self.msv2 = msv2.msv2()
        self.downloading = 0

    @Slot(str)
    def ser_connect(self, port):
        if(self.msv2.connect(port)):
            self.connect_sig.emit("CONNECTED")
        else:
            self.connect_sig.emit("ERROR")
    @Slot()
    def ser_disconnect(self):
        if(self.msv2.disconnect()):
            self.connect_sig.emit("DISCONNECTED")
        else:
            self.connect_sig.emit("ERROR")

    @Slot(int, list)
    def send_generic(self, opcode, data):
        if self.msv2.is_connected():
            resp = self.msv2.send(opcode, data)
            print("generic: ",'[{}]'.format(', '.join(hex(x) for x in resp)))
            if opcode == TRANSACTION:
                self.transaction_sig.emit(resp)

    @Slot()
    def download(self):
        if self.msv2.is_connected():
            last_recv = 0
            err_counter = 0
            self.downloading = 1
            while 1:
                bin_data = struct.pack("I", last_recv)
                recv_data = []
                data = self.msv2.send(DOWNLOAD, bin_data)
                if(not data):
                    err_counter += 1
                    if(err_counter > 10):
                        last_recv += 1
                        err_counter = 0
                    continue
                for i in range(5):
                    tmp_data = struct.unpack("HhhhiiiIBBHi", bytes(data[i*32:(i+1)*32]))
                    #print(bytes(data[i*32:(i+1)*32]))
                    recv_data.append(tmp_data)
                last_recv += 5
                self.download_sig.emit(recv_data, last_recv)
                if(last_recv > total_data):
                    self.downloading = 0
                    break
               



    @Slot()
    def send_ping(self):
        if self.msv2.is_connected() and not self.downloading:
            stat = self.msv2.send(GET_STAT, [0x00, 0x00])
            if stat == -1 or stat==0:
                self.connect_sig.emit("RECONNECTING...")
            else:
                self.connect_sig.emit("CONNECTED")
            self.update_status_sig.emit(stat)

    @Slot()
    def start_ping(self, period):
        self.timer = QTimer()
        self.timer.timeout.connect(self.send_ping)
        self.timer.start(period)





def clean_quit():
    worker_thread.quit()
    sys.exit(0)


if __name__ == "__main__":
    app = QApplication(sys.argv)

    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    ui_file_name = "mainwindow.ui"
    ui_file = QFile(ui_file_name)
    if not ui_file.open(QIODevice.ReadOnly):
        print("Cannot open {}: {}".format(ui_file_name, ui_file.errorString()))
        sys.exit(-1)
    loader = QUiLoader()
    window = loader.load(ui_file)
    ui_file.close()
    if not window:
        print(loader.errorString())
        sys.exit(-1)

    res = None

    if platform.system() == 'Darwin':
        dev_dir = os.listdir('/dev');
        res = None
        for d in dev_dir:
            res = re.search(r'cu.usbmodem[0-9]', d), d
            if(res[0] is not None):
                res = "/dev/{}".format(d)
                break

    if(res is not None):
        COM_PORT = res
    else:
        COM_PORT = 'COM4'

    print(type(COM_PORT))
    if(type(COM_PORT)==str):
        window.connect_device.clear()
        window.connect_device.insert(COM_PORT)

    #CONNECT THREADED CALLBACKS

    #create worker and thread
    worker_thread = QThread()
    serial_worker = Serial_worker()
    serial_worker.moveToThread(worker_thread)

    #CONNECT CB SIGNALS


    serial_worker.connect_sig.connect(connect_cb)
    serial_worker.update_status_sig.connect(ping_cb)
    serial_worker.download_sig.connect(download_cb)
    serial_worker.transaction_sig.connect(transaction_cb)

    #start worker thread
    worker_thread.start()
    #worker_thread.setPriority(QThread.TimeCriticalPriority)

    serial_worker.start_ping(HEART_BEAT)




    window.connect_btn.clicked.connect(connect_trig)
    window.status_abort.clicked.connect(abort_trig)
    window.status_recover.clicked.connect(recover_trig)
    window.status_boot.clicked.connect(boot_trig)
    window.status_shutdown.clicked.connect(shutdown_trig)

    window.quit.clicked.connect(clean_quit)
    window.local_record.clicked.connect(start_record)
    window.download.clicked.connect(download_trig)
    window.tvc_motor_move.clicked.connect(tvc_motor_move_trig)

    window.trans_btn.clicked.connect(transaction_trig)

    transaction_timer = QTimer()
    transaction_timer.timeout.connect(transaction_trig)
    transaction_timer.start(0.5)


    window.show()

    sys.exit(app.exec_())
