from PIL import Image
import sys
import numpy as np

# Image parameters
LOGO_WIDTH = 180
LOGO_HEIGHT = 152
BITS_PER_PIXEL = 2
PIXELS_PER_BYTE = 8 // BITS_PER_PIXEL
LOGO_ROW_BYTES = (LOGO_WIDTH * BITS_PER_PIXEL + 7) // 8

# Pixel definitions
PX_BLACK = 0
PX_TRANSPARENT = 1
PX_WHITE = 2
PX_GRAY = 3

COLOR_RGB_MAP = {
    PX_BLACK: (0, 0, 0),
    PX_WHITE: (255, 255, 255),
    PX_GRAY:  (128, 128, 128),
}

def classify_pixel(r, g, b, a):
    if a == 0:
        return PX_TRANSPARENT
    colors = np.array([COLOR_RGB_MAP[px] for px in (PX_BLACK, PX_WHITE, PX_GRAY)])
    pixel = np.array([r, g, b])
    distances = np.sum((colors - pixel) ** 2, axis=1)
    return [PX_BLACK, PX_WHITE, PX_GRAY][np.argmin(distances)]

def image_to_2bpp_array(image_path):
    # Open the image
    img = Image.open(image_path).convert('RGBA')

    # Check image size
    if img.size != (LOGO_WIDTH, LOGO_HEIGHT):
        print(f"Warning: Image size is {img.size}, expected {LOGO_WIDTH}x{LOGO_HEIGHT}.")
        response = input("Resize and continue? (y/n): ").strip().lower()
        if response != 'y':
            print("Aborting.")
            sys.exit(1)
        img = img.resize((LOGO_WIDTH, LOGO_HEIGHT), Image.NEAREST)

    pixels = img.load()

    output_bytes = []

    for y in range(LOGO_HEIGHT):
        byte = 0
        bit_pos = 6
        for x in range(LOGO_WIDTH):
            r, g, b, a = pixels[x, y]
            pixel_value = classify_pixel(r, g, b, a)

            byte |= (pixel_value << bit_pos)
            bit_pos -= 2

            if bit_pos < 0:
                output_bytes.append(byte)
                byte = 0
                bit_pos = 6

        if bit_pos != 6:
            output_bytes.append(byte)

    return output_bytes

def format_as_c_array(byte_array):
    lines = []
    lines.append('#ifndef LOGO_H')
    lines.append('#define LOGO_H')
    lines.append('#include <stdint.h>\n')

    lines.append(f'#define LOGO_WIDTH     ({LOGO_WIDTH})')
    lines.append(f'#define LOGO_HEIGHT    ({LOGO_HEIGHT})')
    lines.append(f'#define LOGO_ROW_BYTES ((LOGO_WIDTH * {BITS_PER_PIXEL} + 7) / 8)\n')

    lines.append('__attribute__((section(".logo")))')
    lines.append(f'const uint8_t logo_data[{len(byte_array)}] = {{')

    line = '    '
    for i, byte in enumerate(byte_array):
        line += f'0x{byte:02X}, '
        if (i + 1) % 12 == 0:
            lines.append(line)
            line = '    '

    if line.strip():
        lines.append(line)

    lines.append('};')
    lines.append('\n#endif //LOGO_H\n')

    return '\n'.join(lines)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_logo.py <input_image> <output_header>")
        sys.exit(1)

    image_path = sys.argv[1]
    output_path = sys.argv[2]

    byte_array = image_to_2bpp_array(image_path)
    c_code = format_as_c_array(byte_array)

    with open(output_path, 'w') as f:
        f.write(c_code)

    print(f"Logo successfully converted and saved to {output_path}")
