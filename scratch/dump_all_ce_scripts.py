import re

with open('Death end Re;Quest.CT', 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()

entries = re.findall(r'<CheatEntry>.*?<Description>"(.*?)"</Description>.*?<AssemblerScript>(.*?)</AssemblerScript>.*?</CheatEntry>', content, re.DOTALL)

for desc, script in entries:
    print(f"=== {desc} ===")
    print(script[:500])
    print("\n" + "="*40 + "\n")
