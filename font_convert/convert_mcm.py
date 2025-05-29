def convert_mcm_to_c_array(file_path, output_file):
    try:
        with open(file_path, "r") as file:
            lines = file.readlines()

        if len(lines) != 16385:
            print(f"Error: File contains {len(lines)} lines, but 16385 are expected (1 metadata + 16384 data).")
            return

        metadata_line = lines[0].strip()
        if metadata_line != "MAX7456":
            print(f"Error: First line does not match 'MAX7456'. Found: '{metadata_line}'")
            return

        data_lines = lines[1:]

        byte_values = []
        for idx, line in enumerate(data_lines):
            byte_str = line.strip()
            if len(byte_str) == 8 and all(bit in '01' for bit in byte_str):
                byte_value = int(byte_str, 2)
                byte_values.append(byte_value)
            else:
                print(f"Error: Invalid byte format '{byte_str}' on line {idx+2}")
                return

        header = """#ifndef FONT_DATA_H
#define FONT_DATA_H
#include <stdint.h>

#define FONT_WIDTH      (12)
#define FONT_HEIGHT     (18)

#define FONT_BPP        (2) // 2bits per pixel
#define FONT_CHARS      (256)
#define BYTES_PER_ROW   ((FONT_WIDTH * FONT_BPP) / 8)     // = 3
#define BYTES_PER_CHAR  (BYTES_PER_ROW * FONT_HEIGHT)     // = 54
#define FONT_STRIDE     (sizeof(font_data) / FONT_CHARS)  // = 64

__attribute__((section(".font")))
"""

        footer = "\n#endif // FONT_DATA_H\n"

        c_array = header
        c_array += "const uint8_t font_data[16384] = {\n"

        BYTES_PER_CHAR = 64
        TOTAL_CHARS = 256

        for ch in range(TOTAL_CHARS):
            addr = ch * BYTES_PER_CHAR
            ch_repr = chr(ch) if 32 <= ch <= 126 else ''
            c_array += f"\n/* Hex 0x{ch:02X}, Char '{ch_repr}' */\n"

            for i in range(BYTES_PER_CHAR):
                if i % 16 == 0:
                    c_array += "    "
                c_array += f"0x{byte_values[addr + i]:02X}, "
                if (i + 1) % 16 == 0:
                    c_array += "\n"

        c_array = c_array.rstrip(", \n") + "\n};\n"
        c_array += footer

        with open(output_file, "w") as output:
            output.write(c_array)

        print(f"File successfully converted and saved to {output_file}")

    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print(f"Usage: python {sys.argv[0]} font.mcm font.h")
    else:
        convert_mcm_to_c_array(sys.argv[1], sys.argv[2])