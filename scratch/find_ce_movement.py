import re

ct_path = r"Death end Re;Quest.CT"
with open(ct_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

entries = re.findall(r"<CheatEntry>.*?</CheatEntry>", content, re.DOTALL)

for entry in entries:
    desc_match = re.search(r"<Description>\"(.*?)\"</Description>", entry)
    if desc_match:
        desc = desc_match.group(1)
        if any(w in desc.lower() for w in ["walk", "move", "run", "speed", "bug", "wing", "lily"]):
            print("================================================================================")
            print(f"Description: {desc}")
            print("================================================================================")
            script_match = re.search(r"<AssemblerScript>(.*?)</AssemblerScript>", entry, re.DOTALL)
            if script_match:
                print(script_match.group(1).strip())
            else:
                print("No script for this entry.")
            print("\n")
