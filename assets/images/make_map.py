# Render a Pip-Boy green map (inverted OSM tiles) centred on home.
# Map data (c) OpenStreetMap contributors, ODbL.
# usage: make_map.py LAT LON ZOOM W H OUT
import io, math, sys, urllib.request
from PIL import Image, ImageDraw, ImageOps

GRID_STEP = 48

lat, lon, z, w, h, out = float(sys.argv[1]), float(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), sys.argv[6]

def world_px(lat, lon):
    n = 256 * 2**z
    x = (lon + 180) / 360 * n
    y = (1 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2 * n
    return x, y

cx, cy = world_px(lat, lon)
x0, y0 = int(cx - w / 2), int(cy - h / 2)
canvas = Image.new("RGB", (w, h))
for ty in range(y0 // 256, (y0 + h) // 256 + 1):
    for tx in range(x0 // 256, (x0 + w) // 256 + 1):
        url = f"https://tile.openstreetmap.org/{z}/{tx}/{ty}.png"
        req = urllib.request.Request(url, headers={"User-Agent": "esphome-map-render/1.0"})
        tile = Image.open(io.BytesIO(urllib.request.urlopen(req).read())).convert("RGB")
        canvas.paste(tile, (tx * 256 - x0, ty * 256 - y0))

g = ImageOps.autocontrast(ImageOps.invert(canvas.convert("L")), cutoff=1)
g = g.point(lambda v: int(255 * (v / 255) ** 1.8))  # darker background, roads/edges stay bright
# black -> Pip-Boy background, white -> a muted green so the markers stand out
img = ImageOps.colorize(g, black=(10, 15, 11), white=(80, 170, 105))
# grid and frame like the Pip-Boy local map
draw = ImageDraw.Draw(img)
GRID = (36, 67, 46)
for x in range(GRID_STEP, w, GRID_STEP):
    draw.line([(x, 0), (x, h - 1)], fill=GRID)
for y in range(GRID_STEP, h, GRID_STEP):
    draw.line([(0, y), (w - 1, y)], fill=GRID)
draw.rectangle([0, 0, w - 1, h - 1], outline=GRID)
img.save(out)
print(f"saved {out}, centre world px {cx:.1f},{cy:.1f}")
