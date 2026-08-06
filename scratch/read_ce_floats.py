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

def read_float_rva(rva):
    offset = rva_to_offset(rva)
    if offset is None:
        return None
    val_bytes = data[offset : offset + 4]
    return struct.unpack("<f", val_bytes)[0]

target_rvas = [0xE3250C, 0xE32580, 0xE325E4, 0xE3273C, 0xE32910, 0xE32A30, 0xE32BB0, 0xE32C3C, 0xE32C68, 0xE32C70, 0xE32CD8]
for rva in target_rvas:
    val = read_float_rva(rva)
    print(f"RVA 0x{rva:X} -> Float: {val}")
