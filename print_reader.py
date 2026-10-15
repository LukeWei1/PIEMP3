"""
Reads serial port messages for debugging.
Serial Monitor in the Arduino IDE also works but this is more customizable.
"""
import serial
import time

# Setup the connection to the serial port.
ARDUINO_COM_PORT = "COM7"
BAUD_RATE = 9600
serialPort = serial.Serial(ARDUINO_COM_PORT, BAUD_RATE, timeout=1)

# Read the data as it comes in and print it to the terminal.
while True:
    lineOfData = serialPort.readline().decode()
    if len(lineOfData) > 0:
        print(lineOfData)

        # Delay after each message if they're coming in too fast to read.
        time.sleep(0.5)
