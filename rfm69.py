"""
Radio Interface

Author: Rhett Garber
"""
import time
import busio
from digitalio import DigitalInOut, Direction, Pull
import board

# Import the RFM69 radio module.
import adafruit_rfm69

# Create the I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# RFM69 Configuration
CS = DigitalInOut(board.D5)
RESET = DigitalInOut(board.D6)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)

def initialize_radio():
    try:
        rfm69 = adafruit_rfm69.RFM69(spi, CS, RESET, 915.0)
        print('RFM69: Detected', 0, 0, 1)
    except RuntimeError as error:
        # Thrown on version mismatch
        print('RFM69 Error: ', error)
        sys.exit(1)
    return rfm69

def do_recv(rfm69):
    packet = rfm69.receive()
    if packet is not None:
        packet_text = str(packet, "utf-8")
        print("RX: {}".format(packet_text))

def do_send_ping(rfm69):
        send_data = bytes("hello\r\n","utf-8")
        print("TX: {}", send_data)
        rfm69.send(send_data)
        last_send = time.time()
    

def main():
    rfm69 = initialize_radio()

    last_send = 0
    while True:
        loop_start = time.time()

        if rfm69.payload_ready():
            do_recv(rfm69)

        if time.time() - last_send > 1.0:
            do_send_ping(rfm69)
            last_send = time.time()

        loop_duration = time.time() - loop_start
        if loop_duration > 0.1:
            print("Loop: {}s".format(loop_duration))

        if loop_duration < 0.1:
            time.sleep(0.1)

if __name__ == '__main__':
    main()
