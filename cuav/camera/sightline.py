import socket
from threading import Thread
import time

SEND_PORT = 14001
RECEIVE_PORT = 14002

class Connector(object):
    def __init__(self, sightline_ip_address, data_received_callback):
        self.shut_down = False
        self.data_received_callback = data_received_callback

        self.send_sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        self.recv_sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        self.send_ip = sightline_ip_address
        self.listen_thread_handle = Thread(target=self.listen_thread)

    def listen_thread(self):
        while not self.shut_down:
            data, addr = self.recv_sock.recvfrom(1024)
            if data is not None:
                data = bytearray(data)
                if data[0] == 0x51 and data[1] == 0xAC:
                    offset = 2
                    packet_len = data[offset]
                    offset += 1
                    if packet_len > 127:
                        packet_len = data[offset]
                        offset += 1
                    ck = CRC.computeCheckSum(data[offset:len(data) - 1])
                    if ck != data[offset + packet_len - 1]:
                        print "bad checksum: {0}".format(data)
                        continue
                    self.data_received_callback(data[offset:len(data) - 1])


    def start(self):
        try:
            self.recv_sock.bind(('', RECEIVE_PORT))
            self.listen_thread_handle.start()
        except Exception as ex:
            pass

    def send(self, data):
        self.send_sock.sendto(data, (self.send_ip, SEND_PORT))


class CRC(object):
    crc8Table = bytearray([0,  94, 188, 226,  97,  63, 221, 131, 194, 156, 126,  32, 163, 253,  31,  65,
  157, 195,  33, 127, 252, 162,  64,  30,  95,   1, 227, 189,  62,  96, 130, 220,
   35, 125, 159, 193,  66,  28, 254, 160, 225, 191,  93,   3, 128, 222,  60,  98,
  190, 224,   2,  92, 223, 129,  99,  61, 124,  34, 192, 158,  29,  67, 161, 255,
   70,  24, 250, 164,  39, 121, 155, 197, 132, 218,  56, 102, 229, 187,  89,   7,
  219, 133, 103,  57, 186, 228,   6,  88,  25,  71, 165, 251, 120,  38, 196, 154,
  101,  59, 217, 135,   4,  90, 184, 230, 167, 249,  27,  69, 198, 152, 122,  36,
  248, 166,  68,  26, 153, 199,  37, 123,  58, 100, 134, 216,  91,   5, 231, 185,
  140, 210,  48, 110, 237, 179,  81,  15,  78,  16, 242, 172,  47, 113, 147, 205,
   17,  79, 173, 243, 112,  46, 204, 146, 211, 141, 111,  49, 178, 236,  14,  80,
  175, 241,  19,  77, 206, 144, 114,  44, 109,  51, 209, 143,  12,  82, 176, 238,
   50, 108, 142, 208,  83,  13, 239, 177, 240, 174,  76,  18, 145, 207,  45, 115,
  202, 148, 118,  40, 171, 245,  23,  73,   8,  86, 180, 234, 105,  55, 213, 139,
   87,   9, 235, 181,  54, 104, 138, 212, 149, 203,  41, 119, 244, 170,  72,  22,
  233, 183,  85,  11, 136, 214,  52, 106,  43, 117, 151, 201,  74,  20, 246, 168,
  116,  42, 200, 150,  21,  75, 169, 247, 182, 232,  10,  84, 215, 137, 107,  53])

    @classmethod
    def computeCheckSum(cls, data):
        crc = 1
        for val in data:
            crc = CRC.crc8Table[crc ^ val]
        return crc

class BaseCommand(object):

    def create_header(self):
        return bytearray([0x51,0xAC])

    def generate_command(self, payload):
        cmd = self.create_header()
        #length of packet
        length = len(payload) + 1
        if length < 128:
            cmd.append(length)
        else:
            cmd.append(128)
            cmd.append(length - 127)
        cmd = cmd + payload
        cksum = CRC.computeCheckSum(payload)
        cmd.append(cksum)
        return cmd

    def get_parameters(self, param_id):
        return self.generate_command(bytearray([0x28, param_id]))

    def get_version(self):
        # 0 is the parameter id for getting the version number
        return self.get_parameters(0)

class Snapshot(BaseCommand):
    def __init__(self, host_ip, user, pwd):
        self.host = host_ip
        self.user = user
        self.password = pwd

    def set_parameters(self):
        buf = bytearray([0x5e]) # 0x5e code for SetSnapShot
        buf.append(0) # 0=ftp server; 1=sd card
        buf.append(1) # 0=jpg; 1=png
        buf.append(1) # 1=captured image; 2=display image
        buf.append(100) # Compression quality level 1 - 100;  png is a lossless compression
        buf.append(1) # downsample 1=full resolution; 2=2x2 downsample; 4=4x4 downsample

        ip_parts = self.host.split('.')
        buf.append(int(ip_parts[0]))
        buf.append(int(ip_parts[1]))
        buf.append(int(ip_parts[2]))
        buf.append(int(ip_parts[3]))
        buf.append(0) #server port MSB
        buf.append(21) #server port LSB
        buf.append(len(self.user)) # length of user name
        buf = buf + bytearray(self.user) # user name
        buf.append(len(self.password)) #length of password
        buf = buf + bytearray(self.password) # password
        return self.generate_command(buf)

    def do_snapshot(self, file_name):
        buf = bytearray([0x60]) # 0x60 code for DoSnapShot
        buf.append(1) # frame step
        buf.append(1) # number of frame snapshots to take
        buf.append(len(file_name)) # length of file name
        buf = buf + bytearray(file_name)
        return self.generate_command(buf)

def print_output(data):
    print "data received: ".format(data)

if __name__ == '__main__':
    con = Connector('10.0.107.56', print_output)
    con.start()

    buf = BaseCommand().get_version()
    con.send(buf)

    ss = Snapshot('10.0.107.90', 'anonymous', 'odroid')
    con.send(ss.set_parameters())

    con.send(ss.do_snapshot('images/pythontest'))
    time.sleep(5)

    pass
