#!/usr/bin/env python3
"""DMultiTool microSD file transfer over paired BLE. SPDX-License-Identifier: GPL-3.0-only"""
import argparse
import asyncio
import os
import struct
import zlib
from pathlib import Path

RX = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'
TX = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'


class Link:
    def __init__(self, client):
        self.client = client
        self.packets = asyncio.Queue()
        self.text = b''

    async def start(self):
        loop = asyncio.get_running_loop()
        await self.client.start_notify(TX, lambda _, data: loop.call_soon_threadsafe(self.packets.put_nowait, bytes(data)))

    async def write(self, opcode, data=b''):
        if len(data) > 199:
            raise ValueError('Command/path must fit 199 UTF-8 bytes')
        await self.client.write_gatt_char(RX, bytes([opcode]) + data, response=True)

    async def packet(self):
        return await asyncio.wait_for(self.packets.get(), 15)

    async def line(self):
        while b'\n' not in self.text:
            packet = await self.packet()
            if not packet or packet[0] != 0x20:
                raise ValueError('Unexpected reply')
            self.text += packet[1:]
            if len(self.text) > 8192:
                raise ValueError('Reply too large')
        line, self.text = self.text.split(b'\n', 1)
        result = line.decode('utf-8', errors='replace')
        if result.startswith('ERR'):
            raise RuntimeError(result)
        return result

    async def ls(self, directory, offset=0):
        await self.write(1, f'{directory}\t{offset}'.encode())
        lines = []
        while True:
            line = await self.line()
            if line == 'END':
                return lines
            lines.append(line)

    async def get(self, remote, destination):
        destination = Path(destination)
        partial = Path(str(destination) + '.part')
        if destination.exists():
            raise FileExistsError(destination)
        # Open exclusively before requesting bytes: do not overwrite a local file.
        with partial.open('xb') as output:
            try:
                await self.write(2, remote.encode())
                line = await self.line()
                if not line.startswith('SIZE\t'):
                    raise ValueError('Missing SIZE reply')
                expected = int(line.split('\t')[1])
                received = crc = 0
                while True:
                    packet = await self.packet()
                    if packet[0] == 0x10:
                        data = packet[1:]
                        output.write(data)
                        received += len(data)
                        crc = zlib.crc32(data, crc)
                        if received > expected:
                            raise ValueError('File exceeds announced size')
                        await self.write(5)
                    elif packet[0] == 0x11 and len(packet) == 9:
                        size, remote_crc = struct.unpack('<II', packet[1:])
                        if size != expected or size != received or remote_crc != crc:
                            raise ValueError('CRC/size mismatch')
                        break
                    else:
                        raise ValueError('Transfer interrupted')
                output.flush()
                os.fsync(output.fileno())
            except BaseException:
                output.close()
                partial.unlink(missing_ok=True)
                raise
        # Atomic create without replacing a file created during the transfer.
        try:
            os.link(partial, destination)
        finally:
            partial.unlink(missing_ok=True)
        return received

    async def put(self, source, remote):
        source = Path(source)
        size = source.stat().st_size
        if size > 16 * 1024 * 1024:
            raise ValueError('Maximum file size is 16 MiB')
        await self.write(3, f'{remote}\t{size}'.encode())
        if await self.line() != 'READY':
            raise ValueError('Missing READY reply')
        crc = 0
        with source.open('rb') as stream:
            while data := stream.read(19):
                await self.write(4, data)
                if await self.line() != 'ACK':
                    raise ValueError('Missing chunk ACK')
                crc = zlib.crc32(data, crc)
        await self.write(6, struct.pack('<I', crc))
        if await self.line() != 'DONE':
            raise ValueError('Missing verified commit')
        return size


async def main(args):
    from bleak import BleakClient, BleakScanner
    address = args.address
    if not address:
        device = await BleakScanner.find_device_by_filter(lambda d, a: (a.local_name or d.name or '').startswith('DMultiTool'), timeout=15)
        if not device:
            raise RuntimeError('Open Utils → File transfer on DMultiTool first')
        address = device
    async with BleakClient(address, pair=True, timeout=30) as client:
        link = Link(client)
        await link.start()
        if args.command == 'ls':
            print('\n'.join(await link.ls(args.path, args.offset)))
        elif args.command == 'get':
            print('Received', await link.get(args.path, args.output), 'bytes (CRC verified)')
        else:
            print('Sent', await link.put(args.source, args.path), 'bytes (CRC verified)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--address', help='Bluetooth address/identifier (optional)')
    commands = parser.add_subparsers(dest='command', required=True)
    ls = commands.add_parser('ls'); ls.add_argument('path', nargs='?', default='/'); ls.add_argument('--offset', type=int, default=0)
    get = commands.add_parser('get'); get.add_argument('path'); get.add_argument('output')
    put = commands.add_parser('put'); put.add_argument('source'); put.add_argument('path')
    try:
        asyncio.run(main(parser.parse_args()))
    except (OSError, RuntimeError, ValueError, TimeoutError) as error:
        parser.exit(1, str(error) + '\n')
