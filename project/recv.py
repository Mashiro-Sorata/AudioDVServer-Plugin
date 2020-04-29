import socket
import time
import struct

host='127.0.0.1'
port = 5050
addr=(host,port)

tcpCliSock=socket.socket(socket.AF_INET,socket.SOCK_STREAM)

tcpCliSock.connect(addr)
while True:
    data=tcpCliSock.recv(512)
    if not data:
        break;
    print(struct.unpack('128f',data))

