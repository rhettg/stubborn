#!/usr/bin/python3
# 
# testing out some unix domain socket stuff
# can be tested with netcat (nc)
#    echo "hello" | nc -U ./test
#
import socket
import os
import sys
import signal
import asyncio
import time
import functools
import concurrent.futures
import logging

import rfm69

def usage():
    print("usage: uds.py <name>", file=sys.stderr)
    sys.exit(1)

radio = None

def send_to_radio(radio, data):
    radio.send(data)
    logging.info("TX: {}".format(data))

def recv_from_radio(radio):
    packet = radio.receive()
    if packet is not None:
        logging.info("RX: {}".format(packet))
    return packet

async def handle_reader(reader):
    loop = asyncio.get_running_loop()
    while True:
        req = await reader.read(1024)
        if len(req) == 0:
            break

        # The radio only supports packets up to 60 bytes. It's up to the client
        # to handle it's own framing.
        if len(req) > 60:
            logging.error("client request too large ({})".format(len(req)))
            break

        with concurrent.futures.ThreadPoolExecutor() as pool:
            await loop.run_in_executor(pool, functools.partial(send_to_radio, radio, req))

    logging.info("handle_reader: done")

async def handle_writer(writer):
    loop = asyncio.get_running_loop()
    while True:
        if radio.payload_ready():
            with concurrent.futures.ThreadPoolExecutor() as pool:
                result = await loop.run_in_executor(pool, functools.partial(recv_from_radio, radio))

            writer.write(result) 
            await writer.drain()
        else:
            await asyncio.sleep(0.001)

    logging.info("handle_writer: done")

async def handle(reader, writer):
    logging.info("start handling")

    writer_task = asyncio.create_task(handle_writer(writer))
    await handle_reader(reader)
    writer_task.cancel()

    writer.close()
    await writer.wait_closed()

    logging.info("done handling")

def shutdown(loop):
    logging.info('received stop signal, shutting down...')
    for task in asyncio.all_tasks():
        task.cancel()

def setup_shutdown():
    loop = asyncio.get_running_loop()
    loop.add_signal_handler(signal.SIGHUP, functools.partial(shutdown, loop))
    loop.add_signal_handler(signal.SIGINT, functools.partial(shutdown, loop))
    loop.add_signal_handler(signal.SIGTERM, functools.partial(shutdown, loop))

def validate_args():
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

    return socketName

async def run(socketName):
    setup_shutdown()

    server = await asyncio.start_unix_server(handle, path=socketName)

    try:
        async with server:
            await server.serve_forever()
    except asyncio.CancelledError:
        pass
    logging.info("server stopped")


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)

    socketName = validate_args()

    logging.info("starting radio")
    radio = rfm69.initialize_radio()

    logging.info("starting server on {}".format(socketName))
    asyncio.run(run(socketName))
    