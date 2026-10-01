"""Small TCP/PNG/hash helpers for the representative native smoke route."""
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = "5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8"
BIOS_SHA1 = "300c20df6731a33952ded8c436f7f186d25d3492"


class Client:
    def __init__(self, port, process):
        deadline = time.monotonic() + 15
        while True:
            if process.poll() is not None:
                raise RuntimeError(f"process exited early: {process.returncode}")
            try:
                self.sock = socket.create_connection(("127.0.0.1", port), 1)
                break
            except OSError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.1)
        self.sock.settimeout(30)
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.stream = self.sock.makefile("rb")

    def call(self, cmd, **args):
        self.sock.sendall(json.dumps(dict(cmd=cmd, **args)).encode() + b"\n")
        response = json.loads(self.stream.readline())
        if not response.get("ok"):
            raise RuntimeError(f"{cmd}: {response}")
        return response

    def close(self):
        self.stream.close()
        self.sock.close()


def png(path, rgb):
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data)))
    scanlines = b"".join(b"\0" + rgb[y * 720:(y + 1) * 720] for y in range(160))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" +
                     chunk(b"IHDR", struct.pack(">2I5B", 240, 160, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b""))


def digest(data):
    return hashlib.sha256(data).hexdigest()
