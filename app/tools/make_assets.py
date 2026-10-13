#!/usr/bin/env python3
"""Draws the XBXPS3 tile and splash images (app/Assets). Needs Pillow and DejaVu Sans Bold.

Usage: make_assets.py <output_dir>
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

BG = (16, 20, 24, 255)
ACCENT = (16, 124, 16, 255)   # Xbox green
TEXT = (240, 240, 240, 255)
FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'

SIZES = {
    'StoreLogo.png': (50, 50),
    'Square44x44Logo.png': (44, 44),
    'Square150x150Logo.png': (150, 150),
    'Wide310x150Logo.png': (310, 150),
    'SplashScreen.png': (620, 300),
}


def draw(size):
    w, h = size
    img = Image.new('RGBA', size, BG)
    d = ImageDraw.Draw(img)
    bar = max(2, h // 12)
    d.rectangle([0, h - bar, w, h], fill=ACCENT)

    label = 'XBXPS3' if w >= 100 else 'X3'
    font_size = max(10, int(min(w / (len(label) * 0.68), h * 0.42)))
    font = ImageFont.truetype(FONT, font_size)
    box = d.textbbox((0, 0), label, font=font)
    tw, th = box[2] - box[0], box[3] - box[1]
    d.text(((w - tw) / 2 - box[0], (h - bar - th) / 2 - box[1]), label, font=font, fill=TEXT)
    return img


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else '.')
    out.mkdir(parents=True, exist_ok=True)
    for name, size in SIZES.items():
        draw(size).save(out / name)
        print(name, size)


if __name__ == '__main__':
    main()
