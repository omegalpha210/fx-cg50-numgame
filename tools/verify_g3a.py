#!/usr/bin/env python3
"""Validate the G3A container against fxSDK fxgxa/g3a.h and util.c.
Adapted from DIFF EQ commit 1a16b1b728c7c9e8a0e2491d96fde4b95cdacdbb.
See docs/ASSET_PROVENANCE.md for license status.
This checks packaging, not execution on calculator hardware.
"""
import hashlib
import struct
import sys
from pathlib import Path


def verify(path):
    raw = Path(path).read_bytes()
    diagnostic = Path(path).name == 'NUMGDIAG.g3a'
    expected_internal = b'@NGDIAG' if diagnostic else b'@NUMGAME'
    expected_name = b'NUM DIAG' if diagnostic else b'NUM GAME'
    if len(raw) < 0x7114:
        raise ValueError('G3A is too short')
    header = bytes(b ^ 255 for b in raw[:32])
    be32 = lambda data, offset: struct.unpack_from('>I', data, offset)[0]
    checks = {
        'magic': header[:8] == b'USBPower',
        'type': header[8] == 0x2c,
        'internal identity': raw[0x60:0x6b].split(b'\0')[0] == expected_internal,
        'NUM GAME identity': raw[0x40:0x50].split(b'\0')[0] == expected_name,
        'signature': header[9:14] == bytes([0, 1, 0, 1, 0]),
        'size1': be32(header, 0x10) == len(raw),
        'size2': be32(raw, 0x2e) == len(raw) - 0x7004,
        'size3': be32(raw, 0x5c) == len(raw),
        'control1': header[0x0e] == (len(raw) + 0x41) & 255,
        'control2': header[0x14] == (len(raw) + 0xb8) & 255,
        'word checksum': struct.unpack_from('>H', header, 0x16)[0]
            == sum(struct.unpack_from('>8H', raw, 0x7100)) & 65535,
        'checksum': be32(raw, 0x20)
            == (sum(raw[:32]) + sum(raw[0x24:-4])) & 0xffffffff,
        'footer': raw[0x20:0x24] == raw[-4:],
        'unselected icon': len(set(raw[0x1000:0x1000 + 92*64*2])) > 1,
        'selected icon': len(set(raw[0x4000:0x4000 + 92*64*2])) > 1,
    }
    if not diagnostic:
        checks['release size <= 1,200,000 decimal bytes'] = len(raw) <= 1_200_000
    failed = [name for name, ok in checks.items() if not ok]
    if failed:
        raise ValueError('Invalid G3A: ' + ', '.join(failed))
    name = raw[0x40:0x50].split(b'\0')[0].decode('ascii')
    print(f'{path}: VALID G3A, {len(raw)} bytes, name={name!r}')
    print(f'SHA256 {hashlib.sha256(raw).hexdigest()}')
    print(f'{len(checks)} container checks passed. HARDWARE TEST REQUIRED.')
    if not diagnostic:
        print(f'1,000,000-byte target: {"PASS" if len(raw) <= 1_000_000 else "EXCEEDED (hard cap passed)"}')


if __name__ == '__main__':
    for file in sys.argv[1:]:
        verify(file)
