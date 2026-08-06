import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Let's inspect bytes before and after 0x624162 (RVA 0x624D62)
off2 = 0x624162
print("Bytes at 0x624D62 (Fix 2):", ' '.join(f'{b:02X}' for b in data[off2-16:off2+32]))

off1 = 0x623F0D
print("Bytes at 0x624B0D (Fix 1):", ' '.join(f'{b:02X}' for b in data[off1-16:off1+32]))
