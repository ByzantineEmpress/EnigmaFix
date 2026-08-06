import re

ct_path = r"Death end Re;Quest.CT"
with open(ct_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

# Find all occurrences of "Application.exe"+E32... or just E32...
matches = re.findall(r"E32[0-9A-Fa-f]{3}", content)
unique_matches = sorted(list(set(matches)))
print(f"Unique E32 RVAs: {unique_matches}")

# Also let's print lines matching them
with open(ct_path, "r", encoding="utf-8", errors="ignore") as f:
    for idx, line in enumerate(f):
        if any(m in line for m in unique_matches):
            if "<Description>" in line or "alloc" in line or "mov" in line or "mul" in line or "div" in line or "add" in line:
                print(f"{idx+1}: {line.strip()}")
