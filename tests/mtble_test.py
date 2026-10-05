import asyncio
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

spec = importlib.util.spec_from_file_location('mtble', Path(__file__).parents[1] / 'tools/mtble.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FakeClient:
    def __init__(self, data=b'', corrupt=False):
        self.data = data
        self.corrupt = corrupt
        self.offset = 0
        self.sent = b''
        self.size = 0
        self.link = None

    def status(self, text):
        encoded = (text + '\n').encode()
        for offset in range(0, len(encoded), 19):
            self.link.packets.put_nowait(b'\x20' + encoded[offset:offset + 19])

    def next(self):
        if self.offset < len(self.data):
            chunk = self.data[self.offset:self.offset + 19]
            self.offset += len(chunk)
            self.link.packets.put_nowait(b'\x10' + chunk)
        else:
            self.link.packets.put_nowait(b'\x11' + struct.pack('<II', len(self.data), zlib.crc32(self.data) ^ int(self.corrupt)))

    async def write_gatt_char(self, uuid, packet, response):
        assert uuid == module.RX and response
        opcode, data = packet[0], packet[1:]
        if opcode == 1:
            self.status('F\t9\tnine.txt\nMORE\t64\nEND')
        elif opcode == 2:
            self.status(f'SIZE\t{len(self.data)}')
            self.next()
        elif opcode == 5:
            self.next()
        elif opcode == 3:
            self.size = int(data.decode().split('\t')[1])
            self.status('READY')
        elif opcode == 4:
            assert len(data) <= 19
            self.sent += data
            self.status('ACK')
        elif opcode == 6:
            assert self.size == len(self.sent)
            assert struct.unpack('<I', data)[0] == zlib.crc32(self.sent)
            self.status('DONE')


class TransferTests(unittest.IsolatedAsyncioTestCase):
    def link(self, data=b'', corrupt=False):
        client = FakeClient(data, corrupt)
        link = module.Link(client)
        client.link = link
        return link, client

    async def test_download_and_empty(self):
        with tempfile.TemporaryDirectory() as directory:
            for index, data in enumerate((b'', b'123456789', bytes(range(256)) * 10)):
                link, _ = self.link(data)
                output = Path(directory) / str(index)
                self.assertEqual(await link.get('/file', output), len(data))
                self.assertEqual(output.read_bytes(), data)
                self.assertFalse(Path(str(output) + '.part').exists())

    async def test_crc_and_existing_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'file'
            link, _ = self.link(b'content', corrupt=True)
            with self.assertRaises(ValueError):
                await link.get('/file', output)
            self.assertFalse(output.exists())
            self.assertFalse(Path(str(output) + '.part').exists())
            output.write_bytes(b'keep')
            with self.assertRaises(FileExistsError):
                await link.get('/file', output)
            self.assertEqual(output.read_bytes(), b'keep')
            output.unlink()
            link, client = self.link(b'content')
            original_write = client.write_gatt_char

            async def competing_file(uuid, packet, response):
                if packet[0] == 2:
                    output.write_bytes(b'created meanwhile')
                await original_write(uuid, packet, response)

            client.write_gatt_char = competing_file
            with self.assertRaises(FileExistsError):
                await link.get('/file', output)
            self.assertEqual(output.read_bytes(), b'created meanwhile')
            self.assertFalse(Path(str(output) + '.part').exists())

    async def test_upload_crc_and_listing(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'file'
            source.write_bytes(bytes(range(256)) * 5)
            link, client = self.link()
            self.assertEqual(await link.put(source, '/copy'), source.stat().st_size)
            self.assertEqual(client.sent, source.read_bytes())
            self.assertEqual(await link.ls('/'), ['F\t9\tnine.txt', 'MORE\t64'])
            with self.assertRaises(ValueError):
                await link.write(2, b'a' * 200)


if __name__ == '__main__':
    unittest.main()
