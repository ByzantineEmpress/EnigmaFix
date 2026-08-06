import re
import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

pe_offset = struct.unpack("<I", data[0x3C:0x40])[0]
num_sections = struct.unpack("<H", data[pe_offset + 6 : pe_offset + 8])[0]
size_of_optional_header = struct.unpack("<H", data[pe_offset + 20 : pe_offset + 22])[0]
section_table_offset = pe_offset + 24 + size_of_optional_header

sections = []
for i in range(num_sections):
    sec_offset = section_table_offset + i * 40
    sec_data = data[sec_offset : sec_offset + 40]
    name = sec_data[0:8].decode('utf-8', errors='ignore').strip('\x00')
    vsize, vaddr, raw_size, raw_ptr = struct.unpack("<IIII", sec_data[8:24])
    sections.append({'name': name, 'vsize': vsize, 'vaddr': vaddr, 'raw_size': raw_size, 'raw_ptr': raw_ptr})

text_sec = [s for s in sections if s['name'] == '.text'][0]
text_start = text_sec['raw_ptr']
text_size  = text_sec['raw_size']
text_bytes = data[text_start : text_start + text_size]

def rva_to_offset(rva):
    for sec in sections:
        if sec['vaddr'] <= rva < sec['vaddr'] + sec['vsize']:
            return rva - sec['vaddr'] + sec['raw_ptr']
    return None

def offset_to_rva(offset):
    for sec in sections:
        if sec['raw_ptr'] <= offset < sec['raw_ptr'] + sec['raw_size']:
            return offset - sec['raw_ptr'] + sec['vaddr']
    return None

# Pattern from Plugin_DERQ.cpp
sig = bytes.fromhex("8B80" + "00"*4 + "8944" + "00"*2 + "837C2444" + "00" + "74" + "00" + "837C2444")
pattern = b"\x8b\x80"
matches = []
for i in range(len(text_bytes) - 40):
    chunk = text_bytes[i:i+22]
    if (chunk[0:2] == b'\x8b\x80' and chunk[6:8] == b'\x89\x44' and
        chunk[10:14] == b'\x83\x7c\x24\x44' and chunk[15] == 0x74 and
        chunk[17:21] == b'\x83\x7c\x24\x44'):
        rva = offset_to_rva(text_start + i)
        matches.append((rva, i))
        print(f"Found pattern at RVA 0x{rva:X}")
        # Print 80 bytes from the match start
        code = text_bytes[i : i + 80]
        print(' '.join(f'{b:02X}' for b in code))
        print()

if not matches:
    print("No matches found!")
