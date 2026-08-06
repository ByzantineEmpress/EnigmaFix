import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

# Test signature: 3C 01 75 12 F3 0F 59
# 55 14 3C 01 75 12 F3 0F 59
pattern = b"\x3C\x01\x75\x12\xF3\x0F\x59"
matches = []
for i in range(len(data) - len(pattern)):
    if data[i:i+len(pattern)] == pattern:
        matches.append(hex(i))

print(f"Pattern matches ({len(matches)}):", matches)
