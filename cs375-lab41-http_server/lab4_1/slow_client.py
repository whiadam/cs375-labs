import socket
import sys
import time

port = int(sys.argv[1])
num = sys.argv[2]
s = socket.create_connection(("127.0.0.1", port))
for part in ["hello", "bye"]:
    s.sendall(f"client {num} says {part}\n".encode())
    print(s.recv(1024).decode().strip())
    time.sleep(0.5)
s.close()
