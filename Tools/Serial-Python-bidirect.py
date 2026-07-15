from machine import UART, Pin
import sys
import select
import time

uart = UART(0, 4800, tx=Pin(0), rx=Pin(1))

poller = select.poll()
poller.register(sys.stdin, select.POLLIN)

while True:
    # Nodo → PC
    if uart.any():
        data = uart.read()
        if data:
            try:
                sys.stdout.write(data.decode())
            except UnicodeError:
                pass

    # PC → nodo
    if poller.poll(0):
        char = sys.stdin.read(1)
        if char:
            uart.write(char)

    time.sleep_ms(2)