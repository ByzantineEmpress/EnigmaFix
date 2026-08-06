import re

ct_path = r"Death end Re;Quest.CT"
with open(ct_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

# Find all CheatEntry nodes
entries = re.findall(r"<CheatEntry>.*?</CheatEntry>", content, re.DOTALL)
print(f"Total entries found: {len(entries)}")

for entry in entries:
    desc_match = re.search(r"<Description>\"(.*?)\"</Description>", entry)
    script_match = re.search(r"<AssemblerScript>(.*?)</AssemblerScript>", entry, re.DOTALL)
    if desc_match and script_match:
        desc = desc_match.group(1)
        script = script_match.group(1).strip()
        # Check if the description or script contains speed/framerate/decouple/animation/delta
        search_str = (desc + " " + script).lower()
        if any(w in search_str for w in ["speed", "framerate", "decouple", "animation", "delta", "dt"]):
            print("================================================================================")
            print(f"Description: {desc}")
            print("================================================================================")
            print(script)
            print("\n")
