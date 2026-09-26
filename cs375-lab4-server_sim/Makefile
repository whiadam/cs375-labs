# Lab 4: Client-Server Using Fork
# Build everything:  make
# Clean:             make clean

CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -O2

all: server client server_cpp client_cpp dos_sim_local

server:        server.c        ; $(CC)  $(CFLAGS) $< -o $@
client:        client.c        ; $(CC)  $(CFLAGS) $< -o $@
server_cpp:    server.cpp      ; $(CXX) $(CFLAGS) $< -o $@
client_cpp:    client.cpp      ; $(CXX) $(CFLAGS) $< -o $@
dos_sim_local: dos_sim_local.c ; $(CC)  $(CFLAGS) $< -o $@

clean:
	rm -f server client server_cpp client_cpp dos_sim_local *.log
