"""USB diagnostic parser tests; no board or pyserial required."""
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from video_status import read_status


class FakePort:
    def __init__(self, data):
        self.data = bytearray(data)
        self.written = b""

    def write(self, data):
        self.written += data

    def read(self, size):
        result = bytes(self.data[:1])  # deliberately fragment every field
        del self.data[:1]
        return result


class StatusTest(unittest.TestCase):
    def test_fragmented_reply_with_banner_and_bad_checksum(self):
        payload = struct.pack("<4H", 0xc20d, 100, 20, 19)
        body = bytes([8, 254]) + payload
        checksum = 0
        for value in body:
            checksum ^= value
        frame = b"$M>" + body + bytes([checksum])
        bad = frame[:-1] + bytes([checksum ^ 1])
        port = FakePort(b"OpenPixelOSD banner\r\n" + bad + frame)
        self.assertEqual(read_status(port), (0xc20d, 100, 20, 19))
        self.assertEqual(port.written, b"$M<\x00\xfe\xfe")

    def test_no_reply_times_out(self):
        with patch("video_status.time.monotonic", side_effect=[0, 0, 3]):
            with self.assertRaises(TimeoutError):
                read_status(FakePort(b""))


if __name__ == "__main__":
    unittest.main()
