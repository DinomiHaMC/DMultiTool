#!/usr/bin/env python3
"""Prepare a photo or silent MJPEG video for DMultiTool. SPDX-License-Identifier: GPL-3.0-only"""
import argparse
import os
import tempfile
from pathlib import Path
import shutil
import subprocess


def command(source, destination, width=240, height=268, fps=10, quality=5):
    source, destination = Path(source).resolve(), Path(destination).resolve()
    if not source.is_file():
        raise ValueError('Input file does not exist')
    if destination.exists() or source.resolve() == destination.resolve():
        raise ValueError('Output must be a new file')
    if not (16 <= width <= 320 and 16 <= height <= 320 and 2 <= fps <= 20 and 2 <= quality <= 15):
        raise ValueError('Size 16..320, FPS 2..20, quality 2..15')
    suffix = destination.suffix.lower()
    if suffix not in ('.mjpeg', '.mjpg', '.jpg', '.jpeg', '.bmp'):
        raise ValueError('Use .mjpeg/.mjpg video or .jpg/.jpeg/.bmp photo')
    scale = f'scale={width}:{height}:force_original_aspect_ratio=decrease:force_divisible_by=2'
    args = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-nostdin', '-n', '-i', str(source), '-an']
    if suffix in ('.mjpeg', '.mjpg'):
        args += ['-vf', scale + f',fps={fps}', '-c:v', 'mjpeg', '-pix_fmt', 'yuvj420p', '-q:v', str(quality), '-f', 'mjpeg']
    else:
        args += ['-frames:v', '1', '-vf', scale]
        args += ['-c:v', 'bmp', '-pix_fmt', 'bgr24'] if suffix == '.bmp' else ['-c:v', 'mjpeg', '-pix_fmt', 'yuvj420p', '-q:v', str(quality)]
    return args + [str(destination)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--width', type=int, default=240)
    parser.add_argument('--height', type=int, default=268)
    parser.add_argument('--fps', type=int, default=10)
    parser.add_argument('--quality', type=int, default=5, help='2..15; lower is better')
    args = parser.parse_args()
    try:
        if not shutil.which('ffmpeg'):
            raise ValueError('Install ffmpeg first')
        invocation = command(args.source, args.destination, args.width, args.height, args.fps, args.quality)
        sidecar = Path(str(args.destination) + '.fps')
        video = args.destination.suffix.lower() in ('.mjpeg', '.mjpg')
        if video and sidecar.exists():
            raise ValueError('FPS sidecar already exists')
        # Publish exclusively: a competing writer must never lose its file.
        with tempfile.TemporaryDirectory(prefix='.dmt-media-', dir=args.destination.resolve().parent) as temporary:
            converted = Path(temporary) / ('converted' + args.destination.suffix.lower())
            invocation[-1] = str(converted)
            result = subprocess.run(invocation, check=False)
            if result.returncode or not converted.is_file() or not converted.stat().st_size:
                raise ValueError(f'ffmpeg failed ({result.returncode})')
            os.link(converted, args.destination)
        if video:
            try:
                with sidecar.open('x') as stream:
                    stream.write(str(args.fps) + '\n')
            except OSError as error:
                raise ValueError(f'Video saved, but FPS sidecar could not be created: {error}') from error
        print(args.destination)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
