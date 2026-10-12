# Draws logo.png: a selection box with handles, a cursor dragging it, and "{ }" for the code. python tools/logo.py
from PIL import Image, ImageDraw

S = 512
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
d.rounded_rectangle((16, 16, S - 16, S - 16), radius=96, fill=(28, 32, 48, 255))

orange, cyan, white = (255, 153, 38, 255), (90, 204, 255, 255), (235, 235, 245, 255)
# The node: a button-ish rounded rect, faint fill
d.rounded_rectangle((96, 120, 368, 300), radius=28, fill=(64, 52, 50, 255))
# The selection box and its handles
d.rectangle((96, 120, 368, 300), outline=orange, width=10)
for x, y in ((96, 120), (368, 120), (96, 300), (368, 300), (232, 120), (232, 300), (96, 210), (368, 210)):
    d.rectangle((x - 14, y - 14, x + 14, y + 14), fill=orange)
# A guide line through the middle, as when snapping
d.line((232, 60, 232, 452), fill=cyan, width=6)
# "{ }" under it
d.text((70, 330), "{ }", fill=white, font_size=110)
# The cursor
cursor = [(300, 230), (300, 430), (346, 386), (378, 456), (408, 442), (376, 374), (436, 370)]
d.polygon(cursor, fill=white, outline=(28, 32, 48, 255))
img.save("logo.png")
