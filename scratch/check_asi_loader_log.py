import os

game_dir = r"C:\Program Files\GOG Galaxy\Games\Death end reQuest\resource\bin"
for f in os.listdir(game_dir):
    if f.lower().endswith(".log"):
        path = os.path.join(game_dir, f)
        print(f"--- Log: {f} ---")
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as file:
                lines = file.readlines()
                for line in lines[-20:]:  # Print last 20 lines
                    print(line.strip())
        except Exception as e:
            print("Failed to read:", e)
