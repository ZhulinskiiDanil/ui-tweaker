# A GIF from tools/frames.ps1's frames, with the mouse cursor drawn in (a screen copy has none):
#   python tools/make_gif.py <frames folder> <out.gif> [width]
import os
import sys

from PIL import Image, ImageDraw

folder, out = sys.argv[1], sys.argv[2]
width = int(sys.argv[3]) if len(sys.argv) > 3 else 760
# A maximized window's client area goes under the taskbar: its bottom is cut off
cut = float(sys.argv[4]) if len(sys.argv) > 4 else 0.03
names = sorted(n for n in os.listdir(folder) if n.endswith(".png"))
cursors = [tuple(map(int, line.split())) for line in open(os.path.join(folder, "cursor.txt"))]

frames = []
for name, (cx, cy) in zip(names, cursors):
    frame = Image.open(os.path.join(folder, name)).convert("RGB")
    draw = ImageDraw.Draw(frame)
    s = frame.size[0] / 1280 * 1.6
    arrow = [(0, 0), (0, 17), (4, 13), (7, 20), (10, 19), (7, 12), (12, 12)]
    draw.polygon([(cx + x * s, cy + y * s) for x, y in arrow], fill=(255, 255, 255), outline=(0, 0, 0))
    frame = frame.crop((0, 0, frame.size[0], round(frame.size[1] * (1 - cut))))
    height = round(frame.size[1] * width / frame.size[0])
    frames.append(frame.resize((width, height), Image.LANCZOS))

# One palette for all, so the GIF stays small and the colors don't flicker; made from frames across the whole
# of it, as the main menu's background goes through every hue
picks = frames[:: max(1, len(frames) // 8)]
sheet = Image.new("RGB", (frames[0].size[0], frames[0].size[1] * len(picks)))
for i, frame in enumerate(picks):
    sheet.paste(frame, (0, frames[0].size[1] * i))
palette = sheet.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
quantized = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
quantized[0].save(out, save_all=True, append_images=quantized[1:], duration=90, loop=0, optimize=True)
print(out, len(frames), "frames", os.path.getsize(out) // 1024, "KB")
