import re
import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Sections
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
    sections.append({
        'name': name,
        'vsize': vsize,
        'vaddr': vaddr,
        'raw_size': raw_size,
        'raw_ptr': raw_ptr
    })

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

text_sec = [s for s in sections if s['name'] == '.text'][0]
text_start = text_sec['raw_ptr']
text_size = text_sec['raw_size']
text_bytes = data[text_start : text_start + text_size]

def sig_to_regex(sig_str):
    parts = sig_str.split()
    re_parts = []
    for p in parts:
        if p == '??' or p == '?':
            re_parts.append(b'.')
        else:
            re_parts.append(re.escape(bytes([int(p, 16)])))
    return b''.join(re_parts)

# Signatures from Cheat Engine Table:
signatures = {
    "Animation Interpolation Speed": "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? EB ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? EB ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 48 83 C4",
    "2D Effect Speed": "F3 0F ?? ?? ?? ?? ?? ?? 0F 57 ?? F3 0F ?? ?? ?? 0F 2F ?? 0F 83",
    "Live2D Animation Speed": "F3 44 ?? ?? ?? ?? ?? ?? ?? 45 0F ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 45 0F ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 44 0F ?? ?? ?? ?? F3 44",
    "Battle Movement Speed": "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 48 8D ?? ?? 4C 8D"
}

for name, sig in signatures.items():
    regex = sig_to_regex(sig)
    matches = list(re.finditer(regex, text_bytes))
    print(f"\n--- Signature Scan: {name} ---")
    if not matches:
        print("No matches found!")
    for m in matches:
        offset = text_start + m.start()
        rva = offset_to_rva(offset)
        print(f"Match found at RVA 0x{rva:X} (Offset 0x{offset:X})")
        # Print first 16 bytes of the match
        match_bytes = text_bytes[m.start() : m.start() + 16]
        print(f"Bytes: {' '.join(f'{b:02X}' for b in match_bytes)}")
