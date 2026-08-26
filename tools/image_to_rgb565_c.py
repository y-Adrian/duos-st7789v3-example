#!/usr/bin/env python3
"""Convert an image file to an ST7789-friendly RGB565 C array."""

import argparse
import re
from pathlib import Path
from PIL import Image, ImageOps


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert PNG/JPG/etc. to big-endian RGB565 bytes for ST7789."
    )
    parser.add_argument("input", help="source image path")
    parser.add_argument("-o", "--output", default="picture_data.c", help="output .c path")
    parser.add_argument("--header", help="output .h path, default is output path with .h suffix")
    parser.add_argument("--name", default="picture_tab", help="C array name")
    parser.add_argument("--width", type=int, default=320, help="output width")
    parser.add_argument("--height", type=int, default=170, help="output height")
    parser.add_argument(
        "--fit",
        choices=("cover", "contain", "stretch"),
        default="cover",
        help="resize behavior before RGB565 conversion",
    )
    return parser.parse_args()


def macro_name(name):
    macro = re.sub(r"[^0-9A-Za-z]+", "_", name).strip("_").upper()
    if not macro:
        return "IMAGE_RGB565"
    if macro[0].isdigit():
        macro = "_" + macro
    return macro


def header_guard(path):
    return macro_name(path.name) + "_"


def resize_image(image, width, height, fit):
    image = image.convert("RGB")

    if fit == "stretch":
        return image.resize((width, height), Image.Resampling.LANCZOS)

    if fit == "contain":
        contained = ImageOps.contain(image, (width, height), Image.Resampling.LANCZOS)
        canvas = Image.new("RGB", (width, height), (0, 0, 0))
        canvas.paste(contained, ((width - contained.width) // 2,
                                 (height - contained.height) // 2))
        return canvas

    return ImageOps.fit(image, (width, height), Image.Resampling.LANCZOS)


def main():
    args = parse_args()
    image = ImageOps.exif_transpose(Image.open(args.input))
    image = resize_image(image, args.width, args.height, args.fit)

    values = []
    for r, g, b in image.get_flattened_data():
        rgb565 = rgb888_to_rgb565(r, g, b)
        values.append(rgb565 >> 8)
        values.append(rgb565 & 0xFF)

    output = Path(args.output)
    header = Path(args.header) if args.header else output.with_suffix(".h")
    array_macro = macro_name(args.name)
    guard = header_guard(header)

    with header.open("w", encoding="utf-8") as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {array_macro}_WIDTH  {args.width}\n")
        f.write(f"#define {array_macro}_HEIGHT {args.height}\n")
        f.write(f"#define {array_macro}_SIZE   ({array_macro}_WIDTH * {array_macro}_HEIGHT * 2)\n\n")
        f.write(f"extern const uint8_t {args.name}[{array_macro}_SIZE];\n\n")
        f.write("#endif\n")

    with output.open("w", encoding="utf-8") as f:
        f.write(f"#include \"{header.name}\"\n\n")
        f.write(f"const uint8_t {args.name}[{array_macro}_SIZE] = {{\n")

        for offset in range(0, len(values), 12):
            line = values[offset:offset + 12]
            f.write("    " + ", ".join(f"0x{value:02X}" for value in line))
            f.write(",\n")

        f.write("};\n")


if __name__ == "__main__":
    main()
