import socket
import sys
import time

port = int(sys.argv[1])
count = int(sys.argv[2])
socks = []
for i in range(count):
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=5)
        s.sendall(f"held client {i}\n".encode())
        socks.append(s)
    except Exception as e:
        print("connect failed:", e)
time.sleep(4)
for s in socks:
    s.close()
print(f"opened {len(socks)} connections")
