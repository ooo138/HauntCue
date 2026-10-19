"""Check Xtensa windowed-ABI stack frames in the linked classic ESP32 ELF.

No compiler/disassembler dependency. ENTRY encodes a 12-bit frame size in
8-byte units. These per-function limits complement runtime boot verification.
"""
from pathlib import Path
import struct
import sys

def check(path):
    data = Path(path).read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected a little-endian ELF32 application.')
    offset = struct.unpack_from('<I', data, 32)[0]
    stride, count = struct.unpack_from('<HH', data, 46)
    sections = [struct.unpack_from('<10I', data, offset + i * stride) for i in range(count)]
    frames = []
    for table in sections:
        if table[1] != 2:
            continue
        strings = sections[table[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(table[4], table[4] + table[5], table[9]):
            name, address, length, info, other, index = struct.unpack_from('<IIIBBH', data, at)
            if info & 15 != 2 or not 0 < index < len(sections):
                continue
            section = sections[index]
            start = section[4] + address - section[3]
            instruction = data[start:start + 3]
            if len(instruction) == 3 and instruction[0] == 0x36:
                function = names[name:].split(b'\0')[0].decode(errors='replace')
                frames.append(((int.from_bytes(instruction, 'little') >> 12) * 8, function))
    if not any(name == '_Z5setupv' for size, name in frames):
        raise ValueError('Could not verify the startup stack frame.')
    for size, name in sorted(frames, reverse=True)[:5]:
        print(f'Stack frame: {size:5d} bytes  {name[:100]}')
    failures = [(size, name) for size, name in frames
                if size >= (2048 if name.startswith(('_Z5setupv', '_Z12decodeConfig')) else 4096)]
    if failures:
        raise ValueError('Unsafe task stack budget: ' + ', '.join(f'{name}={size}' for size, name in failures))
    print('PASS: startup/decoder frames under 2048 bytes; all other checked frames under 4096 bytes.')

if __name__ == '__main__':
    try:
        check(sys.argv[1])
    except Exception as error:
        print('STACK CHECK FAILED: ' + str(error), file=sys.stderr)
        sys.exit(1)
