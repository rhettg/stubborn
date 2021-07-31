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

def usage():
    print("usage: uds.py <name>", file=sys.stderr)
    sys.exit(1)

radio = None

def send_to_radio(radio, data):
    # Simulate non-async blocking io
    time.sleep(0.5)
    logging.info("TX: {}".format(data))

def recv_from_radio(radio):
    # Simulate non-async blocking io
    time.sleep(0.5)
    data = b'pong\n'
    logging.info("RX: {}".format(data))
    return data

async def handle_reader(reader):
    loop = asyncio.get_running_loop()
    while True:
        req = await reader.read(1024)
        if len(req) == 0:
            break

        with concurrent.futures.ThreadPoolExecutor() as pool:
            result = await loop.run_in_executor(pool, functools.partial(send_to_radio, radio, req))

    logging.info("handle_reader: done")

async def handle_writer(writer):
    loop = asyncio.get_running_loop()
    while True:
        with concurrent.futures.ThreadPoolExecutor() as pool:
            result = await loop.run_in_executor(pool, functools.partial(recv_from_radio, radio))

        writer.write(result) 
        await writer.drain()

    logging.info("handle_writer: done")

async def handle(reader, writer):
    logging.info("start handling")

    writer_task = asyncio.create_task(handle_writer(writer))
    await handle_reader(reader)
    writer_task.cancel()

    logging.info("done handling")

def shutdown(loop):
    logging.info('received stop signal, cancelling tasks...')
    for task in asyncio.all_tasks():
        task.cancel()

async def main():
    loop = asyncio.get_running_loop()
    loop.add_signal_handler(signal.SIGHUP, functools.partial(shutdown, loop))
    loop.add_signal_handler(signal.SIGINT, functools.partial(shutdown, loop))
    loop.add_signal_handler(signal.SIGTERM, functools.partial(shutdown, loop))

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
    
    logging.info("opening {}".format(socketName))

    server = await asyncio.start_unix_server(handle, path=socketName)
    try:
        async with server:
            await server.serve_forever()
    except asyncio.CancelledError:
        pass

if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    asyncio.run(main())
    