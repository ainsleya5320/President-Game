"""Create a small garden panorama from the project's licensed HDR environment."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

here = Path(__file__).resolve().parent
src = here.parent / 'SourceAssets' / 'Textures' / 'garden.hdr'
dst = here / 'garden-view.jpg'

with src.open('rb') as fh:
    while fh.readline().strip():
        pass
    orientation = fh.readline().decode('ascii').strip().split()
    height, width = int(orientation[1]), int(orientation[3])
    pixels = np.empty((height, width, 4), dtype=np.uint8)
    for y in range(height):
        head = fh.read(4)
        if head[:2] != b'\x02\x02' or head[2] * 256 + head[3] != width:
            raise ValueError(f'Unsupported HDR scanline {y}: {head!r}')
        for channel in range(4):
            x = 0
            while x < width:
                count = fh.read(1)[0]
                if count > 128:
                    run = count - 128
                    value = fh.read(1)[0]
                    pixels[y, x:x + run, channel] = value
                else:
                    run = count
                    pixels[y, x:x + run, channel] = np.frombuffer(fh.read(run), dtype=np.uint8)
                x += run
            if x != width:
                raise ValueError(f'Bad HDR row {y} channel {channel}')

exponent = pixels[:, :, 3].astype(np.int16) - 128
linear = pixels[:, :, :3].astype(np.float32) * np.exp2(exponent[:, :, None].astype(np.float32) - 8)
linear[pixels[:, :, 3] == 0] = 0
linear = np.clip(linear, 0, 40)
mapped = 1 - np.exp(-linear * 1.35)
rgb = np.uint8(np.clip(mapped ** (1 / 2.2) * 255, 0, 255))
Image.fromarray(rgb).filter(ImageFilter.GaussianBlur(1.5)).save(dst, quality=86, optimize=True)
print(dst, dst.stat().st_size, 'linear percentiles', np.percentile(linear, [50, 90, 99]).round(3))
