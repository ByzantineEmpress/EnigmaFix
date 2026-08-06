import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Let's map target offsets
# In dump_limit_target.py, at File Offset 0xE7D33, the instruction was:
# F3 0F 10 05 00 A3 D4 00 (movss xmm0, [rip + 0xD4A300])
# rip at end of instruction is 0xE7D33 + 8 = 0xE7D3B.
# RVA of rip is 0xE7D3B - 0x400 + 0x1000 = 0xE893B.
# So target RVA is 0xE893B + 0xD4A300 = 0xE32C3B!
# Let's map RVA 0xE32C3B back to file offset:
# RVA 0xE32C3B is in .rdata. RawOffset = RVA - 0xBD8000 + 0xBD6A00 = 0xE3163B!

def read_float_rva(rva):
    # Map RVA to raw offset
    # Section .rdata: RVA=0xBD8000, RawOffset=0xBD6A00
    raw_offset = rva - 0xBD8000 + 0xBD6A00
    val_bytes = data[raw_offset : raw_offset + 4]
    return struct.unpack("<f", val_bytes)[0]

# Let's check the 3 floats starting at 0xE32C3B (which is 0xE32C30 to 0xE32C48)
base_rva = 0xE32C30
for i in range(10):
    rva = base_rva + i * 4
    val = read_float_rva(rva)
    print(f"RVA 0x{rva:X} -> Float value: {val}")
