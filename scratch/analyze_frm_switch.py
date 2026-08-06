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

# Hook is at framerateCapFunc + 6 = 0xE88FA + 6 = 0xE8900
# Let's dump 200 bytes from 0xE88FA to understand the full function context
start_rva = 0xE88FA
start_off = rva_to_offset(start_rva)
code = data[start_off : start_off + 200]

print("Full disassembly region (hex bytes):")
for i in range(0, len(code), 16):
    chunk = code[i:i+16]
    hex_part = ' '.join(f'{b:02X}' for b in chunk)
    print(f"RVA 0x{start_rva + i:X}: {hex_part}")

# The branches are at offsets from start:
# case 0 je +0x29  from RVA 0xE8900+2 = 0xE8902, target = 0xE8902 + 2 + 0x29 = 0xE892D
# case 1 je +0x17  from RVA 0xE8907+2 = 0xE8909, target = 0xE8909 + 2 + 0x17 = 0xE8922  
# case 2 je +0x1B  from RVA 0xE890E+2 = 0xE8910, target = 0xE8910 + 2 + 0x1B = 0xE892D  
# Let's recalculate properly from the bytes

# Pattern starts at 0xE88FA
# +0: 8B 80 A0 00 00 00  (6 bytes) mov eax,[rax+A0]
# +6: 89 44 24 44        (4 bytes) mov [rsp+44],eax  <- HOOK POINT
# +A: 83 7C 24 44 00     (5 bytes) cmp [rsp+44],0
# +F: 74 29              (2 bytes) je case0 target
# +11: 83 7C 24 44 01    (5 bytes) cmp [rsp+44],1
# +16: 74 17             (2 bytes) je case1 target
# +18: 83 7C 24 44 02    (5 bytes)
# +1D: 74 1B             (2 bytes)
# +1F: 83 7C 24 44 03    (5 bytes)
# +24: 74 24             (2 bytes)
# +26: 83 7C 24 44 04    (5 bytes)
# +2B: 74 2D             (2 bytes)
# +2D: EB 39             (2 bytes) fallthrough jmp
# +2F: 0F 57 C0          (3 bytes) case0: xorps xmm0,xmm0
# +32: F3 0F 11 44 24 40 (6 bytes) movss [rsp+40],xmm0  (stores 0.0f)
# +38: EB 2E             (2 bytes)
# +3A: F3 0F 10 05 ...   case1 body: movss xmm0, [rip+...]
# +42: F3 0F 11 44 24 40 movss [rsp+40],xmm0 (stores sleep budget float)
# +48: EB 1E             jmp end

# case0 target: 0xE88FA + 0x2F = 0xE8929
# case1 target: 0xE88FA + 0x3A = 0xE8934  
# But from je: at offset 0x0F, je +0x29 from next instr = 0xE88FA+0x11+0x29 = 0xE893A? 
# Let me just print the targets explicitly

base = start_rva
# case 0 je: at offset 0x0F, target = (base+0x11) + 0x29 
case0_je_src = base + 0x11
case0_target = case0_je_src + 0x29
print(f"\ncase 0 je target: RVA 0x{case0_target:X}")

case1_je_src = base + 0x18
case1_target = case1_je_src + 0x17
print(f"case 1 je target: RVA 0x{case1_target:X}")

case2_je_src = base + 0x1F
case2_target = case2_je_src + 0x1B
print(f"case 2 je target: RVA 0x{case2_target:X}")

case3_je_src = base + 0x26
case3_target = case3_je_src + 0x24
print(f"case 3 je target: RVA 0x{case3_target:X}")

case4_je_src = base + 0x2D
case4_target = case4_je_src + 0x2D
print(f"case 4 je target: RVA 0x{case4_target:X}")

fallthrough_src = base + 0x2F
fallthrough_target = fallthrough_src + 0x39
print(f"fallthrough jmp target: RVA 0x{fallthrough_target:X}")
