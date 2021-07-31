#!/usr/bin/python3
# 
# testing out some unix domain socket stuff
# can be tested with netcat (nc)
#    echo "hello" | nc -U ./test
#
import socket
import os
import sys

def usage():
    print("usage: uds.py <name>", file=sys.stderr)
    sys.exit(1)

def handle(conn):
    while True:
        req = conn.recv(1024)
        if len(req) == 0:
            break

        print("RX: {}".format(req))

def main():
    try:
        socketName = sys.argv[1]
    except IndexError:
        usage()
    try:
        os.unlink(socketName)
    except OSError:
        if os.path.exists(socketName):
            print("{} already exists".format(socketName), file=sys.stderr)
            sys.exit(1)
    
    print("opening {}".format(socketName))

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(socketName)
    sock.listen(1)

    while True:
        connection, client_address = sock.accept()
        print("accepted connection")
        handle(connection)
        print("connection complete")
        connection.close()


if __name__ == '__main__':
    main()
    