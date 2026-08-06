import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Target float value: 1/60 (0.0166666675)
target_val = 1.0 / 60.0

# Parse sections
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

# Target RVAs of 1/60 float constant we found:
target_rvas = [0x2F1481, 0x2FC50A, 0x405FA3, 0x598861, 0x7A5275, 0x7A5336, 0xE3250C]

# Scan the .text section (code) for RIP-relative references to these RVAs.
# In x86_64, an instruction referencing a target RVA will look like:
# [opcode...] [32-bit signed offset]
# The target address = rip_of_next_instruction + signed_offset.
# So: target_rva = (inst_rva + inst_len) + signed_offset
# signed_offset = target_rva - inst_rva - inst_len.
# Let's search the .text section bytes for any 4-byte values that match the calculated signed_offset
# for each instruction position!

text_sec = [s for s in sections if s['name'] == '.text'][0]
text_start = text_sec['raw_ptr']
text_size = text_sec['raw_size']
text_vaddr = text_sec['vaddr']

print(f"Scanning .text section (RVA range 0x{text_vaddr:X} - 0x{text_vaddr+text_size:X})...")

references = []
# We scan byte-by-byte in .text.
# An instruction operand of size 4 bytes can start at any offset.
# To be robust, let's check every 4-byte offset.
# For each offset, we read a 32-bit signed integer `disp`.
# The instruction size for RIP-relative accesses is typically between 5 and 9 bytes.
# For movss xmm, [rip+disp], the instruction size is typically 8 bytes (e.g. F3 0F 10 05 [disp32] is 8 bytes).
# Let's try common instruction lengths (e.g., 6, 7, 8, 9) and check if the RVA matches!
for offset in range(text_start, text_start + text_size - 4):
    disp = struct.unpack("<i", data[offset:offset+4])[0]
    # Check if this matches any target RVA for common instruction lengths
    curr_rva = offset_to_rva(offset)
    for inst_len in [5, 6, 7, 8, 9]:
        # The offset is the displacement offset. It is located at `inst_end - 4`.
        # So inst_end = offset + 4.
        inst_rva = curr_rva - (inst_len - 4)
        target = inst_rva + inst_len + disp
        if target in target_rvas:
            # Let's print the match
            raw_inst_start = rva_to_offset(inst_rva)
            if raw_inst_start is not None:
                inst_bytes = data[raw_inst_start : raw_inst_start + inst_len]
                print(f"Match found: Instruction RVA 0x{inst_rva:X} (len={inst_len}), Opcode: {' '.join(f'{b:02X}' for b in inst_bytes)} -> References RVA 0x{target:X}")
