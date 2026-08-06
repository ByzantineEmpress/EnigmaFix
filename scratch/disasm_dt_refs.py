import struct

# We will print the surrounding bytes of the matching instruction RVAs to see what they do.
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

# Instruction RVAs we found:
ref_rvas = [0xFD193, 0x2695B7, 0x2F1C77, 0x339EEE, 0x656D91, 0x7F54FC, 0x81838F, 0x84FAD8]

# Print 16 bytes before and 32 bytes after each reference RVA
for rva in ref_rvas:
    offset = rva_to_offset(rva)
    if offset is not None:
        print(f"\n--- Surrounding Assembly for RVA 0x{rva:X} ---")
        start_off = offset - 16
        end_off = offset + 32
        chunk = data[start_off:end_off]
        for idx in range(len(chunk)):
            curr_rva = rva - 16 + idx
            # Highlight the matching instruction start
            marker = "<- MATCH" if curr_rva == rva else ""
            print(f"RVA 0x{curr_rva:X} [Offset 0x{start_off+idx:X}]: {chunk[idx]:02X} {marker}")
