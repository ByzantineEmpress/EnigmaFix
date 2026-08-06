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

start_rva = 0x624D50
offset = rva_to_offset(start_rva)
if offset is not None:
    code_bytes = data[offset : offset + 100]
    print(f"Bytes from RVA 0x{start_rva:X}:")
    # Let's print each byte in hex
    hex_str = ' '.join(f'{b:02X}' for b in code_bytes)
    print(hex_str)
    
    # Let's disassemble using Zydis (we don't have python-zydis easily, but we can print with a simple instruction boundary marker or use zydis from tool if we want, or just print byte chunks)
    # We can write a quick C++ scratch script to disassemble it using Zydis or we can use our eyes!
    # Let's just output the hex bytes first.
else:
    print("Could not resolve RVA")
