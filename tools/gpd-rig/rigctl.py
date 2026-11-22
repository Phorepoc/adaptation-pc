#!/usr/bin/env python3
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
"""rigctl.py -- the lab side of the GPD Win 2 rig's debug link (the kernel's xk_netlog.cpp; wire format in
adaptation-x32 os/tools/phorepoc-x86/ekern/xk_netlog_proto.h). The reference for PhoneLab's gpd driver.

  rigctl.py log [--out FILE]              print the kernel log (UDP broadcasts to :6666), boot changes and losses marked
  rigctl.py ping|reboot|dump|hang [--to IP]
  rigctl.py key STDKEY [--up]             a raw key event (a TStdScanCode value from e32keys.h)
  rigctl.py touch X Y [move|down|up]
  rigctl.py shot FILE.png                 the OS screen

Commands go to --to (default: broadcast 255.255.255.255) port 6667 with --token (default 0); the reply comes back to
this socket. Windows: allow UDP 6666 in (Private profile) for the log."""
import argparse, socket, struct, sys, time, zlib

LOG_PORT, CMD_PORT = 6666, 6667


def log(args):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("", LOG_PORT))
    out = open(args.out, "ab") if args.out else None
    boot, seq = None, None
    while True:
        d, a = s.recvfrom(2048)
        if len(d) < 12 or d[:4] != b"PHL1":
            continue
        b, q = struct.unpack_from("<II", d, 4)
        mark = b""
        if b != boot:
            mark = b"\n===== rig: boot %08x from %s =====\n" % (b, a[0].encode())
            boot, seq = b, q - 1
        if q != seq + 1:
            mark += b"\n===== rig: %d packets lost =====\n" % (q - seq - 1)
        seq = q
        for o in (sys.stdout.buffer, out):
            if o:
                o.write(mark + d[12:]); o.flush()


def command(args, op, data=b"", stream=False):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
    s.bind(("", 0))
    s.settimeout(2.0)
    cid = int(time.time() * 1000) & 0xFFFFFFFF
    s.sendto(b"PHC1" + struct.pack("<IIB", args.token, cid, ord(op)) + data, (args.to, CMD_PORT))
    while True:
        d, a = s.recvfrom(2048)
        if d[:4] == b"PHR1" and struct.unpack_from("<I", d, 4)[0] == cid:
            st = struct.unpack_from("<i", d, 9)[0]
            if not stream:
                return st, d[13:], a
            break
    if st != 0:
        return st, b"", a
    w, h = struct.unpack_from("<HH", d, 13)
    px = bytearray(w * h * 4)
    got = 0
    while True:
        d, _ = s.recvfrom(2048)
        if d[:4] != b"PHF1" or struct.unpack_from("<I", d, 4)[0] != cid:
            continue
        first, n = struct.unpack_from("<IH", d, 12)
        if n == 0:
            break
        px[first * 4:(first + n) * 4] = d[18:18 + n * 4]
        got += n
    return 0, (w, h, px, got), a


def png(path, w, h, px):
    raw = b"".join(b"\0" + bytes(px[y * w * 4 + x * 4 + k] for x in range(w) for k in (2, 1, 0)) for y in range(h))
    def chunk(t, b):
        return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                           chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd")
    ap.add_argument("rest", nargs="*")
    ap.add_argument("--to", default="255.255.255.255")
    ap.add_argument("--token", type=lambda v: int(v, 0), default=0)
    ap.add_argument("--out")
    ap.add_argument("--up", action="store_true")
    args = ap.parse_args()
    if args.cmd == "log":
        return log(args)
    ops = {"ping": "P", "reboot": "R", "dump": "D", "hang": "H"}
    if args.cmd in ops:
        st, d, a = command(args, ops[args.cmd])
        extra = ""
        if args.cmd == "ping" and len(d) >= 10:
            extra = " boot %08x mac %s" % (struct.unpack_from("<I", d)[0], ":".join("%02x" % b for b in d[4:10]))
        print("%s: %d from %s%s" % (args.cmd, st, a[0], extra))
    elif args.cmd == "key":
        st, _, _ = command(args, "K", struct.pack("<HB", int(args.rest[0], 0), 0 if args.up else 1))
        print("key:", st)
    elif args.cmd == "touch":
        act = {"move": 0, "down": 1, "up": 2}[args.rest[2] if len(args.rest) > 2 else "move"]
        st, _, _ = command(args, "T", struct.pack("<HHB", int(args.rest[0]), int(args.rest[1]), act))
        print("touch:", st)
    elif args.cmd == "shot":
        st, r, _ = command(args, "S", stream=True)
        if st:
            sys.exit("shot: %d" % st)
        w, h, px, got = r
        png(args.rest[0], w, h, px)
        print("shot: %dx%d, %d of %d pixels -> %s" % (w, h, got, w * h, args.rest[0]))
    else:
        ap.error("unknown command " + args.cmd)


if __name__ == "__main__":
    main()
