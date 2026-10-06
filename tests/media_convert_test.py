"""PC converter integration: actual ffmpeg, exclusive publication and errors."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
import shutil

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('media_convert', ROOT / 'tools/media_convert.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

@unittest.skipUnless(shutil.which('ffmpeg'), 'ffmpeg required')
class ConverterTest(unittest.TestCase):
    def invoke(self, source, destination, *options):
        return subprocess.run(['python3', str(ROOT / 'tools/media_convert.py'), str(source), str(destination), *options], capture_output=True)

    def test_video_and_photo(self):
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder) / 'clip.mjpeg'
            source = Path(folder) / 'input.mp4'
            subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-nostdin', '-i', str(ROOT / 'examples/sd/media/demo.mjpeg'), '-c:v', 'mpeg4', str(source)], check=True)
            result = self.invoke(source, out, '--fps', '6')
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertTrue(out.read_bytes().startswith(b'\xff\xd8'))
            self.assertEqual(Path(str(out) + '.fps').read_text(), '6\n')
            result = self.invoke(out, Path(folder) / 'photo.bmp')
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual((Path(folder) / 'photo.bmp').read_bytes()[:2], b'BM')
            photo = Path(folder) / 'photo.jpg'
            self.assertEqual(self.invoke(source, photo).returncode, 0)
            self.assertTrue(photo.read_bytes().startswith(b'\xff\xd8'))
            original = out.read_bytes()
            self.assertNotEqual(self.invoke(out, out).returncode, 0)
            self.assertEqual(out.read_bytes(), original)
            self.assertFalse(list(Path(folder).glob('.dmt-media-*')))

    def test_invalid_input_preserves_files(self):
        with tempfile.TemporaryDirectory() as folder:
            source, out = Path(folder) / 'bad.mp4', Path(folder) / 'video.mjpeg'
            source.write_bytes(b'broken')
            self.assertNotEqual(self.invoke(source, out).returncode, 0)
            self.assertFalse(out.exists())
            out.write_bytes(b'keep')
            self.assertNotEqual(self.invoke(source, out).returncode, 0)
            self.assertEqual(out.read_bytes(), b'keep')
            out.unlink()
            sidecar = Path(str(out) + '.fps')
            sidecar.write_text('keep')
            self.assertNotEqual(self.invoke(source, out).returncode, 0)
            self.assertEqual(sidecar.read_text(), 'keep')
            self.assertFalse(out.exists())

    def test_parameters(self):
        source = ROOT / 'tests/media-fixtures/corners.jpg'
        for options in ({'width':0}, {'fps':21}, {'quality':1}):
            with self.assertRaises(ValueError):
                module.command(source, '/tmp/nonexistent-dmt-conversion.mjpeg', **options)
        with self.assertRaises(ValueError):
            module.command(source, '/tmp/nonexistent-dmt-conversion.mp4')

if __name__ == '__main__':
    unittest.main()
