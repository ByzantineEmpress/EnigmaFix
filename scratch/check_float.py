import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

def rva_to_offset(rva):
    pe_offset = struct.unpack("<I", data[0x3C:0x40])[0]
    num_sections = struct.unpack("<H", data[pe_offset + 6 : pe_offset + 8])[0]
    size_of_optional_header = struct.unpack("<H", data[pe_offset + 20 : pe_offset + 22])[0]
    section_table_offset = pe_offset + 24 + size_of_optional_header
    for i in range(num_sections):
        sec_offset = section_table_offset + i * 40
        sec_data = data[sec_offset : sec_offset + 40]
        vsize, vaddr, raw_size, raw_ptr = struct.unpack("<IIII", sec_data[8:24])
        if vaddr <= rva < vaddr + vsize:
            return rva - vaddr + raw_ptr
    return None

off = rva_to_offset(0x1045E48)
print("Offset:", off)
if off and off < len(data):
    val = struct.unpack("<f", data[off:off+4])[0]
    print("Float at 0x1045E48:", val)
