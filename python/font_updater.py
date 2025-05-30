import serial
import time
import sys

MSP_OSD_CHAR_WRITE = 87
BAUDRATE = 115200

def msp_checksum(data: bytes) -> int:
    csum = 0
    for b in data:
        csum ^= b
    return csum

def build_msp_command(cmd: int, payload: bytes) -> bytes:
    length = len(payload)
    header = b"$M<"
    frame = bytes([length, cmd]) + payload
    csum = msp_checksum(frame)
    return header + frame + bytes([csum])

def send_symbol(ser, index, symbol_data):
    if len(symbol_data) != 64:
        raise ValueError("Symbol data size must be exactly 64 bytes")
    payload = bytes([index]) + symbol_data
    cmd = build_msp_command(MSP_OSD_CHAR_WRITE, payload)
    ser.write(cmd)
    ser.flush()
    print(f"Sent symbol index {index}")
    time.sleep(0.05)

def parse_mcm(file_path):
    with open(file_path, "r") as file:
        lines = file.readlines()

    if len(lines) != 16385:
        raise ValueError(f"Expected 16385 lines (1 header + 16384 data), got {len(lines)}")

    if lines[0].strip() != "MAX7456":
        raise ValueError(f"First line must be 'MAX7456', got '{lines[0].strip()}'")

    data_lines = lines[1:]

    byte_values = []
    for idx, line in enumerate(data_lines):
        byte_str = line.strip()
        if len(byte_str) == 8 and all(bit in "01" for bit in byte_str):
            byte_values.append(int(byte_str, 2))
        else:
            raise ValueError(f"Invalid byte format '{byte_str}' at line {idx+2}")

    if len(byte_values) != 16384:
        raise ValueError(f"Parsed data length mismatch, expected 16384 bytes, got {len(byte_values)}")

    symbols = [byte_values[i*64:(i+1)*64] for i in range(256)]
    return symbols

def main(mcm_file_path, serial_port):
    symbols = parse_mcm(mcm_file_path)

    ser = serial.Serial(serial_port, BAUDRATE, timeout=1)
    time.sleep(2)

    for i, symbol in enumerate(symbols):
        send_symbol(ser, i, bytes(symbol))

    ser.close()
    print("All symbols sent.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: python {sys.argv[0]} font.mcm /dev/ttyUSB0")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
