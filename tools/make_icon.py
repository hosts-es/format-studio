"""生成 Format Studio 的应用图标（PNG + ICO）。

构图：左上 / 右下 两张白色文件卡，外围是绿色与蓝色的两段半环箭头，
表示"格式转换"的循环。深色圆角方块作底，适配 Windows 任务栏与资源管理器。

用法：
    python tools/make_icon.py
输出：
    icons/app.png            1024x1024
    icons/app.ico            多尺寸（16..256）
    icons/app-preview.png    深/浅背景预览
"""

import math
import os

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICON_DIR = os.path.join(ROOT, "icons")
SIZE = 1024

BG_TOP = (27, 36, 54)
BG_BOTTOM = (13, 18, 30)
CARD = (248, 250, 255)
CARD_EDGE = (214, 224, 240)
LINE = (196, 209, 228)
GREEN = (47, 212, 128)
BLUE = (59, 130, 246)


def rounded_mask(size, radius):
    mask = Image.new("L", size, 0)
    d = ImageDraw.Draw(mask)
    d.rounded_rectangle([0, 0, size[0] - 1, size[1] - 1], radius=radius, fill=255)
    return mask


def vertical_gradient(size, top, bottom):
    img = Image.new("RGB", (1, size[1]))
    d = ImageDraw.Draw(img)
    for y in range(size[1]):
        t = y / max(1, size[1] - 1)
        d.point((0, y), fill=(
            round(top[0] + (bottom[0] - top[0]) * t),
            round(top[1] + (bottom[1] - top[1]) * t),
            round(top[2] + (bottom[2] - top[2]) * t),
        ))
    return img.resize(size, Image.BICUBIC)


def tangent_at(deg):
    a = math.radians(deg)
    return (-math.sin(a), math.cos(a))


def ring_polygon(cx, cy, radius, width, start_deg, end_deg, steps=260):
    outer, inner = [], []
    for i in range(steps + 1):
        a = math.radians(start_deg + (end_deg - start_deg) * i / steps)
        ca, sa = math.cos(a), math.sin(a)
        outer.append((cx + (radius + width / 2) * ca, cy + (radius + width / 2) * sa))
        inner.append((cx + (radius - width / 2) * ca, cy + (radius - width / 2) * sa))
    return outer + inner[::-1]


def draw_ring(draw, cx, cy, radius, width, start_deg, end_deg, color, s):
    """s 为超采样倍数，所有坐标都已按 s 放大。"""
    poly = ring_polygon(cx * s, cy * s, radius * s, width * s, start_deg, end_deg)
    draw.polygon(poly, fill=color)
    # 两端的圆头
    for deg in (start_deg, end_deg):
        a = math.radians(deg)
        px = (cx + radius * math.cos(a)) * s
        py = (cy + radius * math.sin(a)) * s
        r = width / 2 * s
        draw.ellipse([px - r, py - r, px + r, py + r], fill=color)


def draw_arrow_head(draw, cx, cy, radius, width, deg, color, s,
                    length=118, half_width=64):
    a = math.radians(deg)
    bx = (cx + radius * math.cos(a)) * s
    by = (cy + radius * math.sin(a)) * s
    dx, dy = tangent_at(deg)
    px, py = -dy, dx
    tip = (bx + dx * length * s, by + dy * length * s)
    left = (bx + px * half_width * s, by + py * half_width * s)
    right = (bx - px * half_width * s, by - py * half_width * s)
    draw.polygon([tip, left, right], fill=color)


def make_card(width, height, radius=30):
    card = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    d = ImageDraw.Draw(card)
    d.rounded_rectangle([0, 0, width - 1, height - 1], radius=radius, fill=CARD,
                        outline=CARD_EDGE, width=3)

    line_x0, line_x1 = 38, width - 38

    # 顶部一条细色条，暗示文件类型
    d.rounded_rectangle([line_x0, 44, line_x0 + 54, 62], radius=9, fill=BLUE)

    # 文件内容的三行"文字"
    for y, ratio in ((96, 1.0), (146, 0.86), (196, 0.62)):
        d.rounded_rectangle([line_x0, y, line_x0 + (line_x1 - line_x0) * ratio, y + 16],
                            radius=8, fill=LINE)
    return card


