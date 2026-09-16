"""Read OSD hardware counters over USB CDC without modifying configuration.

Usage: python tools/video_status.py COM3
Requires pyserial (also used by the repository's font updater).
"""
import argparse
import struct
import time


def read_status(port):
    port.write(b"$M<\x00\xfe\xfe")  # MSP_DEBUG, empty request
    data = bytearray()
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        data.extend(port.read(64))
        while True:
            start = data.find(b"$M>")
            if start < 0:
                data[:] = data[-2:]
                break
            del data[:start]
            if len(data) < 5:
                break
            size, command = data[3:5]
            if len(data) < size + 6:
                break
            frame = bytes(data[:size + 6])
            del data[:size + 6]
            checksum = 0
            for value in frame[3:]:
                checksum ^= value
            if command == 254 and size == 8 and checksum == 0:
                return struct.unpack("<4H", frame[5:13])
    raise TimeoutError("No OSD debug reply. Check the port and flash the SyncScan firmware.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    parser.add_argument("--samples", type=int, default=12)
    args = parser.parse_args()
    import serial
    with serial.Serial(args.port, 115200, timeout=0.1) as port:
        for _ in range(args.samples):
            flags, edges, armed, completed = read_status(port)
            print(f"camera={1 + ((flags >> 14) & 1)} threshold={flags & 0x1fff}mV "
                  f"locked={bool(flags & 0x8000)} dma_error={bool(flags & 0x2000)} "
                  f"edges={edges} armed={armed} completed={completed}", flush=True)
            time.sleep(1)
    print("Counters wrap at 65536. Edges must increase; armed/completed must increase once locked.")


if __name__ == "__main__":
    main()
