import re

with open('Death end Re;Quest.CT', 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()
    
descriptions = re.findall(r'<Description>"(.*?)"</Description>', content)
for i, d in enumerate(descriptions):
    if 'Fix' in d or 'FPS' in d or 'Speed' in d or 'Animation' in d or 'Effect' in d:
        print(f"[{i}] {d}")
