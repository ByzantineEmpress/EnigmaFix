import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Pattern: 3C 01 75 12 F3 0F 59 ?? ?? ?? ?? ?? F3 0F 59 15 ?? ?? ?? ?? EB 10
# Bytes: 3C 01 75 12 F3 0F 59 0D (8 bytes) ... (4 bytes) F3 0F 59 15 (4 bytes) (4 bytes) EB 10 (2 bytes)
def match_pattern(offset):
    b = data[offset:offset+25]
    if len(b) < 25: return False
    return (b[0:7] == b"\x3C\x01\x75\x12\xF3\x0F\x59" and 
            b[12:16] == b"\xF3\x0F\x59\x15" and 
            b[20:22] == b"\xEB\x10")

matches = []
for i in range(len(data) - 25):
    if match_pattern(i):
        matches.append(hex(i))

print(f"Matches count: {len(matches)}")
for m in matches:
    print(m)
