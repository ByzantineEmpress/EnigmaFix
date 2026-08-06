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

def offset_to_rva(offset):
    for sec in sections:
        if sec['raw_ptr'] <= offset < sec['raw_ptr'] + sec['raw_size']:
            return offset - sec['raw_ptr'] + sec['vaddr']
    return None

text_sec = [s for s in sections if s['name'] == '.text'][0]
text_start = text_sec['raw_ptr']
text_bytes = data[text_start : text_start + text_sec['raw_size']]

# Battle Sig: F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 48 8D ?? ?? 4C 8D
# Pattern: mulss xmm1, [mem]; mulss xmm2, [mem]; mulss xmm6, [mem]; lea rax, ...
for i in range(len(text_bytes) - 30):
    chunk = text_bytes[i:i+30]
    if (chunk[0:2] == b'\xF3\x0F' and chunk[8:10] == b'\xF3\x0F' and chunk[16:18] == b'\xF3\x0F' and chunk[24:26] == b'\x48\x8D'):
        rva = offset_to_rva(text_start + i)
        print(f"Match found at RVA 0x{rva:X}")
