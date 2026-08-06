import re

with open('Death end Re;Quest.CT', 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()

entries = content.split('<CheatEntry>')
for entry in entries:
    if '<Description>"Fixes"</Description>' in entry:
        print(entry[:2000]) # print first 2000 chars of entry to see its script
