#!/usr/bin/env python3
"""Convert an image file to an ST7789-friendly RGB565 C array."""

import argparse
from pathlib import Path
from PIL import Image, ImageOps


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert PNG/JPG/etc. to big-endian RGB565 bytes for ST7789."
    )
    parser.add_argument("input", help="source image path")
    parser.add_argument("-o", "--output", required=True, help="output .c or .h path")
    parser.add_argument("--name", default="image_rgb565", help="C array name")
    parser.add_argument("--width", type=int, default=120, help="output width")
    parser.add_argument("--height", type=int, default=120, help="output height")
    parser.add_argument(
        "--fit",
        choices=("cover", "contain", "stretch"),
        default="cover",
        help="resize behavior before RGB565 conversion",
    )
    return parser.parse_args()


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
    image = Image.open(args.input)
    image = resize_image(image, args.width, args.height, args.fit)

    values = []
    for r, g, b in image.getdata():
        rgb565 = rgb888_to_rgb565(r, g, b)
        values.append(rgb565 >> 8)
        values.append(rgb565 & 0xFF)

    output = Path(args.output)
    with output.open("w", encoding="utf-8") as f:
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {args.name.upper()}_WIDTH {args.width}\n")
        f.write(f"#define {args.name.upper()}_HEIGHT {args.height}\n\n")
        f.write(f"const uint8_t {args.name}[] = {{\n")

        for offset in range(0, len(values), 12):
            line = values[offset:offset + 12]
            f.write("    " + ", ".join(f"0x{value:02X}" for value in line))
            f.write(",\n")

        f.write("};\n")


if __name__ == "__main__":
    main()
