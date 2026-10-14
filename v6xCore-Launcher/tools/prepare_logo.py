#!/usr/bin/env python3
"""Optional asset rebuild: pip install Pillow numpy.
Fits each original animation frame into the header mark without clipping its zoom.
"""
from pathlib import Path
from PIL import Image
import numpy as np

ROOT = Path(__file__).resolve().parents[1] / "assets"
image = Image.open(ROOT / "v6x_loading.gif")
size, columns, count = 128, 16, image.n_frames
atlas = Image.new("RGBA", (size * columns, size * ((count + columns - 1) // columns)))
durations = []
for index in range(count):
    image.seek(index)
    durations.append(max(20, image.info.get("duration", 30)))
    original = image.convert("RGB")
    pixels = np.asarray(original)
    bright = pixels.max(axis=2) > 100
    # Count-thresholds remove isolated background stars without changing the mark.
    xs = np.where(bright.sum(axis=0) >= 5)[0]
    ys = np.where(bright.sum(axis=1) >= 5)[0]
    if len(xs) and len(ys):
        cx, cy = (int(xs[0]) + int(xs[-1])) / 2, (int(ys[0]) + int(ys[-1])) / 2
        side = max(int(xs[-1]) - int(xs[0]), int(ys[-1]) - int(ys[0])) * 1.32
        crop = (round(cx - side / 2), round(cy - side / 2), round(cx + side / 2), round(cy + side / 2))
    else:
        crop = (100, 100, 412, 412)
    frame = original.crop(crop).resize((size, size), Image.Resampling.LANCZOS)
    values = np.asarray(frame, dtype=np.float32)
    alpha = np.clip((values.max(axis=2) - 15) / 45, 0, 1)
    frame = Image.fromarray(np.dstack((values, alpha * 255)).astype("uint8"))
    atlas.paste(frame, ((index % columns) * size, (index // columns) * size))
    if index == 0:
        frame.save(ROOT / "logo.png")
        frame.resize((256, 256), Image.Resampling.LANCZOS).save(
            ROOT / "v6xCore.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
atlas.save(ROOT / "logo-atlas.png", optimize=True)
(ROOT / "logo_frames.hpp").write_text(
    "#pragma once\n#include <array>\nnamespace v6xLogo {\n"
    f"inline constexpr int width=128, height=128, columns=16, frames={count};\n"
    f"inline constexpr std::array<unsigned,{count}> durations = {{{','.join(map(str,durations))}}};\n"
    f"inline constexpr unsigned duration={sum(durations)};\n}}\n", encoding="utf-8")
print(f"Prepared {count} fitted logo frames, {sum(durations)} ms; original GIF unchanged.")
