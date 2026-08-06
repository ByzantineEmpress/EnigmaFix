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

target_rvas = [0xE3250C]

text_sec = [s for s in sections if s['name'] == '.text'][0]
text_start = text_sec['raw_ptr']
text_size = text_sec['raw_size']
text_vaddr = text_sec['vaddr']

print(f"Scanning .text section (RVA range 0x{text_vaddr:X} - 0x{text_vaddr+text_size:X}) for writes/reads...")

for offset in range(text_start, text_start + text_size - 4):
    disp = struct.unpack("<i", data[offset:offset+4])[0]
    curr_rva = offset_to_rva(offset)
    for inst_len in [5, 6, 7, 8, 9]:
        inst_rva = curr_rva - (inst_len - 4)
        target = inst_rva + inst_len + disp
        if target in target_rvas:
            raw_inst_start = rva_to_offset(inst_rva)
            if raw_inst_start is not None:
                inst_bytes = data[raw_inst_start : raw_inst_start + inst_len]
                # Print instruction bytes to see if it's a write (like F3 0F 11 or similar)
                opcode_str = ' '.join(f'{b:02X}' for b in inst_bytes)
                print(f"Instruction RVA 0x{inst_rva:X} (len={inst_len}): {opcode_str} -> References RVA 0x{target:X}")
