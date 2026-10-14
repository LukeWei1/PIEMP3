import serial
import time

ARDUINO_COM_PORT = "COM7"
BAUD_RATE = 9600
serialPort = serial.Serial(ARDUINO_COM_PORT, BAUD_RATE, timeout=1)

while True:
    lineOfData = serialPort.readline().decode()
    if len(lineOfData) > 0:
        print(lineOfData)
        time.sleep(0.5)
