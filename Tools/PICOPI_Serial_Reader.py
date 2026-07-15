from machine import UART, Pin
import time

uart = UART(0, baudrate=4800, tx=Pin(0), rx=Pin(1))

print("Pico UART bridge started")

while True:
    if uart.any():
        data = uart.read()
        if data:
            try:
                print(data.decode(), end="")
            except UnicodeError:
                print(data)
    time.sleep_ms(10)