def main():
    os.makedirs(ICON_DIR, exist_ok=True)

    # 圆角底板
    canvas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    board = vertical_gradient((SIZE, SIZE), BG_TOP, BG_BOTTOM).convert("RGBA")
    board.putalpha(rounded_mask((SIZE, SIZE), 226))
    canvas.alpha_composite(board)

    # 内描边，深色背景下有一点点层次
    edge = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    ImageDraw.Draw(edge).rounded_rectangle([10, 10, SIZE - 11, SIZE - 11], radius=216,
                                           outline=(255, 255, 255, 26), width=3)
    canvas.alpha_composite(edge)

    # ── 两张文件卡（左上、右下） ──────────────────────────────
    card_a = make_card(250, 320).rotate(-13, expand=True, resample=Image.BICUBIC)
    card_b = make_card(250, 320).rotate(10, expand=True, resample=Image.BICUBIC)
    pos_a = (352, 356)
    pos_b = (676, 664)

    # 阴影
    shadow = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    for card, pos in ((card_a, pos_a), (card_b, pos_b)):
        sil = Image.new("RGBA", card.size, (0, 0, 0, 0))
        sil.paste((0, 0, 0, 130), (0, 0), card.split()[3])
        shadow.alpha_composite(sil, (int(pos[0] - card.width / 2), int(pos[1] - card.height / 2) + 14))
    shadow = shadow.filter(ImageFilter.GaussianBlur(22))
    canvas.alpha_composite(shadow)

    # ── 半环箭头（4 倍超采样，边缘平滑） ──────────────────────
    cx = cy = SIZE / 2
    radius = 300
    stroke = 58
    ss = 4
    ring = Image.new("RGBA", (SIZE * ss, SIZE * ss), (0, 0, 0, 0))
    rd = ImageDraw.Draw(ring)

    # 绿色：从左上方绕到右侧
    draw_ring(rd, cx, cy, radius, stroke, 250, 348, GREEN, ss)
    draw_arrow_head(rd, cx, cy, radius, stroke, 348, GREEN, ss)

    # 蓝色：从右下方绕到左侧
    draw_ring(rd, cx, cy, radius, stroke, 70, 168, BLUE, ss)
    draw_arrow_head(rd, cx, cy, radius, stroke, 168, BLUE, ss)

    ring = ring.resize((SIZE, SIZE), Image.LANCZOS)
    canvas.alpha_composite(ring)

    # 文件压在环上，形成"环绕"的层次
    for card, pos in ((card_a, pos_a), (card_b, pos_b)):
        canvas.alpha_composite(card, (int(pos[0] - card.width / 2), int(pos[1] - card.height / 2)))

    png_path = os.path.join(ICON_DIR, "app.png")
    canvas.save(png_path)

    ico_path = os.path.join(ICON_DIR, "app.ico")
    canvas.save(ico_path, format="ICO",
                sizes=[(16, 16), (20, 20), (24, 24), (32, 32), (40, 40),
                       (48, 48), (64, 64), (128, 128), (256, 256)])

    # 预览：深色 / 浅色背景
    preview = Image.new("RGB", (SIZE + 40, 520), (240, 243, 248))
    preview.paste((22, 26, 34), (0, 0, SIZE + 40, 500))
    big = canvas.resize((440, 440), Image.LANCZOS)
    preview.paste(big, (40, 30), big)
    small = canvas.resize((96, 96), Image.LANCZOS)
    preview.paste(small, (520, 60), small)
    tiny = canvas.resize((32, 32), Image.LANCZOS)
    preview.paste(tiny, (520, 200), tiny)
    preview.save(os.path.join(ICON_DIR, "app-preview.png"))

    print("wrote", png_path)
    print("wrote", ico_path)


if __name__ == "__main__":
    main()
