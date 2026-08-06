import struct

dll_path = r"Binaries\Windows\EnigmaFix.asi"
with open(dll_path, "rb") as f:
    data = f.read()

pe_offset = struct.unpack("<I", data[0x3C:0x40])[0]
num_sections = struct.unpack("<H", data[pe_offset + 6 : pe_offset + 8])[0]
size_of_optional_header = struct.unpack("<H", data[pe_offset + 20 : pe_offset + 22])[0]
optional_header_offset = pe_offset + 24
section_table_offset = optional_header_offset + size_of_optional_header

# Read sections
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

# Get Import Directory RVA (Data Directory 1 in Optional Header)
# For PE32+, Optional Header has magic 0x20B, size is 112+128=240. Data Directories start at offset 112.
magic = struct.unpack("<H", data[optional_header_offset : optional_header_offset + 2])[0]
if magic == 0x20B:  # PE32+ (64-bit)
    import_dir_rva = struct.unpack("<I", data[optional_header_offset + 120 : optional_header_offset + 124])[0]
    import_dir_size = struct.unpack("<I", data[optional_header_offset + 124 : optional_header_offset + 128])[0]
elif magic == 0x10B:  # PE32 (32-bit)
    import_dir_rva = struct.unpack("<I", data[optional_header_offset + 96 : optional_header_offset + 100])[0]
    import_dir_size = struct.unpack("<I", data[optional_header_offset + 100 : optional_header_offset + 104])[0]
else:
    print("Unknown PE magic:", hex(magic))
    exit(1)

print(f"Import Directory RVA: 0x{import_dir_rva:X}, Size: {import_dir_size}")

if import_dir_rva == 0:
    print("No imports found.")
    exit(0)

import_offset = rva_to_offset(import_dir_rva)
if import_offset is None:
    print("Could not map import directory RVA to file offset.")
    exit(1)

# Read Import Descriptor table (20 bytes per entry, ends with all zero entry)
imported_dlls = []
idx = 0
while True:
    entry_offset = import_offset + idx * 20
    entry_data = data[entry_offset : entry_offset + 20]
    if len(entry_data) < 20 or all(b == 0 for b in entry_data):
        break
    # OriginalFirstThunk, TimeDateStamp, ForwarderChain, NameRVA, FirstThunk
    name_rva = struct.unpack("<I", entry_data[12:16])[0]
    name_offset = rva_to_offset(name_rva)
    if name_offset is not None:
        # read null-terminated string
        name_bytes = []
        off = name_offset
        while data[off] != 0:
            name_bytes.append(data[off])
            off += 1
        dll_name = bytes(name_bytes).decode('utf-8', errors='ignore')
        imported_dlls.append(dll_name)
    idx += 1

print("Imported DLLs:")
for d in imported_dlls:
    print("  " + d)
