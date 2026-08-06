import struct

exe_path = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin\Application.exe"
with open(exe_path, "rb") as f:
    data = f.read()

sig2 = b"\xF3\x0F\x10\x4D\x10\xF3\x0F\x10\x55\x14\x3C\x01\x75\x12"
matches2 = [hex(i) for i in range(len(data)) if data[i:i+len(sig2)] == sig2]
print(f"Fix 2 matches ({len(matches2)}):", matches2)

sig1 = b"\xF3\x0F\x10\x4C\x24\x50\xF3\x0F\x10\x54\x24\x54\x3C\x01\x75\x12"
matches1 = [hex(i) for i in range(len(data)) if data[i:i+len(sig1)] == sig1]
print(f"Fix 1 matches ({len(matches1)}):", matches1)
