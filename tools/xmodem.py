
import termios
import os
import sys
import time
import getopt
from xmodem import XMODEM
import logging

first_send = True
port = None

# Map from the numbers to the termios constants (which are pretty much
# the same numbers).

BPS_SYMS = {
    4800: termios.B4800,
    9600: termios.B9600,
    19200: termios.B19200,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
    500000: termios.B500000,
    1000000: termios.B1000000
}


# Indices into the termios tuple.

IFLAG = 0
OFLAG = 1
CFLAG = 2
LFLAG = 3
ISPEED = 4
OSPEED = 5
CC = 6


def bps_to_termios_sym(bps):
    return BPS_SYMS[bps]


class SerialPort(object):
    """Represents a serial port connected to an Arduino."""
    def __init__(self, serialport, bps):
        """Takes the string name of the serial port (e.g.
        "/dev/tty.usbserial","COM1") and a baud rate (bps) and connects to
        that port at that speed and 8N1. Opens the port in fully raw mode
        so you can send binary data.
        """
        self.fd = os.open(serialport, os.O_RDWR | os.O_NOCTTY | os.O_NDELAY)
        attrs = termios.tcgetattr(self.fd)
        bps_sym = bps_to_termios_sym(bps)
        # Set I/O speed.
        attrs[ISPEED] = bps_sym
        attrs[OSPEED] = bps_sym

        # 8N1
        attrs[CFLAG] &= ~termios.PARENB
        attrs[CFLAG] &= ~termios.CSTOPB
        attrs[CFLAG] &= ~termios.CSIZE
        attrs[CFLAG] |= termios.CS8
        # No flow control
        attrs[CFLAG] &= ~termios.CRTSCTS

        # Turn on READ & ignore contrll lines.
        attrs[CFLAG] |= termios.CREAD | termios.CLOCAL
        # Turn off software flow control.
        attrs[IFLAG] &= ~(termios.IXON | termios.IXOFF | termios.IXANY)

        # Make raw.
        attrs[LFLAG] &= ~(termios.ICANON |
                          termios.ECHO |
                          termios.ECHOE |
                          termios.ISIG)
        attrs[OFLAG] &= ~termios.OPOST

        # It's complicated--See
        # http://unixwiz.net/techtips/termios-vmin-vtime.html
        attrs[CC][termios.VMIN] = 0
        attrs[CC][termios.VTIME] = 5
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)

    def read(self):
        try:
            return os.read(self.fd, 1)
        except BlockingIOError:
            return None

    def read_until(self, num):
        buf = str.encode("")
        done = False
        i = 0
        while True:
            try:
                n = os.read(self.fd, 1)
            except BlockingIOError:
                time.sleep(0.01)
                continue
            if n == '':
                # FIXME: Maybe worth blocking instead of busy-looping?
                time.sleep(0.01)
                continue
            buf = buf + n
            i = i + 1
            if num == i:
                break
            
        return buf

    def write(self, str):
        os.write(self.fd, str)

    def write_byte(self, byte):
        os.write(self.fd, byte)

def getc(size, timeout=1):
    c = port.read_until(size)
    return c or None

def putc(data, timeout=1):
    global first_send

    c = port.write_byte(bytes(data))  # note that this ignores the timeout

    #for d in data:
    #    print(int(d), "", end="")
    #print() 

def purge_serial_in(abortchar=None):
    while True:
        c = port.read()
        if c in [None, ""]:
            break
        if abortchar != None and c == str.encode(abortchar):
            break
        else:
            try:
                print(c.decode("ascii"), end="")
            except UnicodeDecodeError:
                pass
        time.sleep(0.01)
    


def transmit_status_cb(total_packets, success_count, error_count):
    print(".", end='', flush=True)

def main(args):
    global port
    bps = 115200
    port = None
    sendfile = None
    sendfilename = None


    try:
        optlist, args = getopt.getopt(
            args[1:], 'hp:b:s:f:',
            ['help', 'port=', 'baud=', 'send=', 'file='])
        for (o, v) in optlist:
            if o == '-f' or o == '--file':
                sendfile = open(v, "rb")
                sendfilename = "".join(v.split("/")[-1:])
            elif o == '-b' or o == '--baud':
                bps = int(v)
            elif o == '-p' or o == '--port':
                port = SerialPort(v, bps)


        purge_serial_in()
        print("sending ", sendfilename)

        for char in str.encode("rx {}\n".format("".join(sendfilename.split("/")[-1:]))):
            port.write_byte(bytes([char]))
            time.sleep(0.01)
        time.sleep(1)
        purge_serial_in(abortchar='C')
        modem = XMODEM(getc, putc, purge_serial_in)
        
        
        starttime = time.time()
        modem.send(sendfile, retry=15, callback=transmit_status_cb)
        print("average transfer speed was : {:.2f} KB/s".format((os.fstat(sendfile.fileno()).st_size / 1024) / (time.time() - starttime)))
        purge_serial_in()
        sys.exit(0)

    except getopt.GetoptError as e:
        sys.stderr.write('%s: %s\n' % (args[0], e.msg))
        usage()
        sys.exit(1)

logging.basicConfig(stream=sys.stdout, level=logging.INFO)
if __name__ == '__main__':
    main(sys.argv)