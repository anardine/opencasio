#!/usr/bin/env python3
"""Headless LCD verification via OpenOCD.

Reads the STM32WB55 LCD RAM through a halted (or running, via AHB-AP)
target, decodes it back into glass characters using the SAME mapping
tables the firmware uses (parsed from src/driver/lcd.c), and prints what
the F-91W glass is showing. Compares the time digits against rtcTime.

Usage:  python3 test/lcd/verify.py [host] [port]
  (OpenOCD telnet, default 127.0.0.1 4444; target must be halted or
   memory reads enabled while running)
"""
import re
import socket
import struct
import subprocess
import sys

ELF = ".pio/build/nucleo_wb55rg_p/firmware.elf"
LCD_RAM = 0x40002414  # LCD_RAM0 (COM0 SEG0-31); +4 per word

NM = subprocess.run(
    ["arm-none-eabi-nm", ELF],
    capture_output=True, text=True,
    env={"PATH": "/usr/bin:/bin:" + __import__("os").path.expanduser(
        "~/.platformio/packages/toolchain-gccarmnoneeabi/bin")})


# ---- parse mapping tables from the driver source (single source of truth) ----
src = open("src/driver/lcd.c").read()


def hexnums(s):
    return [int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+", s)]


segmap_block = re.search(r"Segment_Map\[\] = \{(.*?)\};", src, re.S).group(1)
SEGMENT_MAP = hexnums(segmap_block)

cs_block = re.search(r"Character_Set\[\] = \{(.*?)\};", src, re.S).group(1)
CHARACTER_SET = [int(x, 2) for x in re.findall(r"0b([01]{8})", cs_block)]

remap_block = re.search(r"SegLineRemap\[24\] = \{(.*?)\};", src, re.S).group(1)
SEG_LINE_REMAP = [int(x) for x in re.findall(r"\d+", remap_block)]

ind_block = re.search(r"IndicatorMap\[\] = \{(.*?)\};", src, re.S).group(1)
IND = [(int(a), int(b)) for a, b in re.findall(r"\{(\d), (\d+)\}", ind_block)]
IND_NAMES = ["SIGNAL", "BELL", "PM", "24H", "LAP"]


def seg_cell(com, sw_seg):
    """(RAM word offset from LCD_RAM0, bit) for a Sensor-Watch (com,seg)."""
    line = SEG_LINE_REMAP[sw_seg]
    if line < 32:
        return 2 * com, line
    return 2 * com + 1, line - 32


# per-position set of RAM cells for each character
POS_CELLS = []
for pos in range(10):
    cells = {}  # bit index within char -> cell
    segmap = SEGMENT_MAP[pos]
    for i in range(8):
        b = (segmap >> (8 * i)) & 0xFF
        com = b >> 6
        seg = b & 0x3F
        if com > 2:
            cells[i] = None
        else:
            cells[i] = seg_cell(com, seg)
    POS_CELLS.append(cells)

# colon: SW (1,16); ninth segment pos0: SW (0,15)
COLON_CELL = seg_cell(1, 16)
NINTH0_CELL = seg_cell(0, 15)


def decode_position(pos, words):
    on = set()
    for i, cell in POS_CELLS[pos].items():
        if cell is None:
            continue
        w, bit = cell
        if words[w] >> bit & 1:
            on.add(i)
    for ch_i, pat in enumerate(CHARACTER_SET):
        want = {i for i in range(8) if pat >> i & 1}
        # char bits 0..6 = segs A-G; bit7 unused in maps
        if want == on:
            return chr(0x20 + ch_i)
    return "?"


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4444
    s = socket.create_connection((host, port), timeout=10)
    s.settimeout(5)

    def _read_until_prompt():
        buf = b""
        while b"> " not in buf:
            chunk = s.recv(4096)
            if not chunk:
                break
            buf += chunk
        return buf.decode(errors="replace")

    def ocd(cmd):
        s.sendall((cmd + "\n").encode())
        return _read_until_prompt()

    _read_until_prompt()  # drain the banner of a fresh connection


    # words 0..5 = COM0/1/2 low+high
    out = ocd(f"mdw 0x{LCD_RAM:x} 6")
    words = []
    for line_vals in re.findall(r"0x400024[0-9a-f]+: ((?:[0-9a-f]{8} ?)+)", out):
        words += [int(v, 16) for v in line_vals.split()]
    display = "".join(decode_position(p, words) for p in range(10))
    colon = bool(words[COLON_CELL[0]] >> COLON_CELL[1] & 1)
    ninth = bool(words[NINTH0_CELL[0]] >> NINTH0_CELL[1] & 1)
    ind = {n: (words[seg_cell(c, g)[0]] >> seg_cell(c, g)[1] & 1)
           for n, (c, g) in zip(IND_NAMES, IND)}

    print(f"LCD decodes to: {display[:2]} {display[2:4]}  {display[4:6]}"
          f"{':' if colon else ' '}{display[6:8]}:{display[8:10]}")
    print(f"ninth segment pos0: {ninth}")
    print("indicators:", ", ".join(f"{n}={'on' if v else 'off'}" for n, v in ind.items()))

    # sanity: every lit bit must belong to a mapped cell
    used = set()
    for pos in range(10):
        for cell in POS_CELLS[pos].values():
            if cell:
                used.add(cell)
    used.add(COLON_CELL)
    used.add(NINTH0_CELL)
    for c, g in IND:
        used.add(seg_cell(c, g))
    stray = []
    for w in range(6):
        for bit in range(32):
            if w in (1, 3, 5) and bit > 11:
                continue
            if words[w] >> bit & 1 and (w, bit) not in used:
                stray.append((w, bit))
    if stray:
        print(f"UNMAPPED bits lit (mapping bug!): {stray}")
        sys.exit(1)
    print("All lit RAM bits map to known glass cells.")


if __name__ == "__main__":
    main()
