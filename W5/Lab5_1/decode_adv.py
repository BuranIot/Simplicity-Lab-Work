#!/usr/bin/env python3
"""Decode Lab5_1 manufacturer data (hex from Simplicity Connect) into decimal.

Usage:
  python3 decode_adv.py 0x01000C             # just the 3 data bytes
  python3 decode_adv.py "FF 02 01 00 0C"     # with company ID 0x02FF (sent as FF 02)
  python3 decode_adv.py                      # no argument: paste lines, Ctrl+D to quit
"""
import re
import sys


def decode(text):
    h = re.sub(r"0x|[^0-9a-fA-F]", "", text).upper()
    if len(h) % 2:
        raise ValueError("odd number of hex digits")
    b = bytes.fromhex(h)
    i = b.find(b"\xFF\x02")  # company ID 0x02FF, little-endian on air
    if i >= 0:
        b = b[i + 2:]
    if len(b) < 3:
        raise ValueError("need 3 data bytes: LED0 LED1 count")
    return {"LED0": b[0], "LED1": b[1], "count": int(f"{b[2]:02X}")}  # count is BCD


def main():
    lines = sys.argv[1:] or sys.stdin
    for line in lines:
        if not line.strip():
            continue
        try:
            d = decode(line)
            print(f"LED0={d['LED0']} LED1={d['LED1']} count={d['count']}")
        except ValueError as e:
            print(f"cannot decode {line.strip()!r}: {e}")


# self-check
assert decode("0x010012") == {"LED0": 1, "LED1": 0, "count": 12}
assert decode("FF 02 00 01 99") == {"LED0": 0, "LED1": 1, "count": 99}
assert decode("02 01 06 06 FF FF 02 01 00 01 05 09 34 39 35 31")["count"] == 1

if __name__ == "__main__":
    main()
